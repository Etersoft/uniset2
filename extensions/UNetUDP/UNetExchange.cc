/*
 * Copyright (c) 2015 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Lesser Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
// -------------------------------------------------------------------------
#include <sstream>
#include <iomanip>
#include "Exceptions.h"
#include "Extensions.h"
#include "UNetExchange.h"
#include "UNetLogSugar.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
using namespace uniset::extensions;
// -----------------------------------------------------------------------------
UNetExchange::UNetExchange(uniset::ObjectId objId, uniset::ObjectId shmId, const std::shared_ptr<SharedMemory>& ic, const std::string& prefix ):
	UniSetObject(objId),
	initPause(0),
	activated(false),
	no_sender(false)
{
	if( objId == DefaultObjectId )
		throw uniset::SystemError("(UNetExchange): objId=-1?!! Use --" + prefix + "-unet-name" );

	auto conf = uniset_conf();

	cnode = conf->getNode(myname);

	if( cnode == NULL )
		throw uniset::SystemError("(UNetExchange): Not found conf-node for " + myname );

	unetlog = make_shared<DebugStream>();
	unetlog->setLogName(myname);
	conf->initLogStream(unetlog, prefix + "-log");

	loga = make_shared<LogAgregator>(myname + "-loga");
	loga->add(unetlog);
	loga->add(ulog());
	loga->add(dlog());

	shm = make_shared<SMInterface>(shmId, ui, objId, ic);

	UniXML::iterator it(cnode);

	logserv = make_shared<LogServer>(loga);
	logserv->init( prefix + "-logserver", cnode );

	if( findArgParam("--" + prefix + "-run-logserver", conf->getArgc(), conf->getArgv()) != -1 )
	{
		logserv_host = conf->getArg2Param("--" + prefix + "-logserver-host", it.getProp("logserverHost"), "localhost");
		logserv_port = conf->getArgPInt("--" + prefix + "-logserver-port", it.getProp("logserverPort"), getId());
	}

	// определяем фильтр
	s_field = conf->getArgParam("--" + prefix + "-filter-field");
	s_fvalue = conf->getArgParam("--" + prefix + "-filter-value");
	unetinfo << myname << "(init): read filter-field='" << s_field
			 << "' filter-value='" << s_fvalue << "'" << endl;

	const string n_field(conf->getArgParam("--" + prefix + "-nodes-filter-field"));
	const string n_fvalue(conf->getArgParam("--" + prefix + "-nodes-filter-value"));
	unetinfo << myname << "(init): read nodes-filter-field='" << n_field
			 << "' nodes-filter-value='" << n_fvalue << "'" << endl;

	int recvTimeout = conf->getArgPInt("--" + prefix + "-recv-timeout", it.getProp("recvTimeout"), 5000);
	int prepareTime = conf->getArgPInt("--" + prefix + "-prepare-time", it.getProp("prepareTime"), 2000);
	int evrunTimeout = conf->getArgPInt("--" + prefix + "-evrun-timeout", it.getProp("evrunTimeout"), 60000);
	int sendpause = conf->getArgPInt("--" + prefix + "-sendpause", it.getProp("sendpause"), 100);
	int packsendpause = conf->getArgPInt("--" + prefix + "-packsendpause", it.getProp("packsendpause"), 5);
	int packsendpauseFactor = conf->getArgPInt("--" + prefix + "-packsendpause-factor", it.getProp("packsendpauseFactor"), 0);
	int updatepause = conf->getArgPInt("--" + prefix + "-updatepause", it.getProp("updatepause"), 100);
	int lostTimeout = conf->getArgPInt("--" + prefix + "-lost-timeout", it.getProp("lostTimeout"), 2 * updatepause);
	steptime = conf->getArgPInt("--" + prefix + "-steptime", it.getProp("steptime"), 1000);
	int maxDiff = conf->getArgPInt("--" + prefix + "-maxdifferense", it.getProp("maxDifferense"), 100);
	int checkConnectionPause = conf->getArgPInt("--" + prefix + "-checkconnection-pause", it.getProp("checkConnectionPause"), 10000);
	int initpause = conf->getArgPInt("--" + prefix + "-initpause", it.getProp("initpause"), 5000);
	int recvBufferSize = conf->getArgPInt("--" + prefix + "-recv-buffer-size", it.getProp("recvBufferSize"), 3000);

	std::string updateStrategy = conf->getArg2Param("--" + prefix + "-update-strategy", it.getProp("updateStrategy"), "evloop");

	no_sender = conf->getArgInt("--" + prefix + "-nosender", it.getProp("nosender"));

	std::string nconfname = conf->getArg2Param("--" + prefix + "-nodes-confnode", it.getProp("nodesConfNode"), "nodes");

	xmlNode* nodes = 0;

	if( nconfname == "nodes" )
		nodes = conf->getXMLNodesSection();
	else
	{
		auto xml = conf->getConfXML();
		nodes = conf->findNode(xml->getFirstNode(), nconfname);
	}

	unetinfo << myname << "(init): init from <" << nconfname << ">" << endl;

	if( !nodes )
		throw uniset::SystemError("(UNetExchange): Not found confnode <" + nconfname + ">");

	UniXML::iterator n_it(nodes);

	string default_ip(n_it.getProp("unet_broadcast_ip"));
	string default_ip2(n_it.getProp("unet_broadcast_ip2"));

	if( !n_it.goChildren() )
		throw uniset::SystemError("(UNetExchange): Items not found for <nodes>");

	for( ; n_it.getCurrent(); n_it.goNext() )
	{
		if( n_it.getIntProp("unet_ignore") )
		{
			unetinfo << myname << "(init): unet_ignore.. for " << n_it.getProp("name") << endl;
			continue;
		}

		// проверяем фильтры для подсетей
		if( !uniset::check_filter(n_it, n_field, n_fvalue) )
			continue;

		// Если указано поле unet_broadcast_ip непосредственно у узла - берём его
		// если не указано берём общий broadcast_ip

		string h = { n_it.getProp2("unet_broadcast_ip", default_ip) };
		string h2 = { n_it.getProp2("unet_broadcast_ip2", default_ip2) };

		if( h.empty() )
		{
			ostringstream err;
			err << myname << "(init): Unknown broadcast IP for " << n_it.getProp("name");
			unetcrit << err.str() << endl;
			throw uniset::SystemError(err.str());
		}

		if( h2.empty() )
			unetinfo << myname << "(init): ip2 not used..." << endl;

		// Если указано поле unet_port - используем его
		// Иначе port = идентификатору узла
		int p = n_it.getPIntProp("unet_port", n_it.getIntProp("id"));

		// по умолчанию порт на втором канале такой же как на первом (если не задан отдельно)
		int p2 = n_it.getPIntProp("unet_port2", p);

		string n(n_it.getProp("name"));

		if( n == conf->getLocalNodeName() )
		{
			if( no_sender )
			{
				unetinfo << myname << "(init): sender OFF for this node...("
						 << n_it.getProp("name") << ")" << endl;
				continue;
			}

			unetinfo << myname << "(init): init sender.. my node " << n_it.getProp("name") << endl;
			sender = make_shared<UNetSender>(h, p, shm, false, s_field, s_fvalue, "unet", prefix);
			sender->setSendPause(sendpause);
			sender->setPackSendPause(packsendpause);
			sender->setPackSendPauseFactor(packsendpauseFactor);
			sender->setCheckConnectionPause(checkConnectionPause);
			loga->add(sender->getLog());

			try
			{
				// создаём "писателя" для второго канала если задан
				if( !h2.empty() )
				{
					unetinfo << myname << "(init): init sender2.. my node " << n_it.getProp("name") << endl;
					sender2 = make_shared<UNetSender>(h2, p2, shm, false, s_field, s_fvalue, prefix);
					sender2->setSendPause(sendpause);
					sender2->setCheckConnectionPause(checkConnectionPause);
					loga->add(sender2->getLog());
				}
			}
			catch(...)
			{
				// т.е. это "резервный канал", то игнорируем ошибку его создания
				// при запуске "интерфейс" может быть и не доступен...
				sender2 = 0;
				unetcrit << myname << "(ignore): DON`T CREATE 'UNetSender' for " << h2 << ":" << p2 << endl;
			}

			continue;
		}

		unetinfo << myname << "(init): add UNetReceiver for " << h << ":" << p << endl;

		if( checkExistUNetHost(h, p) )
		{
			unetinfo << myname << "(init): " << h << ":" << p << " already added! Ignore.." << endl;
			continue;
		}

		bool resp_invert = n_it.getIntProp("unet_respond_invert");

		string s_resp_id(n_it.getProp("unet_respond1_id"));
		uniset::ObjectId resp_id = uniset::DefaultObjectId;

		if( !s_resp_id.empty() )
		{
			resp_id = conf->getSensorID(s_resp_id);

			if( resp_id == uniset::DefaultObjectId )
			{
				ostringstream err;
				err << myname << ": " << n_it.getProp("name") << " : Unknown RespondID.. Not found id for '" << s_resp_id << "'" << endl;
				unetcrit << myname << "(init): " << err.str() << endl;
				throw SystemError(err.str());
			}
		}

		string s_resp2_id(n_it.getProp("unet_respond2_id"));
		uniset::ObjectId resp2_id = uniset::DefaultObjectId;

		if( !s_resp2_id.empty() )
		{
			resp2_id = conf->getSensorID(s_resp2_id);

			if( resp2_id == uniset::DefaultObjectId )
			{
				ostringstream err;
				err << myname << ": " << n_it.getProp("name") << " : Unknown RespondID(2).. Not found id for '" << s_resp2_id << "'" << endl;
				unetcrit << myname << "(init): " << err.str() << endl;
				throw SystemError(err.str());
			}
		}

		string s_lp_id(n_it.getProp("unet_lostpackets1_id"));
		uniset::ObjectId lp_id = uniset::DefaultObjectId;

		if( !s_lp_id.empty() )
		{
			lp_id = conf->getSensorID(s_lp_id);

			if( lp_id == uniset::DefaultObjectId )
			{
				ostringstream err;
				err << myname << ": " << n_it.getProp("name") << " : Unknown LostPacketsID.. Not found id for '" << s_lp_id << "'" << endl;
				unetcrit << myname << "(init): " << err.str() << endl;
				throw SystemError(err.str());
			}
		}

		string s_lp2_id(n_it.getProp("unet_lostpackets2_id"));
		uniset::ObjectId lp2_id = uniset::DefaultObjectId;

		if( !s_lp2_id.empty() )
		{
			lp2_id = conf->getSensorID(s_lp2_id);

			if( lp2_id == uniset::DefaultObjectId )
			{
				ostringstream err;
				err << myname << ": " << n_it.getProp("name") << " : Unknown LostPacketsID(2).. Not found id for '" << s_lp2_id << "'" << endl;
				unetcrit << myname << "(init): " << err.str() << endl;
				throw SystemError(err.str());
			}
		}

		string s_lp_comm_id(n_it.getProp("unet_lostpackets_id"));
		uniset::ObjectId lp_comm_id = uniset::DefaultObjectId;

		if( !s_lp_comm_id.empty() )
		{
			lp_comm_id = conf->getSensorID(s_lp_comm_id);

			if( lp_comm_id == uniset::DefaultObjectId )
			{
				ostringstream err;
				err << myname << ": " << n_it.getProp("name") << " : Unknown LostPacketsID(comm).. Not found id for '" << s_lp_comm_id << "'" << endl;
				unetcrit << myname << "(init): " << err.str() << endl;
				throw SystemError(err.str());
			}
		}

		string s_resp_comm_id(n_it.getProp("unet_respond_id"));
		uniset::ObjectId resp_comm_id = uniset::DefaultObjectId;

		if( !s_resp_comm_id.empty() )
		{
			resp_comm_id = conf->getSensorID(s_resp_comm_id);

			if( resp_comm_id == uniset::DefaultObjectId )
			{
				ostringstream err;
				err << myname << ": " << n_it.getProp("name") << " : Unknown RespondID(comm).. Not found id for '" << s_resp_comm_id << "'" << endl;
				unetcrit << myname << "(init): " << err.str() << endl;
				throw SystemError(err.str());
			}
		}

		string s_numchannel_id(n_it.getProp("unet_numchannel_id"));
		uniset::ObjectId numchannel_id = uniset::DefaultObjectId;

		if( !s_numchannel_id.empty() )
		{
			numchannel_id = conf->getSensorID(s_numchannel_id);

			if( numchannel_id == uniset::DefaultObjectId )
			{
				ostringstream err;
				err << myname << ": " << n_it.getProp("name") << " : Unknown NumChannelID.. Not found id for '" << s_numchannel_id << "'" << endl;
				unetcrit << myname << "(init): " << err.str() << endl;
				throw SystemError(err.str());
			}
		}

		string s_channelSwitchCount_id(n_it.getProp("unet_channelswitchcount_id"));
		uniset::ObjectId channelswitchcount_id = uniset::DefaultObjectId;

		if( !s_channelSwitchCount_id.empty() )
		{
			channelswitchcount_id = conf->getSensorID(s_channelSwitchCount_id);

			if( channelswitchcount_id == uniset::DefaultObjectId )
			{
				ostringstream err;
				err << myname << ": " << n_it.getProp("name") << " : Unknown ChannelSwitchCountID.. Not found id for '" << channelswitchcount_id << "'" << endl;
				unetcrit << myname << "(init): " << err.str() << endl;
				throw SystemError(err.str());
			}
		}

		unetinfo << myname << "(init): (node='" << n << "') add  basic receiver "
				 << h << ":" << p << endl;
		auto r = make_shared<UNetReceiver>(h, p, shm, false, prefix);

		loga->add(r->getLog());

		// на всякий принудительно разблокируем,
		// чтобы не зависеть от значения по умолчанию
		r->setLockUpdate(false);

		r->setReceiveTimeout(recvTimeout);
		r->setPrepareTime(prepareTime);
		r->setEvrunTimeout(evrunTimeout);
		r->setLostTimeout(lostTimeout);
		r->setUpdatePause(updatepause);
		r->setCheckConnectionPause(checkConnectionPause);
		r->setInitPause(initpause);
		r->setMaxDifferens(maxDiff);
		r->setRespondID(resp_id, resp_invert);
		r->setLostPacketsID(lp_id);
		r->connectEvent( sigc::mem_fun(this, &UNetExchange::receiverEvent) );
		r->setBufferSize(recvBufferSize);

		shared_ptr<UNetReceiver> r2(nullptr);

		try
		{
			if( !h2.empty() ) // создаём читателя впо второму каналу
			{
				unetinfo << myname << "(init): (node='" << n << "') add reserv receiver "
						 << h2 << ":" << p2 << endl;

				r2 = make_shared<UNetReceiver>(h2, p2, shm, false, prefix);

				loga->add(r2->getLog());

				// т.к. это резервный канал (по началу блокируем его)
				r2->setLockUpdate(true);

				r2->setReceiveTimeout(recvTimeout);
				r2->setPrepareTime(prepareTime);
				r2->setEvrunTimeout(evrunTimeout);
				r2->setLostTimeout(lostTimeout);
				r2->setUpdatePause(updatepause);
				r2->setCheckConnectionPause(checkConnectionPause);
				r2->setInitPause(initpause);
				r2->setMaxDifferens(maxDiff);
				r2->setRespondID(resp2_id, resp_invert);
				r2->setLostPacketsID(lp2_id);
				r2->connectEvent( sigc::mem_fun(this, &UNetExchange::receiverEvent) );
				r2->setBufferSize(recvBufferSize);
			}
		}
		catch(...)
		{
			// т.е. это "резервный канал", то игнорируем ошибку его создания
			// при запуске "интерфейс" может быть и не доступен...
			r2 = 0;
			unetcrit << myname << "(ignore): DON`T CREATE 'UNetReceiver' for " << h2 << ":" << p2 << endl;
		}

		ReceiverInfo ri(r, r2);
		ri.setRespondID(resp_comm_id, resp_invert);
		ri.setLostPacketsID(lp_comm_id);
		ri.setChannelNumID(numchannel_id);
		ri.setChannelSwitchCountID(channelswitchcount_id);
		recvlist.emplace_back( std::move(ri) );
	}

	// -------------------------------
	// ********** HEARTBEAT *************
	string heart = conf->getArgParam("--" + prefix + "-heartbeat-id", it.getProp("heartbeat_id"));

	if( !heart.empty() )
	{
		sidHeartBeat = conf->getSensorID(heart);

		if( sidHeartBeat == DefaultObjectId )
		{
			ostringstream err;
			err << myname << ": не найден идентификатор для датчика 'HeartBeat' " << heart;
			unetcrit << myname << "(init): " << err.str() << endl;
			throw SystemError(err.str());
		}

		int heartbeatTime = conf->getArgPInt("--" + prefix + "-heartbeat-time", it.getProp("heartbeatTime"), conf->getHeartBeatTime());

		if( heartbeatTime )
			ptHeartBeat.setTiming(heartbeatTime);
		else
			ptHeartBeat.setTiming(UniSetTimer::WaitUpTime);

		maxHeartBeat = conf->getArgPInt("--" + prefix + "-heartbeat-max", it.getProp("heartbeat_max"), 10);
		test_id = sidHeartBeat;
	}
	else
	{
		test_id = conf->getSensorID("TestMode_S");

		if( test_id == DefaultObjectId )
		{
			ostringstream err;
			err << myname << "(init): test_id unknown. 'TestMode_S' not found...";
			unetcrit << myname << "(init): " << err.str() << endl;
			throw SystemError(err.str());
		}
	}

	unetinfo << myname << "(init): test_id=" << test_id << endl;

	activateTimeout    = conf->getArgPInt("--" + prefix + "-activate-timeout", 20000);

	if( ic )
		ic->logAgregator()->add(loga);


	vmonit(s_field);
	vmonit(s_fvalue);
	vmonit(maxHeartBeat);
}
// -----------------------------------------------------------------------------
UNetExchange::~UNetExchange()
{
}
// -----------------------------------------------------------------------------
bool UNetExchange::checkExistUNetHost(const std::string& addr, int port ) noexcept
{
	for( const auto& it : recvlist )
	{
		if( it.r1->getAddress() == addr && it.r1->getPort() == port )
			return true;
	}

	return false;
}
// -----------------------------------------------------------------------------
void UNetExchange::startReceivers()
{
	for( const auto& it : recvlist )
	{
		if( it.r1 )
			it.r1->start();

		if( it.r2 )
			it.r2->start();
	}
}
// -----------------------------------------------------------------------------
bool UNetExchange::waitSMReady()
{
	// waiting for SM is ready...
	int tout = uniset_conf()->getArgPInt("--unet-sm-ready-timeout", "", uniset_conf()->getNCReadyTimeout());

	timeout_t ready_timeout = uniset_conf()->getNCReadyTimeout();

	if( tout > 0 )
		ready_timeout = tout;
	else if( tout < 0 )
		ready_timeout = UniSetTimer::WaitUpTime;

	if( !shm->waitSMreadyWithCancellation(ready_timeout, cancelled, 50) )
	{
		if( !cancelled )
		{
			ostringstream err;
			err << myname << "(waitSMReady): Не дождались готовности SharedMemory к работе в течение " << ready_timeout << " мсек";
			unetcrit << err.str() << endl;
		}

		return false;
	}

	return true;
}
// -----------------------------------------------------------------------------
void UNetExchange::timerInfo( const TimerMessage* tm )
{
	if( !activated )
		return;

	if( tm->id == tmStep )
		step();
}
// -----------------------------------------------------------------------------
void UNetExchange::step() noexcept
{
	if( !activated )
		return;

	if( sidHeartBeat != DefaultObjectId && ptHeartBeat.checkTime() )
	{
		try
		{
			shm->localSetValue(itHeartBeat, sidHeartBeat, maxHeartBeat, getId());
			ptHeartBeat.reset();
		}
		catch( const std::exception& ex )
		{
			unetcrit << myname << "(step): (hb) " << ex.what() << std::endl;
		}
	}

	for( auto&& it : recvlist )
		it.step(shm, myname, unetlog);
}

// -----------------------------------------------------------------------------
void UNetExchange::ReceiverInfo::step( const std::shared_ptr<SMInterface>& shm, const std::string& myname, std::shared_ptr<DebugStream>& unetlog ) noexcept
{
	try
	{
		if( sidRespond != DefaultObjectId )
		{
			bool resp = ( (r1 && r1->isRecvOK()) || (r2 && r2->isRecvOK()) );

			if( respondInvert )
				resp = !resp;

			// сохраняем только если закончилось время на начальную инициализацию
			if( (r1 && r1->isInitOK()) || (r2 && r2->isInitOK()) )
				shm->localSetValue(itRespond, sidRespond, resp, shm->ID());
		}
	}
	catch( const std::exception& ex )
	{
		unetcrit << myname << "(ReceiverInfo::step): (respond): " << ex.what() << std::endl;
	}

	try
	{
		if( sidLostPackets != DefaultObjectId )
		{
			long l = 0;

			if( r1 )
				l += r1->getLostPacketsNum();

			if( r2 )
				l += r2->getLostPacketsNum();

			shm->localSetValue(itLostPackets, sidLostPackets, l, shm->ID());
		}
	}
	catch( const std::exception& ex )
	{
		unetcrit << myname << "(ReceiverInfo::step): (lostpackets): " << ex.what() << std::endl;
	}

	try
	{
		if( sidChannelNum != DefaultObjectId )
		{
			long c = 0;

			if( r1 && !r1->isLockUpdate() )
				c = 1;

			if( r2 && !r2->isLockUpdate() )
				c = 2;

			shm->localSetValue(itChannelNum, sidChannelNum, c, shm->ID());
		}
	}
	catch( const std::exception& ex )
	{
		unetcrit << myname << "(ReceiverInfo::step): (channelnum): " << ex.what() << std::endl;
	}

	try
	{
		if( sidChannelSwitchCount != DefaultObjectId )
			shm->localSetValue(itChannelSwitchCount, sidChannelSwitchCount, channelSwitchCount, shm->ID());
	}
	catch( const std::exception& ex )
	{
		unetcrit << myname << "(ReceiverInfo::step): (channelSwitchCount): " << ex.what() << std::endl;
	}
}
// -----------------------------------------------------------------------------
void UNetExchange::sysCommand( const uniset::SystemMessage* sm )
{
	switch( sm->command )
	{
		case SystemMessage::StartUp:
		{
			if( !logserv_host.empty() && logserv_port != 0 && !logserv->isRunning() )
			{
				try
				{
					unetinfo << myname << "(init): run log server " << logserv_host << ":" << logserv_port << endl;
					logserv->async_run(logserv_host, logserv_port);
				}
				catch( std::exception& ex )
				{
					unetwarn << myname << "(init): run logserver FAILED. ERR: " << ex.what() << endl;
				}
			}

			if( !waitSMReady() )
			{
				if( !cancelled )
				{
					//					std::terminate();
					uterminate();
				}

				return;
			}

			// подождать пока пройдёт инициализация датчиков
			// см. activateObject()
			msleep(initPause);
			PassiveTimer ptAct(activateTimeout);

			while( !cancelled && !activated && !ptAct.checkTime() )
			{
				cout << myname << "(sysCommand): wait activate..." << endl;
				msleep(300);

				if( activated )
					break;
			}

			if( cancelled )
				return;

			if( !activated )
				unetcrit << myname << "(sysCommand): ************* don`t activate?! ************" << endl;

			{
				uniset::uniset_rwmutex_rlock l(mutex_start);

				if( shm->isLocalwork() )
					askSensors(UniversalIO::UIONotify);
			}

			askTimer(tmStep, steptime);
			startReceivers();

			if( sender )
				sender->start();

			if( sender2 )
				sender2->start();
		}
		break;

		case SystemMessage::FoldUp:
		case SystemMessage::Finish:
			if( shm->isLocalwork() )
				askSensors(UniversalIO::UIODontNotify);

			break;

		case SystemMessage::WatchDog:
		{
			startReceivers(); // если уже запущены, то это приведёт к вызову UNetReceiver::forceUpdate() ( см. UNetReceiver::start() )

			// ОПТИМИЗАЦИЯ (защита от двойного перезаказа при старте)
			// Если идёт автономная работа, то нужно заказывать датчики
			// если запущены в одном процессе с SharedMemory2,
			// то обрабатывать WatchDog не надо, т.к. мы и так ждём готовности SM
			// при заказе датчиков, а если SM вылетит, то вместе с этим процессом(UNetExchange)
			if( shm->isLocalwork() )
				askSensors(UniversalIO::UIONotify);
		}
		break;

		case SystemMessage::LogRotate:
		{
			unetlogany << myname << "(sysCommand): logRotate" << std::endl;
			string fname = unetlog->getLogFile();

			if( !fname.empty() )
			{
				unetlog->logFile(fname, true);
				unetlogany << myname << "(sysCommand): ***************** dlog LOG ROTATE *****************" << std::endl;
			}
		}
		break;

		default:
			break;
	}
}
// ------------------------------------------------------------------------------------------
void UNetExchange::askSensors( UniversalIO::UIOCommand cmd )
{
	if( !shm->waitSMworking(test_id, activateTimeout, 50) )
	{
		ostringstream err;
		err << myname
			<< "(askSensors): Не дождались готовности(work) SharedMemory к работе в течение "
			<< activateTimeout << " мсек";

		unetcrit << err.str() << endl << flush;
		uterminate();
		throw SystemError(err.str());
	}

	if( sender )
		sender->askSensors(cmd);

	if( sender2 )
		sender2->askSensors(cmd);
}
// ------------------------------------------------------------------------------------------
void UNetExchange::sensorInfo( const uniset::SensorMessage* sm )
{
	if( sender )
		sender->updateSensor( sm->id, sm->value );

	if( sender2 )
		sender2->updateSensor( sm->id, sm->value );
}
// ------------------------------------------------------------------------------------------
bool UNetExchange::activateObject()
{
	// блокирование обработки Starsp
	// пока не пройдёт инициализация датчиков
	// см. sysCommand()
	{
		activated = false;
		uniset::uniset_rwmutex_wrlock l(mutex_start);
		UniSetObject::activateObject();
		initIterators();
		activated = true;
	}

	return true;
}
// ------------------------------------------------------------------------------------------
bool UNetExchange::deactivateObject()
{
	cancelled = true;

	if( activated )
	{
		unetinfo << myname << "(deactivateObject): disactivate.." << endl;
		activated = false;
		termReceivers();
		termSenders();
	}

	return UniSetObject::deactivateObject();
}
// ------------------------------------------------------------------------------------------
void UNetExchange::termReceivers()
{
	for( const auto& it : recvlist )
	{
		try
		{
			if( it.r1 )
				it.r1->stop();
		}
		catch(...) {}

		try
		{
			if( it.r2 )
				it.r2->stop();
		}
		catch(...) {}
	}
}
// ------------------------------------------------------------------------------------------
void UNetExchange::termSenders()
{
	try
	{
		if( sender )
			sender->stop();
	}
	catch(...) {}

	try
	{
		if( sender2 )
			sender2->stop();
	}
	catch(...) {}
}
// ------------------------------------------------------------------------------------------
void UNetExchange::initIterators() noexcept
{
	shm->initIterator(itHeartBeat);

	if( sender )
		sender->initIterators();

	if( sender2 )
		sender2->initIterators();

	for( auto&& it : recvlist )
		it.initIterators(shm);
}
// -----------------------------------------------------------------------------
void UNetExchange::help_print( int argc, const char* argv[] ) noexcept
{
	cout << "Default prefix='unet'" << endl;
	cout << "--prefix-name NameID             - Идентификтора процесса." << endl;
	cout << "--prefix-recv-timeout msec       - Время для фиксации события 'отсутсвие связи'" << endl;
	cout << "--prefix-prepare-time msec       - Время необходимое на подготовку (восстановление связи) при переключении на другой канал" << endl;
	cout << "--prefix-lost-timeout msec       - Время ожидания заполнения 'дырки' между пакетами. По умолчанию 5000 мсек." << endl;
	cout << "--prefix-sendpause msec          - Пауза между посылками. По умолчанию 100" << endl;
	cout << "--prefix-updatepause msec        - Пауза между обновлением информации в SM (Корелирует с sendpause). По умолчанию 100" << endl;
	cout << "--prefix-steptime msec           - Пауза между обновлением информации о связи с узлами." << endl;
	cout << "--prefix-checkconnection-pause msec  - Пауза между попытками открыть соединение (если это не удалось до этого). По умолчанию: 10000 (10 сек)" << endl;
	cout << "--prefix-maxdifferense num       - Маскимальная разница в номерах пакетов для фиксации события 'потеря пакетов' " << endl;
	cout << "--prefix-nosender [0,1]          - Отключить посылку." << endl;
	cout << "--prefix-recv-buffer-size sz     - Размер циклического буфера для приёма сообщений. По умолчанию: 3000" << endl;
	cout << "--prefix-sm-ready-timeout msec   - Время ожидание я готовности SM к работе. По умолчанию 120000" << endl;
	cout << "--prefix-filter-field name       - Название фильтрующего поля при формировании списка датчиков посылаемых данным узлом" << endl;
	cout << "--prefix-filter-value name       - Значение фильтрующего поля при формировании списка датчиков посылаемых данным узлом" << endl;
	cout << endl;
	cout << "--prefix-nodes-confnode name     - <Узел> откуда считывается список узлов. Default: <nodes>" << endl;
	cout << "--prefix-nodes-filter-field name - Фильтрующее поле для списка узлов" << endl;
	cout << "--prefix-nodes-filter-value name - Значение фильтрующего поля для списка узлов" << endl;
	cout << endl;
	cout << " Logs: " << endl;
	cout << "--prefix-log-...             - log control" << endl;
	cout << "             add-levels ..." << endl;
	cout << "             del-levels ..." << endl;
	cout << "             set-levels ..." << endl;
	cout << "             logfile filaname" << endl;
	cout << "             no-debug " << endl;
	cout << " LogServer: " << endl;
	cout << "--prefix-run-logserver       - run logserver. Default: localhost:id" << endl;
	cout << "--prefix-logserver-host ip   - listen ip. Default: localhost" << endl;
	cout << "--prefix-logserver-port num  - listen port. Default: ID" << endl;
	cout << LogServer::help_print("prefix-logserver") << endl;
}
// -----------------------------------------------------------------------------
std::shared_ptr<UNetExchange> UNetExchange::init_unetexchange(int argc, const char* const argv[], uniset::ObjectId icID,
		const std::shared_ptr<SharedMemory>& ic, const std::string& prefix )
{
	auto conf = uniset_conf();

	string p("--" + prefix + "-name");
	string name = conf->getArgParam(p, "UNetExchange1");

	if( name.empty() )
	{
		cerr << "(unetexchange): Не задан name'" << endl;
		return 0;
	}

	ObjectId ID = conf->getObjectID(name);

	if( ID == uniset::DefaultObjectId )
	{
		cerr << "(unetexchange): Not found ObjectID for '" << name
			 << " in section '" << conf->getObjectsSection() << "'" << endl;
		return 0;
	}

	dinfo << "(unetexchange): name = " << name << "(" << ID << ")" << endl;
	return make_shared<UNetExchange>(ID, icID, ic, prefix);
}
// -----------------------------------------------------------------------------
void UNetExchange::receiverEvent( const shared_ptr<UNetReceiver>& r, UNetReceiver::Event ev ) noexcept
{
	for( auto&& it : recvlist )
	{
		if( it.r1 == r )
		{
			if( ev == UNetReceiver::evTimeout )
			{
				// если нет второго канала или нет связи
				// то и переключать не надо
				if( !it.r2 || !it.r2->isRecvOK() )
					return;

				// пропала связь по первому каналу...
				// переключаемся на второй
				it.r1->setLockUpdate(true);
				it.r2->setLockUpdate(false);
				it.channelSwitchCount++;

				dlog8 << myname << "(event): " << r->getName()
					  << ": timeout for channel1.. select channel 2" << endl;
			}
			else if( ev == UNetReceiver::evOK )
			{
				// если связь восстановилась..
				// проверяем, а что там со вторым каналом
				// если у него связи нет, то забираем себе..
				if( !it.r2 || !it.r2->isRecvOK() )
				{
					it.r1->setLockUpdate(false);

					if( it.r2 )
						it.r2->setLockUpdate(true);

					// если какой-то канал уже работал
					// то увеличиваем счётчик переключений
					// а если ещё не работал, значит это просто первое включение канала
					// а не переключение
					if( it.channelSwitchCount > 0 )
						it.channelSwitchCount++;

					dlog8 << myname << "(event): " << r->getName()
						  << ": link failed for channel2.. select again channel1.." << endl;
				}
			}

			return;
		}

		if( it.r2 == r )
		{
			if( ev == UNetReceiver::evTimeout )
			{
				// если первого канала нет или нет связи
				// то и переключать не надо
				if( !it.r1 || !it.r1->isRecvOK() )
					return;

				// пропала связь по второму каналу...
				// переключаемся на первый
				it.r1->setLockUpdate(false);
				it.r2->setLockUpdate(true);
				it.channelSwitchCount++;

				dlog8 << myname << "(event): " << r->getName()
					  << ": timeout for channel2.. select channel 1" << endl;
			}
			else if( ev == UNetReceiver::evOK )
			{
				// если связь восстановилась..
				// проверяем, а что там со первым каналом
				// если у него связи нет, то забираем себе..
				if( !it.r1 || !it.r1->isRecvOK() )
				{
					if( it.r1 )
						it.r1->setLockUpdate(true);

					it.r2->setLockUpdate(false);

					// если какой-то канал уже работал
					// то увеличиваем счётчик переключений
					// а если ещё не работал, значит это просто первое включение канала
					// а не переключение
					if( it.channelSwitchCount > 0 )
						it.channelSwitchCount++;

					dlog8 << myname << "(event): " << r->getName()
						  << ": link failed for channel1.. select again channel2.." << endl;
				}
			}

			return;
		}
	}
}
// -----------------------------------------------------------------------------
uniset::SimpleInfo* UNetExchange::getInfo( const char* userparam )
{
	uniset::SimpleInfo_var i = UniSetObject::getInfo(userparam);

	ostringstream inf;

	inf << i->info << endl;
	inf << vmon.pretty_str() << endl;
	inf << endl;

	if( logserv )
	{
		inf << "LogServer: " << logserv_host << ":" << logserv_port
			<< ( logserv->isRunning() ? "   [RUNNIG]" : "   [STOPPED]" )
			<< endl
			<< "         " << logserv->getShortInfo()
			<< endl;
	}
	else
		inf << "LogServer: NONE" << endl;

	inf << endl;
	inf << "Receivers: " << endl;

	for( const auto& r : recvlist )
	{
		inf << "[ " << endl;
		inf << "  chan1: " << ( r.r1 ? r.r1->getShortInfo() : "[ DISABLED ]" ) << endl;
		inf << "  chan2: " << ( r.r2 ? r.r2->getShortInfo() : "[ DISABLED ]" ) << endl;
		inf << "]" << endl;
	}

	inf << endl;

	inf << "Senders: " << endl;
	inf << "[ " << endl;
	inf << "  chan1: " << ( sender ? sender->getShortInfo() : "[ DISABLED ]" ) << endl;
	inf << "  chan2: " << ( sender2 ? sender2->getShortInfo() : "[ DISABLED ]" ) << endl;
	inf << "]" << endl;
	inf << endl;

	i->info = inf.str().c_str();
	return i._retn();
}
// ----------------------------------------------------------------------------

