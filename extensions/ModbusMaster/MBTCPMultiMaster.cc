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
#include <cmath>
#include <limits>
#include <sstream>
#include <Exceptions.h>
#include <extensions/Extensions.h>
#include "MBTCPMultiMaster.h"
#include "modbus/MBLogSugar.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// -----------------------------------------------------------------------------
MBTCPMultiMaster::MBTCPMultiMaster( UniSetTypes::ObjectId objId, UniSetTypes::ObjectId shmId,
									const std::shared_ptr<SharedMemory>& ic, const std::string& prefix ):
	MBExchange(objId, shmId, ic, prefix),
	force_disconnect(true)
{
	tcpMutex.setName(myname + "_tcpMutex");

	if( objId == DefaultObjectId )
		throw UniSetTypes::SystemError("(MBTCPMultiMaster): objId=-1?!! Use --" + prefix + "-name" );

	auto conf = uniset_conf();

	prop_prefix = initPropPrefix("tcp_");
	mbinfo << myname << "(init): prop_prefix=" << prop_prefix << endl;

	UniXML::iterator it(cnode);

	checktime = conf->getArgPInt("--" + prefix + "-checktime", it.getProp("checktime"), 5000);
	force_disconnect = conf->getArgInt("--" + prefix + "-persistent-connection", it.getProp("persistent_connection")) ? false : true;

	int ignore_timeout = conf->getArgPInt("--" + prefix + "-ignore-timeout", it.getProp("ignore_timeout"), ptReopen.getInterval());

	UniXML::iterator it1(it);

	if( !it1.find("GateList") )
	{
		ostringstream err;
		err << myname << "(init): not found <GateList>";
		mbcrit << err.str() << endl;
		throw UniSetTypes::SystemError(err.str());
	}

	if( !it1.goChildren() )
	{
		ostringstream err;
		err << myname << "(init): empty <GateList> ?!";
		mbcrit << err.str() << endl;
		throw UniSetTypes::SystemError(err.str());
	}

	for( ; it1.getCurrent(); it1++ )
	{
		if( it1.getIntProp("ignore") )
		{
			mbinfo << myname << "(init): IGNORE " <<  it1.getProp("ip") << ":" << it1.getProp("port") << endl;
			continue;
		}

		MBSlaveInfo sinf;
		sinf.ip = it1.getProp("ip");

		if( sinf.ip.empty() )
		{
			ostringstream err;
			err << myname << "(init): ip='' in <GateList>";
			mbcrit << err.str() << endl;
			throw UniSetTypes::SystemError(err.str());
		}

		sinf.port = it1.getIntProp("port");

		if( sinf.port <= 0 )
		{
			ostringstream err;
			err << myname << "(init): ERROR: port=''" << sinf.port << " for ip='" << sinf.ip << "' in <GateList>";
			mbcrit << err.str() << endl;
			throw UniSetTypes::SystemError(err.str());
		}

		if( !it1.getProp("respondSensor").empty() )
		{
			sinf.respond_id = conf->getSensorID( it1.getProp("respondSensor") );

			if( sinf.respond_id == DefaultObjectId )
			{
				ostringstream err;
				err << myname << "(init): ERROR: Unknown SensorID for '" << it1.getProp("respondSensor") << "' in <GateList>";
				mbcrit << err.str() << endl;
				throw UniSetTypes::SystemError(err.str());
			}
		}

		sinf.priority = it1.getIntProp("priority");
		sinf.mbtcp = std::make_shared<ModbusTCPMaster>();

		sinf.ptIgnoreTimeout.setTiming( it1.getPIntProp("ignore_timeout", ignore_timeout) );
		sinf.recv_timeout = it1.getPIntProp("recv_timeout", recv_timeout);
		sinf.aftersend_pause = it1.getPIntProp("aftersend_pause", aftersend_pause);
		sinf.sleepPause_usec = it1.getPIntProp("sleepPause_usec", sleepPause_usec);
		sinf.respond_invert = it1.getPIntProp("invert", 0);
		sinf.respond_force = it1.getPIntProp("force", 0);

		sinf.force_disconnect = it.getPIntProp("persistent_connection", !force_disconnect) ? false : true;

		ostringstream n;
		n << sinf.ip << ":" << sinf.port;
		sinf.myname = n.str();

		auto l = loga->create(sinf.myname);
		sinf.mbtcp->setLog(l);

		mblist.push_back(sinf);
		mbinfo << myname << "(init): add slave channel " << sinf.myname << endl;
	}

	if( ic )
		ic->logAgregator()->add(loga);


	if( mblist.empty() )
	{
		ostringstream err;
		err << myname << "(init): empty <GateList>!";
		mbcrit << err.str() << endl;
		throw UniSetTypes::SystemError(err.str());
	}

	mblist.sort();
	mbi = mblist.rbegin();

	if( shm->isLocalwork() )
	{
		readConfiguration();
		rtuQueryOptimization(devices);
		initDeviceList();
	}
	else
		ic->addReadItem( sigc::mem_fun(this, &MBTCPMultiMaster::readItem) );

	pollThread = make_shared<ThreadCreator<MBTCPMultiMaster>>(this, &MBTCPMultiMaster::poll_thread);
	pollThread->setFinalAction(this, &MBTCPMultiMaster::final_thread);
	checkThread = make_shared<ThreadCreator<MBTCPMultiMaster>>(this, &MBTCPMultiMaster::check_thread);
	checkThread->setFinalAction(this, &MBTCPMultiMaster::final_thread);


	// Т.к. при "многоканальном" доступе к slave, смена канала должна происходит сразу после
	// неудачной попытки запросов по одному из каналов, то ПЕРЕОПРЕДЕЛЯЕМ reopen, на timeout..
	ptReopen.setTiming(default_timeout);

	if( mblog->is_info() )
		printMap(devices);
}
// -----------------------------------------------------------------------------
MBTCPMultiMaster::~MBTCPMultiMaster()
{
	if( pollThread )
	{
		pollThread->stop();

		if( pollThread->isRunning() )
			pollThread->join();
	}

	if( checkThread )
	{
		checkThread->stop();

		if( checkThread->isRunning() )
			checkThread->join();
	}

	mbi = mblist.rend();
}
// -----------------------------------------------------------------------------
std::shared_ptr<ModbusClient> MBTCPMultiMaster::initMB( bool reopen )
{
	if( mb )
		ptInitChannel.reset();

	// просто движемся по кругу (т.к. связь не проверяется)
	// движемся в обратном порядке, т.к. сортировка по возрастанию приоритета
	if( checktime <= 0 )
	{
		++mbi;

		if( mbi == mblist.rend() )
			mbi = mblist.rbegin();

		mbi->init(mblog);
		mb = mbi->mbtcp;
		return mbi->mbtcp;
	}

	{
		uniset_rwmutex_wrlock l(tcpMutex);

		// сперва надо обновить все ignore
		// т.к. фактически флаги выставляются и сбрасываются только здесь
		for( auto it = mblist.rbegin(); it != mblist.rend(); ++it )
			it->ignore = !it->ptIgnoreTimeout.checkTime();

		// если reopen=true - значит почему-то по текущему каналу связи нет (хотя соединение есть)
		// тогда выставляем ему признак игнорирования
		if( mbi != mblist.rend() && reopen )
		{
			mbi->setUse(false);
			mbi->ignore = true;
			mbi->ptIgnoreTimeout.reset();
			mbwarn << myname << "(initMB): set ignore=true for " << mbi->ip << ":" << mbi->port << endl;
		}

		// Если по текущему каналу связь есть (и мы его не игнорируем), то возвращаем его
		if( mbi != mblist.rend() && !mbi->ignore && mbi->respond )
		{
			// ещё раз проверим соединение (в неблокирующем режиме)
			mbi->respond = mbi->check();

			if( mbi->respond && (mbi->mbtcp->isConnection() || mbi->init(mblog)) )
			{
				mblog4 << myname << "(initMB): SELECT CHANNEL " << mbi->ip << ":" << mbi->port << endl;
				mb = mbi->mbtcp;
				mbi->setUse(true);
				return mbi->mbtcp;
			}

			mbi->setUse(false);
		}

		if( mbi != mblist.rend() )
			mbi->mbtcp->forceDisconnect();
	}

	// проходим по списку (в обратном порядке, т.к. самый приоритетный в конце)
	for( auto it = mblist.rbegin(); it != mblist.rend(); ++it )
	{
		uniset_rwmutex_wrlock l(tcpMutex);

		if( it->respond && !it->ignore && it->init(mblog) )
		{
			mbi = it;
			mb = mbi->mbtcp;
			mbi->setUse(true);
			mblog4 << myname << "(initMB): SELECT CHANNEL " << mbi->ip << ":" << mbi->port << endl;
			return it->mbtcp;
		}
	}

	// если дошли сюда.. значит не нашли ни одного канала..
	// но т.к. мы пропускали те, которые в ignore
	// значит сейчас просто находим первый у кого есть связь и делаем его главным
	for( auto it = mblist.rbegin(); it != mblist.rend(); ++it )
	{
		uniset_rwmutex_wrlock l(tcpMutex);

		if( it->respond && it->check() && it->init(mblog) )
		{
			mbi = it;
			mb = mbi->mbtcp;
			mbi->ignore = false;
			mbi->setUse(true);
			mblog4 << myname << "(initMB): SELECT CHANNEL " << mbi->ip << ":" << mbi->port << endl;
			return it->mbtcp;
		}
	}

	// значит всё-таки связи реально нет...
	{
		uniset_rwmutex_wrlock l(tcpMutex);
		mbi = mblist.rend();
		mb = nullptr;
	}

	return 0;
}
// -----------------------------------------------------------------------------
void MBTCPMultiMaster::final_thread()
{
	setProcActive(false);
}
// -----------------------------------------------------------------------------

