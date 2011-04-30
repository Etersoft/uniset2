#include <sstream>
#include "Exceptions.h"
#include "Extensions.h"
#include "UNetExchange.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
UNetExchange::UNetExchange( UniSetTypes::ObjectId objId, UniSetTypes::ObjectId shmId, SharedMemory* ic ):
UniSetObject_LT(objId),
shm(0),
initPause(0),
activated(false),
no_sender(false),
sender(0)
{
	if( objId == DefaultObjectId )
		throw UniSetTypes::SystemError("(UNetExchange): objId=-1?!! Use --unet-name" );

//	xmlNode* cnode = conf->getNode(myname);
	cnode = conf->getNode(myname);
	if( cnode == NULL )
		throw UniSetTypes::SystemError("(UNetExchange): Not found conf-node for " + myname );

	shm = new SMInterface(shmId,&ui,objId,ic);

	UniXML_iterator it(cnode);

	// определяем фильтр
	s_field = conf->getArgParam("--unet-filter-field");
	s_fvalue = conf->getArgParam("--unet-filter-value");
	dlog[Debug::INFO] << myname << "(init): read filter-field='" << s_field
						<< "' filter-value='" << s_fvalue << "'" << endl;

	int recvTimeout = conf->getArgPInt("--unet-recv-timeout",it.getProp("recvTimeout"), 5000);
	int lostTimeout = conf->getArgPInt("--unet-lost-timeout",it.getProp("lostTimeout"), recvTimeout);
	int recvpause = conf->getArgPInt("--unet-recvpause",it.getProp("recvpause"), 10);
	int sendpause = conf->getArgPInt("--unet-sendpause",it.getProp("sendpause"), 150);
	int updatepause = conf->getArgPInt("--unet-updatepause",it.getProp("updatepause"), 100);
	steptime = conf->getArgPInt("--unet-steptime",it.getProp("steptime"), 1000);
	int maxDiff = conf->getArgPInt("--unet-maxdifferense",it.getProp("maxDifferense"), 1000);
	int maxProcessingCount = conf->getArgPInt("--unet-maxprocessingcount",it.getProp("maxProcessingCount"), 100);

	no_sender = conf->getArgInt("--unet-nosender",it.getProp("nosender"));

	xmlNode* nodes = conf->getXMLNodesSection();
	if( !nodes )
	  throw UniSetTypes::SystemError("(UNetExchange): Not found <nodes>");

	UniXML_iterator n_it(nodes);
	
	string default_ip(n_it.getProp("unet_ip"));

	if( !n_it.goChildren() )
		throw UniSetTypes::SystemError("(UNetExchange): Items not found for <nodes>");

	for( ; n_it.getCurrent(); n_it.goNext() )
	{
		// Если указано поле unet_ip непосредственно у узла - берём его
		// если указано общий broadcast ip для всех узлов - берём его
		// Иначе берём из поля "ip"
		string h(n_it.getProp("ip"));
		if( !default_ip.empty() )
			h = default_ip;
		if( !n_it.getProp("unet_ip").empty() )
			h = n_it.getProp("unet_ip");

		// Если указано поле unet_port - используем его
		// Иначе port = идентификатору узла
		int p = n_it.getIntProp("id");
		if( !n_it.getProp("unet_port").empty() )
			p = n_it.getIntProp("unet_port");

		string n(n_it.getProp("name"));
		if( n == conf->getLocalNodeName() )
		{
			dlog[Debug::INFO] << myname << "(init): init sender.. my node " << n_it.getProp("name") << endl;
			sender = new UNetSender(h,p,shm,s_field,s_fvalue,ic);
			sender->setSendPause(sendpause);
			continue;
		}

		if( n_it.getIntProp("unet_ignore") )
		{
			dlog[Debug::INFO] << myname << "(init): unet_ignore.. for " << n_it.getProp("name") << endl;
			continue;
		}

		dlog[Debug::INFO] << myname << "(init): add UNetReceiver for " << h << ":" << p << endl;

		if( checkExistUNetHost(h,p) )
		{
			dlog[Debug::INFO] << myname << "(init): " << h << ":" << p << " already added! Ignore.." << endl;
			continue;
		}
		
		UNetReceiver* r = new UNetReceiver(h,p,shm);

		r->setReceiveTimeout(recvTimeout);
		r->setLostTimeout(lostTimeout);
		r->setReceivePause(recvpause);
		r->setUpdatePause(updatepause);
		r->setMaxDifferens(maxDiff);
		r->setMaxProcessingCount(maxProcessingCount);
		recvlist.push_back(r);
	}
	
	// -------------------------------
	// ********** HEARTBEAT *************
	string heart = conf->getArgParam("--unet-heartbeat-id",it.getProp("heartbeat_id"));
	if( !heart.empty() )
	{
		sidHeartBeat = conf->getSensorID(heart);
		if( sidHeartBeat == DefaultObjectId )
		{
			ostringstream err;
			err << myname << ": не найден идентификатор для датчика 'HeartBeat' " << heart;
			dlog[Debug::CRIT] << myname << "(init): " << err.str() << endl;
			throw SystemError(err.str());
		}

		int heartbeatTime = getHeartBeatTime();
		if( heartbeatTime )
			ptHeartBeat.setTiming(heartbeatTime);
		else
			ptHeartBeat.setTiming(UniSetTimer::WaitUpTime);

		maxHeartBeat = conf->getArgPInt("--unet-heartbeat-max", it.getProp("heartbeat_max"), 10);
		test_id = sidHeartBeat;
	}
	else
	{
		test_id = conf->getSensorID("TestMode_S");
		if( test_id == DefaultObjectId )
		{
			ostringstream err;
			err << myname << "(init): test_id unknown. 'TestMode_S' not found...";
			dlog[Debug::CRIT] << myname << "(init): " << err.str() << endl;
			throw SystemError(err.str());
		}
	}

	dlog[Debug::INFO] << myname << "(init): test_id=" << test_id << endl;

	activateTimeout	= conf->getArgPInt("--activate-timeout", 20000);

	timeout_t msec = conf->getArgPInt("--unet-timeout",it.getProp("timeout"), 3000);

	dlog[Debug::INFO] << myname << "(init): udp-timeout=" << msec << " msec" << endl;
}
// -----------------------------------------------------------------------------
UNetExchange::~UNetExchange()
{
	for( ReceiverList::iterator it=recvlist.begin(); it!=recvlist.end(); ++it )
		delete (*it);

	delete sender;
	delete shm;
}
// -----------------------------------------------------------------------------
bool UNetExchange::checkExistUNetHost( const std::string addr, ost::tpport_t port )
{
	ost::IPV4Address a1(addr.c_str());
	for( ReceiverList::iterator it=recvlist.begin(); it!=recvlist.end(); ++it )
	{
		if( (*it)->getAddress() == a1.getAddress() && (*it)->getPort() == port )
			return true;
	}

	return false;
}
// -----------------------------------------------------------------------------
void UNetExchange::startReceivers()
{
	 for( ReceiverList::iterator it=recvlist.begin(); it!=recvlist.end(); ++it )
		  (*it)->start();
}
// -----------------------------------------------------------------------------
void UNetExchange::initSender(  const std::string s_host, const ost::tpport_t port, UniXML_iterator& it )
{
	if( no_sender )
		return;
}
// -----------------------------------------------------------------------------
void UNetExchange::waitSMReady()
{
	// waiting for SM is ready...
	int ready_timeout = conf->getArgInt("--unet-sm-ready-timeout","15000");
	if( ready_timeout == 0 )
		ready_timeout = 15000;
	else if( ready_timeout < 0 )
		ready_timeout = UniSetTimer::WaitUpTime;

	if( !shm->waitSMready(ready_timeout,50) )
	{
		ostringstream err;
		err << myname << "(waitSMReady): Не дождались готовности SharedMemory к работе в течение " << ready_timeout << " мсек";
		dlog[Debug::CRIT] << err.str() << endl;
		throw SystemError(err.str());
	}
}
// -----------------------------------------------------------------------------
void UNetExchange::timerInfo( TimerMessage *tm )
{
	if( !activated )
		return;

	if( tm->id == tmStep )
		step();
}
// -----------------------------------------------------------------------------
void UNetExchange::step()
{
	if( !activated )
		return;

	if( sidHeartBeat!=DefaultObjectId && ptHeartBeat.checkTime() )
	{
		try
		{
			shm->localSaveValue(aitHeartBeat,sidHeartBeat,maxHeartBeat,getId());
			ptHeartBeat.reset();
		}
		catch(Exception& ex)
		{
			dlog[Debug::CRIT] << myname << "(step): (hb) " << ex << std::endl;
		}
	}
}

