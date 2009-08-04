// $Id: UDPExchange.cc,v 1.1 2009/02/10 20:38:27 vpashka Exp $
// -----------------------------------------------------------------------------
#include <sstream>
#include "Exceptions.h"
#include "Extensions.h"
#include "UDPExchange.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// -----------------------------------------------------------------------------
UDPExchange::UDPExchange( UniSetTypes::ObjectId objId, UniSetTypes::ObjectId shmId, SharedMemory* ic ):
UniSetObject_LT(objId),
udp(0),
shm(0),
initPause(0),
activated(false),
dlist(100),
maxItem(0)
{
	cout << "$Id: UDPExchange.cc,v 1.1 2009/02/10 20:38:27 vpashka Exp $" << endl;

	if( objId == DefaultObjectId )
		throw UniSetTypes::SystemError("(UDPExchange): objId=-1?!! Use --udp-name" );

//	xmlNode* cnode = conf->getNode(myname);
	cnode = conf->getNode(myname);
	if( cnode == NULL )
		throw UniSetTypes::SystemError("(UDPExchange): Not find conf-node for " + myname );

	shm = new SMInterface(shmId,&ui,objId,ic);

	UniXML_iterator it(cnode);

	// определяем фильтр
	s_field = conf->getArgParam("--udp-filter-field");
	s_fvalue = conf->getArgParam("--udp-filter-value");
	dlog[Debug::INFO] << myname << "(init): read fileter-field='" << s_field
						<< "' filter-value='" << s_fvalue << "'" << endl;

	// ---------- init RS ----------
//	UniXML_iterator it(cnode);
	string s_host	= conf->getArgParam("--udp-host",it.getProp("host"));
	if( s_host.empty() )
		throw UniSetTypes::SystemError(myname+"(UDPExchange): Unknown host. Use --udp-host" );

	string s_port 	= conf->getArgParam("--udp-port",it.getProp("port"));
	if( s_port.empty() )
		throw UniSetTypes::SystemError(myname+"(UDPExchange): Unknown port address. Use --udp-port" );

	port = atoi(s_port.c_str());

	if( dlog.debugging(Debug::INFO) )
		dlog[Debug::INFO] << "(UDPExchange): UDP set to " << s_host << ":" << port << endl;
	
	host = s_host.c_str();
	try
	{
		udp = new ost::UDPDuplex(host,port);
//		udp->UDPTransmit::setBroadcast(false);
//		udp->UDPTransmit::setRouting(false);
	}
	catch( ost::SockException& e )
	{
		ostringstream s;
		s << e.getString() << ": " << e.getSystemErrorString() << endl;
		throw SystemError(s.str());
	}

	thr = new ThreadCreator<UDPExchange>(this, &UDPExchange::poll);

	recvTimeout = atoi(conf->getArgParam("--udp-recv-timeout",it.getProp("recvTimeout")).c_str());
	if( recvTimeout <=0 )
		recvTimeout = 5000;
		
	sendTimeout = atoi(conf->getArgParam("--udp-send-timeout",it.getProp("sendTimeout")).c_str());
	if( sendTimeout <=0 )
		sendTimeout = 5000;

	polltime = atoi(conf->getArgParam("--udp-polltime",it.getProp("polltime")).c_str());
	if( !polltime )
		polltime = 100;


	// -------------------------------
	if( shm->isLocalwork() )
	{
		readConfiguration();
		dlist.resize(maxItem);
		dlog[Debug::INFO] << myname << "(init): dlist size = " << dlist.size() << endl;
	}
	else
		ic->addReadItem( sigc::mem_fun(this,&UDPExchange::readItem) );

	// ********** HEARTBEAT *************
	string heart = conf->getArgParam("--udp-heartbeat-id",it.getProp("heartbeat_id"));
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

		maxHeartBeat = atoi(conf->getArgParam("--udp-heartbeat-max",it.getProp("heartbeat_max")).c_str());
		if( maxHeartBeat <=0 )
			maxHeartBeat = 10;
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

	activateTimeout	= atoi(conf->getArgParam("--activate-timeout").c_str());
	if( activateTimeout<=0 )
		activateTimeout = 20000;

	int msec = atoi(conf->getArgParam("--udp-timeout",it.getProp("timeout")).c_str());
	if( msec <=0 )
		msec = 3000;

	dlog[Debug::INFO] << myname << "(init): udp-timeout=" << msec << " msec" << endl;
}
// -----------------------------------------------------------------------------
UDPExchange::~UDPExchange()
{
	delete udp;
	delete shm;
	delete thr;
}
// -----------------------------------------------------------------------------
void UDPExchange::waitSMReady()
{
	// waiting for SM is ready...
	int ready_timeout = atoi(conf->getArgParam("--udp-sm-ready-timeout","15000").c_str());
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
/*
void UDPExchange::timerInfo( TimerMessage *tm )
{
	if( tm->id == tmExchange )
		step();
}
*/
// -----------------------------------------------------------------------------
void UDPExchange::step()
{
//	{
//		uniset_mutex_lock l(pollMutex,2000);
//		poll();
//	}

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
			dlog[Debug::CRIT] << myname
				<< "(step): (hb) " << ex << std::endl;
		}
	}
}