bool MBTCPMultiMaster::MBSlaveInfo::check()
{
	return mbtcp->checkConnection(ip, port, recv_timeout);
}
// -----------------------------------------------------------------------------
bool MBTCPMultiMaster::MBSlaveInfo::init( std::shared_ptr<DebugStream>& mblog )
{
	try
	{
		ost::Thread::setException(ost::Thread::throwException);

		mbinfo << myname << "(init): connect..." << endl;

		mbtcp->connect(ip, port);
		mbtcp->setForceDisconnect(force_disconnect);

		if( recv_timeout > 0 )
			mbtcp->setTimeout(recv_timeout);

		// if( !initOK )
		{
			mbtcp->setSleepPause(sleepPause_usec);
			mbtcp->setAfterSendPause(aftersend_pause);

			if( mbtcp->isConnection() )
				mbinfo << "(init): " << myname << " connect OK" << endl;

			initOK = true;
		}
		return mbtcp->isConnection();
	}
	catch( ModbusRTU::mbException& ex )
	{
		mbwarn << "(init): " << ex << endl;
	}
	catch( const ost::Exception& e )
	{
		mbwarn << myname << "(init): Can`t create socket " << ip << ":" << port << " err: " << e.getString() << endl;
	}
	catch(...)
	{
		mbwarn << "(init): " << myname << " catch ..." << endl;
	}

	initOK = false;
	return false;
}
// -----------------------------------------------------------------------------
void MBTCPMultiMaster::sysCommand( const UniSetTypes::SystemMessage* sm )
{
	MBExchange::sysCommand(sm);

	if( sm->command == SystemMessage::StartUp )
	{
		pollThread->start();

		if( checktime > 0 )
			checkThread->start();
	}
}
// -----------------------------------------------------------------------------
void MBTCPMultiMaster::poll_thread()
{
	// ждём начала работы..(см. MBExchange::activateObject)
	while( !checkProcActive() )
	{
		UniSetTypes::uniset_rwmutex_rlock l(mutex_start);
	}

	// работаем..
	while( checkProcActive() )
	{
		try
		{
			if( sidExchangeMode != DefaultObjectId && force )
				exchangeMode = shm->localGetValue(itExchangeMode, sidExchangeMode);
		}
		catch( std::exception& ex )
		{
			mbwarn << myname << "(poll_thread): "  << ex.what() << endl;
		}

		try
		{
			poll();
		}
		catch( std::exception& ex)
		{
			mbwarn << myname << "(poll_thread): "  << ex.what() << endl;
		}

		if( !checkProcActive() )
			break;

		msleep(polltime);
	}
}
// -----------------------------------------------------------------------------
void MBTCPMultiMaster::check_thread()
{
	while( checkProcActive() )
	{
		for( auto it = mblist.begin(); it != mblist.end(); ++it )
		{
			try
			{
				// сбрасываем флаг ignore..раз время вышло.
				it->ignore = !it->ptIgnoreTimeout.checkTime();

				// Если use=1" связь не проверяем и считаем что связь есть..
				bool r = ( it->use ? true : it->check() );

				mblog4 << myname << "(check): " << it->myname << " " << ( r ? "OK" : "FAIL" ) << endl;

				try
				{
					if( it->respond_id != DefaultObjectId && (it->respond_force || !it->respond_init || r != it->respond) )
					{
						bool set = it->respond_invert ? !r : r;
						shm->localSetValue(it->respond_it, it->respond_id, (set ? 1 : 0), getId());
						it->respond_init = true;
					}
				}
				catch( const Exception& ex )
				{
					mbcrit << myname << "(check): (respond) " << ex << std::endl;
				}
				catch( const std::exception& ex )
				{
					mbcrit << myname << "(check): (respond) " << ex.what() << std::endl;
				}


				{
					uniset_rwmutex_wrlock l(tcpMutex);
					it->respond = r;
				}
			}
			catch( const std::exception& ex )
			{
				mbcrit << myname << "(check): (respond) " << ex.what() << std::endl;
			}

			if( !checkProcActive() )
				break;
		}

		if( !checkProcActive() )
			break;

		msleep(checktime);
	}
}
// -----------------------------------------------------------------------------
void MBTCPMultiMaster::initIterators()
{
	MBExchange::initIterators();

	for( auto && it : mblist )
		shm->initIterator(it.respond_it);
}
// -----------------------------------------------------------------------------
void MBTCPMultiMaster::sigterm( int signo )
{
	if( pollThread )
	{
		pollThread->stop();

		if( pollThread->isRunning() )
			pollThread->join();
	}

	if( checkThread )
	{
		checkThread->stop();

		if( checkThread->isRunning() )
			checkThread->join();
	}

	try
	{
		MBExchange::sigterm(signo);
	}
	catch( const std::exception& ex )
	{
		mbcrit << myname << "(sigterm): " << ex.what() << std::endl;
	}

	//	catch( ... )
	//	{
	//		std::exception_ptr p = std::current_exception();
	//		std::clog << (p ? p.__cxa_exception_type()->name() : "null") << std::endl;
	//	}
}