// -----------------------------------------------------------------------------
void UNetExchange::processingMessage(UniSetTypes::VoidMessage *msg)
{
	try
	{
		switch(msg->type)
		{
			case UniSetTypes::Message::SysCommand:
			{
				UniSetTypes::SystemMessage sm( msg );
				sysCommand( &sm );
			}
			break;

			case Message::SensorInfo:
			{
				SensorMessage sm( msg );
				sensorInfo(&sm);
			}
			break;

			case Message::Timer:
			{
				TimerMessage tm(msg);
				timerInfo(&tm);
			}
			break;

			default:
				break;
		}
	}
	catch( SystemError& ex )
	{
		dlog[Debug::CRIT] << myname << "(SystemError): " << ex << std::endl;
//		throw SystemError(ex);
		raise(SIGTERM);
	}
	catch( Exception& ex )
	{
		dlog[Debug::CRIT] << myname << "(processingMessage): " << ex << std::endl;
	}
	catch(...)
	{
		dlog[Debug::CRIT] << myname << "(processingMessage): catch ...\n";
	}
}
// -----------------------------------------------------------------------------
void UNetExchange::sysCommand( UniSetTypes::SystemMessage *sm )
{
	switch( sm->command )
	{
		case SystemMessage::StartUp:
		{
			waitSMReady();

			// подождать пока пройдёт инициализация датчиков
			// см. activateObject()
			msleep(initPause);
			PassiveTimer ptAct(activateTimeout);
			while( !activated && !ptAct.checkTime() )
			{	
				cout << myname << "(sysCommand): wait activate..." << endl;
				msleep(300);
				if( activated )
					break;
			}
			
			if( !activated )
				dlog[Debug::CRIT] << myname << "(sysCommand): ************* don`t activate?! ************" << endl;
			
			{
				UniSetTypes::uniset_mutex_lock l(mutex_start, 10000);
				askSensors(UniversalIO::UIONotify);
			}
			askTimer(tmStep,steptime);
			startReceivers();
			if( sender )
				sender->start();
		}

		case SystemMessage::FoldUp:
		case SystemMessage::Finish:
			askSensors(UniversalIO::UIODontNotify);
			break;
		
		case SystemMessage::WatchDog:
		{
			// ОПТИМИЗАЦИЯ (защита от двойного перезаказа при старте)
			// Если идёт локальная работа 
			// (т.е. UNetExchange  запущен в одном процессе с SharedMemory2)
			// то обрабатывать WatchDog не надо, т.к. мы и так ждём готовности SM
			// при заказе датчиков, а если SM вылетит, то вместе с этим процессом(UNetExchange)
			if( shm->isLocalwork() )
				break;

			askSensors(UniversalIO::UIONotify);
		}
		break;

		case SystemMessage::LogRotate:
		{
			// переоткрываем логи
			unideb << myname << "(sysCommand): logRotate" << std::endl;
			string fname = unideb.getLogFile();
			if( !fname.empty() )
			{
				unideb.logFile(fname);
				unideb << myname << "(sysCommand): ***************** UNIDEB LOG ROTATE *****************" << std::endl;
			}

			dlog << myname << "(sysCommand): logRotate" << std::endl;
			fname = dlog.getLogFile();
			if( !fname.empty() )
			{
				dlog.logFile(fname);
				dlog << myname << "(sysCommand): ***************** dlog LOG ROTATE *****************" << std::endl;
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
	if( !shm->waitSMworking(test_id,activateTimeout,50) )
	{
		ostringstream err;
		err << myname 
			<< "(askSensors): Не дождались готовности(work) SharedMemory к работе в течение " 
			<< activateTimeout << " мсек";
	
		dlog[Debug::CRIT] << err.str() << endl;
		kill(SIGTERM,getpid());	// прерываем (перезапускаем) процесс...
		throw SystemError(err.str());
	}
}
// ------------------------------------------------------------------------------------------
void UNetExchange::sensorInfo( UniSetTypes::SensorMessage* sm )
{
	if( sender )
		sender->update(sm->id,sm->value);
}
// ------------------------------------------------------------------------------------------
bool UNetExchange::activateObject()
{
	// блокирование обработки Starsp 
	// пока не пройдёт инициализация датчиков
	// см. sysCommand()
	{
		activated = false;
		UniSetTypes::uniset_mutex_lock l(mutex_start, 5000);
		UniSetObject_LT::activateObject();
		initIterators();
		activated = true;
	}

	return true;
}
// ------------------------------------------------------------------------------------------
void UNetExchange::sigterm( int signo )
{
	cerr << myname << ": ********* SIGTERM(" << signo <<") ********" << endl;
	activated = false;
	UniSetObject_LT::sigterm(signo);
}
// ------------------------------------------------------------------------------------------
void UNetExchange::initIterators()
{
	shm->initAIterator(aitHeartBeat);
}
// -----------------------------------------------------------------------------
void UNetExchange::help_print( int argc, const char* argv[] )
{
	cout << "--unet-name NameID            - Идентификтора процесса." << endl;
	cout << "--unet-recv-timeout msec      - Время для фиксации события 'отсутсвие связи'" << endl;
	cout << "--unet-lost-timeout msec      - Время ожидания заполнения 'дырки' между пакетами. По умолчанию 5000 мсек." << endl;
	cout << "--unet-recvpause msec         - Пауза между приёмами. По умолчанию 10" << endl;
	cout << "--unet-sendpause msec         - Пауза между посылками. По умолчанию 150" << endl;
	cout << "--unet-updatepause msec       - Время ожидания готовности SM к работе, мсек. (-1 - ждать 'вечно')" << endl;
	cout << "--unet-steptime msec		   - Шаг..." << endl;
	cout << "--unet-maxdifferense num      - Маскимальная разница в номерах пакетов для фиксации события 'потеря пакетов' " << endl;
	cout << "--unet-maxprocessingcount num - время на ожидание старта SM" << endl;
	cout << "--unet-nosender [0,1]         - Отключить посылку." << endl;
	cout << "--unet-sm-ready-timeout msec  - Время ожидание я готовности SM к работе. По умолчанию 15000" << endl;
	cout << "--unet-filter-field name      - Название фильтрующего поля при формировании списка датчиков посылаемых данным узлом" << endl;
	cout << "--unet-filter-value name      - Значение фильтрующего поля при формировании списка датчиков посылаемых данным узлом" << endl;
	
}
// -----------------------------------------------------------------------------
UNetExchange* UNetExchange::init_unetexchange( int argc, const char* argv[], UniSetTypes::ObjectId icID, SharedMemory* ic )
{
	string name = conf->getArgParam("--unet-name","UNetExchange1");
	if( name.empty() )
	{
		cerr << "(unetexchange): Не задан name'" << endl;
		return 0;
	}

	ObjectId ID = conf->getObjectID(name);
	if( ID == UniSetTypes::DefaultObjectId )
	{
		cerr << "(unetexchange): идентификатор '" << name
			<< "' не найден в конф. файле!"
			<< " в секции " << conf->getObjectsSection() << endl;
		return 0;
	}

	dlog[Debug::INFO] << "(unetexchange): name = " << name << "(" << ID << ")" << endl;
	return new UNetExchange(ID,icID,ic);
}
// -----------------------------------------------------------------------------