// -----------------------------------------------------------------------------
void UDPExchange::poll()
{
	dlist.resize(maxItem);
	dlog[Debug::INFO] << myname << "(init): dlist size = " << dlist.size() << endl;

	try
	{
		udp->connect(host,port);
	}
	catch( UniSetTypes::Exception& ex)
	{
		cerr << myname << "(step): " << ex << std::endl;
//		reise(SIGTERM);
		return;
	}



	while( activated )
	{
		try
		{
			recv();
			send();
		}
		catch( ost::SockException& e )
		{
			cerr  << e.getString() << ": " << e.getSystemErrorString() << endl;
		}
		catch( UniSetTypes::Exception& ex)
		{
			cerr << myname << "(step): " << ex << std::endl;
		}
		catch(...)
		{
			cerr << myname << "(step): catch ..." << std::endl;
		}	

		msleep(polltime);
	}

	cerr << "************* execute FINISH **********" << endl;
}
// -----------------------------------------------------------------------------
void UDPExchange::recv()
{	
	cout << myname << ": recv...." << endl;
	UniSetUDP::UDPHeader h;
	// receive
	if( udp->isInputReady(recvTimeout) )
	{
		int ret = udp->UDPReceive::receive(&h,sizeof(h));
		if( ret<sizeof(h) )
		{
			cerr << myname << "(receive): ret=" << ret << " sizeof=" << sizeof(h) << endl;
			return;
		}
		
		cout << myname << "(receive): header: " << h << endl;

		UniSetUDP::UDPData d;
		// ignore echo...
#if 0
		if( h.nodeID == conf->getLocalNode() && h.procID == getId() )
		{
			for( int i=0; i<h.dcount;i++ )
			{
				int ret = udp->UDPReceive::receive(&d,sizeof(d));
				if( ret < sizeof(d) )
					return;
			}
			return;
		}
#endif 		
		for( int i=0; i<h.dcount;i++ )
		{
			int ret = udp->UDPReceive::receive(&d,sizeof(d));
			if( ret<sizeof(d) )
			{
				cerr << myname << "(receive data " << i << "): ret=" << ret << " sizeof=" << sizeof(d) << endl;
				break;
			}
			
			cout << myname << "(receive data " << i << "): " << d << endl;
		}
	}
}
// -----------------------------------------------------------------------------
void UDPExchange::send()
{
	cout << myname << ": send..." << endl;

	UniSetUDP::UDPHeader h;
	h.nodeID = conf->getLocalNode();
	h.procID = getId();
	h.dcount = mypack.size();
	// receive
	if( udp->isOutputReady(sendTimeout) )
	{
		int ret = udp->transmit((char*)(&h),sizeof(h));
		if( ret<sizeof(h) )
		{
			cerr << myname << "(send data header): ret=" << ret << " sizeof=" << sizeof(h) << endl;
			return;
		}

#warning use mutex for list!!!
		UniSetUDP::UDPMessage::UDPDataList::iterator it = mypack.dlist.begin();
		for( ; it!=mypack.dlist.end(); ++it )
		{
			cout << myname << "(send): " << (*it) << endl;
			ret = udp->transmit((char*)(&(*it)),sizeof(*it));
			if( ret<sizeof(*it) )
			{
				cerr << myname << "(send data): ret=" << ret << " sizeof=" << sizeof(*it) << endl;
				break;
			}
		}
	}
	
}
// -----------------------------------------------------------------------------
void UDPExchange::processingMessage(UniSetTypes::VoidMessage *msg)
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
void UDPExchange::sysCommand(UniSetTypes::SystemMessage *sm)
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
			thr->start();
		}

		case SystemMessage::FoldUp:
		case SystemMessage::Finish:
			askSensors(UniversalIO::UIODontNotify);
			break;
		
		case SystemMessage::WatchDog:
		{
			// ОПТИМИЗАЦИЯ (защита от двойного перезаказа при старте)
			// Если идёт локальная работа 
			// (т.е. UDPExchange  запущен в одном процессе с SharedMemory2)
			// то обрабатывать WatchDog не надо, т.к. мы и так ждём готовности SM
			// при заказе датчиков, а если SM вылетит, то вместе с этим процессом(UDPExchange)
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
				unideb.logFile(fname.c_str());
				unideb << myname << "(sysCommand): ***************** UNIDEB LOG ROTATE *****************" << std::endl;
			}

			dlog << myname << "(sysCommand): logRotate" << std::endl;
			fname = dlog.getLogFile();
			if( !fname.empty() )
			{
				dlog.logFile(fname.c_str());
				dlog << myname << "(sysCommand): ***************** dlog LOG ROTATE *****************" << std::endl;
			}
		}
		break;

		default:
			break;
	}
}
// ------------------------------------------------------------------------------------------
void UDPExchange::askSensors( UniversalIO::UIOCommand cmd )
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

	DMap::iterator it=dlist.begin();
	for( ; it!=dlist.end(); ++it )
	{
		try
		{
			shm->askSensor(it->si.id,cmd);
		}
		catch( UniSetTypes::Exception& ex )
		{
			dlog[Debug::WARN] << myname << "(askSensors): " << ex << std::endl;
		}
		catch(...){}
	}
}
// ------------------------------------------------------------------------------------------
void UDPExchange::sensorInfo( UniSetTypes::SensorMessage* sm )
{
	DMap::iterator it=dlist.begin();
	for( ; it!=dlist.end(); ++it )
	{
		if( it->si.id == sm->id )
		{
			uniset_spin_lock lock(it->val_lock);
			it->val = sm->value;
			if( it->pack_it != mypack.dlist.end() )
				it->pack_it->val = sm->value;
		}
		break;
	}
}
// ------------------------------------------------------------------------------------------
bool UDPExchange::activateObject()
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
void UDPExchange::sigterm( int signo )
{
	cerr << myname << ": ********* SIGTERM(" << signo <<") ********" << endl;
	activated = false;
	udp->disconnect();
	UniSetObject_LT::sigterm(signo);
}
// ------------------------------------------------------------------------------------------
void UDPExchange::readConfiguration()
{
#warning Сделать сортировку по диапазонам адресов!!!
// чтобы запрашивать одним запросом, сразу несколько входов...
//	readconf_ok = false;
	xmlNode* root = conf->getXMLSensorsSection();
	if(!root)
	{
		ostringstream err;
		err << myname << "(readConfiguration): не нашли корневого раздела <sensors>";
		throw SystemError(err.str());
	}

	UniXML_iterator it(root);
	if( !it.goChildren() )
	{
		std::cerr << myname << "(readConfiguration): раздел <sensors> не содержит секций ?!!\n";
		return;
	}

	for( ;it.getCurrent(); it.goNext() )
	{
		if( check_item(it) )
			initItem(it);
	}
	
//	readconf_ok = true;
}
// ------------------------------------------------------------------------------------------
bool UDPExchange::check_item( UniXML_iterator& it )
{
	if( s_field.empty() )
		return true;

	// просто проверка на не пустой field
	if( s_fvalue.empty() && it.getProp(s_field).empty() )
		return false;

	// просто проверка что field = value
	if( !s_fvalue.empty() && it.getProp(s_field)!=s_fvalue )
		return false;

	return true;
}
// ------------------------------------------------------------------------------------------
bool UDPExchange::readItem( UniXML& xml, UniXML_iterator& it, xmlNode* sec )
{
	if( check_item(it) )
		initItem(it);
	return true;
}
// ------------------------------------------------------------------------------------------
bool UDPExchange::initItem( UniXML_iterator& it )
{
	string sname( it.getProp("name") );

	string tid = it.getProp("id");
	
	ObjectId sid;
	if( !tid.empty() )
	{
		sid = UniSetTypes::uni_atoi(tid);
		if( sid<=0 )
			sid = DefaultObjectId;
	}
	else
		sid = conf->getSensorID(sname);

	if( sid == DefaultObjectId )
	{
		if( dlog )
			dlog[Debug::CRIT] << myname << "(readItem): ID not found for "
							<< sname << endl;
		return false;
	}
	
	UItem p;
	p.si.id = sid;
	p.si.node = conf->getLocalNode();
	mypack.addData(sid,0);
	p.pack_it = (mypack.dlist.end()--);

	if( maxItem >= dlist.size() )
		dlist.resize(maxItem+10);
	
	dlist[maxItem] = p;
	maxItem++;

	if( dlog.debugging(Debug::INFO) )
		dlog[Debug::INFO] << myname << "(initItem): add " << p << endl;

	return true;
}