// -----------------------------------------------------------------------------
void MBTCPMultiMaster::help_print( int argc, const char* const* argv )
{
	cout << "Default: prefix='mbtcp'" << endl;
	MBExchange::help_print(argc, argv);
	cout << endl;
	cout << " Настройки протокола TCP(MultiMaster): " << endl;
	cout << "--prefix-persistent-connection 0,1 - Не закрывать соединение на каждом цикле опроса" << endl;
	cout << "--prefix-checktime                 - период проверки связи по каналам (<GateList>)" << endl;
	cout << "--prefix-ignore-timeout            - Timeout на повторную попытку использования канала после 'reopen-timeout'. По умолчанию: reopen-timeout * 3" << endl;
	cout << endl;
	cout << " ВНИМАНИЕ! '--prefix-reopen-timeout' для MBTCPMultiMaster НЕ ДЕЙСТВУЕТ! " << endl;
	cout << " Смена канала происходит по --prefix-timeout. " << endl;
	cout << " При этом следует учитывать блокировку отключаемого канала на время: --prefix-ignore-timeout" << endl;
}
// -----------------------------------------------------------------------------
std::shared_ptr<MBTCPMultiMaster> MBTCPMultiMaster::init_mbmaster( int argc, const char* const* argv,
		UniSetTypes::ObjectId icID, const std::shared_ptr<SharedMemory>& ic,
		const std::string& prefix )
{
	auto conf = uniset_conf();

	string name = conf->getArgParam("--" + prefix + "-name", "MBTCPMultiMaster1");

	if( name.empty() )
	{
		dcrit << "(MBTCPMultiMaster): Не задан name'" << endl;
		return 0;
	}

	ObjectId ID = conf->getObjectID(name);

	if( ID == UniSetTypes::DefaultObjectId )
	{
		dcrit << "(MBTCPMultiMaster): идентификатор '" << name
			  << "' не найден в конф. файле!"
			  << " в секции " << conf->getObjectsSection() << endl;
		return 0;
	}

	dinfo << "(MBTCPMultiMaster): name = " << name << "(" << ID << ")" << endl;
	return make_shared<MBTCPMultiMaster>(ID, icID, ic, prefix);
}
// -----------------------------------------------------------------------------
const std::string MBTCPMultiMaster::MBSlaveInfo::getShortInfo() const
{
	ostringstream s;
	s << myname << " respond=" << respond
	  << " (respond_id=" << respond_id
	  << " respond_invert=" << respond_invert
	  << " recv_timeout=" << recv_timeout
	  << " resp_force=" << respond_force
	  << " use=" << use
	  << " ignore=" << ( ptIgnoreTimeout.checkTime() ? "0" : "1")
	  << " priority=" << priority
	  << " persistent-connection=" << !force_disconnect
	  << ")";

	return std::move(s.str());
}
// -----------------------------------------------------------------------------
UniSetTypes::SimpleInfo* MBTCPMultiMaster::getInfo( CORBA::Long userparam )
{
	UniSetTypes::SimpleInfo_var i = MBExchange::getInfo(userparam);

	ostringstream inf;

	inf << i->info << endl;
	inf << "Gates: " << (checktime <= 0 ? "/ check connections DISABLED /" : "") << endl;

	for( const auto& m : mblist )
		inf << "   " << m.getShortInfo() << endl;

	inf << endl;

	i->info = inf.str().c_str();
	return i._retn();
}
// ----------------------------------------------------------------------------