// ------------------------------------------------------------------------------------------
void UDPExchange::initIterators()
{
	DMap::iterator it=dlist.begin();
	for( ; it!=dlist.end(); it++ )
	{
		shm->initDIterator(it->dit);
		shm->initAIterator(it->ait);
	}

	shm->initAIterator(aitHeartBeat);
}
// -----------------------------------------------------------------------------
void UDPExchange::help_print( int argc, char* argv[] )
{
	cout << "--udp-polltime msec     - Пауза между опросаом карт. По умолчанию 200 мсек." << endl;
	cout << "--udp-heartbeat-id      - Данный процесс связан с указанным аналоговым heartbeat-дачиком." << endl;
	cout << "--udp-heartbeat-max     - Максимальное значение heartbeat-счётчика для данного процесса. По умолчанию 10." << endl;
	cout << "--udp-ready-timeout     - Время ожидания готовности SM к работе, мсек. (-1 - ждать 'вечно')" << endl;    
	cout << "--udp-initPause		- Задержка перед инициализацией (время на активизация процесса)" << endl;
	cout << "--udp-notRespondSensor - датчик связи для данного процесса " << endl;
	cout << "--udp-sm-ready-timeout - время на ожидание старта SM" << endl;
	cout << " Настройки протокола RS: " << endl;
	cout << "--udp-dev devname  - файл устройства" << endl;
	cout << "--udp-speed        - Скорость обмена (9600,19920,38400,57600,115200)." << endl;
	cout << "--udp-my-addr      - адрес текущего узла" << endl;
	cout << "--udp-recv-timeout - Таймаут на ожидание ответа." << endl;
}
// -----------------------------------------------------------------------------
UDPExchange* UDPExchange::init_udpexchange( int argc, char* argv[], UniSetTypes::ObjectId icID, SharedMemory* ic )
{
	string name = conf->getArgParam("--udp-name","UDPExchange1");
	if( name.empty() )
	{
		cerr << "(udpexchange): Не задан name'" << endl;
		return 0;
	}

	ObjectId ID = conf->getObjectID(name);
	if( ID == UniSetTypes::DefaultObjectId )
	{
		cerr << "(udpexchange): идентификатор '" << name 
			<< "' не найден в конф. файле!"
			<< " в секции " << conf->getObjectsSection() << endl;
		return 0;
	}

	dlog[Debug::INFO] << "(rsexchange): name = " << name << "(" << ID << ")" << endl;
	return new UDPExchange(ID,icID,ic);
}
// -----------------------------------------------------------------------------
std::ostream& operator<<( std::ostream& os, UDPExchange::UItem& p )
{
	return os 	<< " sid=" << p.si.id;
}
// -----------------------------------------------------------------------------
