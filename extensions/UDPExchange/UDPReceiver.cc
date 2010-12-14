#include <sstream>
#include "Exceptions.h"
#include "Extensions.h"
#include "UDPReceiver.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// -----------------------------------------------------------------------------
bool UDPReceiver::PacketCompare::operator()(const UniSetUDP::UDPMessage& lhs,
											const UniSetUDP::UDPMessage& rhs) const
{
//	if( lhs.msg.header.num == rhs.msg.header.num )
//		return (lhs.msg < rhs.msg);

	return lhs.msg.header.num > rhs.msg.header.num;
}
// ------------------------------------------------------------------------------------------
UDPReceiver::UDPReceiver( UniSetTypes::ObjectId objId, UniSetTypes::ObjectId shmId, SharedMemory* ic ):
UniSetObject_LT(objId),
shm(0),
initPause(0),
udp(0),
activated(false)
{
	if( objId == DefaultObjectId )
		throw UniSetTypes::SystemError("(UDPReceiver): objId=-1?!! Use --udp-name" );

//	xmlNode* cnode = conf->getNode(myname);
	cnode = conf->getNode(myname);
	if( cnode == NULL )
		throw UniSetTypes::SystemError("(UDPReceiver): Not find conf-node for " + myname );

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
		throw UniSetTypes::SystemError(myname+"(UDPReceiver): Unknown host. Use --udp-host" );

	port = conf->getArgInt("--udp-port",it.getProp("port"));
	if( port <= 0 )
		throw UniSetTypes::SystemError(myname+"(UDPReceiver): Unknown port address. Use --udp-port" );

	if( dlog.debugging(Debug::INFO) )
		dlog[Debug::INFO] << "(UDPReceiver): UDP set to " << s_host << ":" << port << endl;
	
	host = s_host.c_str();
	try
	{
		udp = new ost::UDPDuplex(host,port);
	}
	catch( ost::SockException& e )
	{
		ostringstream s;
		s << e.getString() << ": " << e.getSystemErrorString() << endl;
		throw SystemError(s.str());
	}

	thr = new ThreadCreator<UDPReceiver>(this, &UDPReceiver::poll);

	recvTimeout = conf->getArgPInt("--udp-recv-timeout",it.getProp("recvTimeout"), 5000);
	polltime = conf->getArgPInt("--udp-polltime",it.getProp("polltime"), 10);
	updatetime = conf->getArgPInt("--udp-updatetime",it.getProp("updatetime"), 100);
	steptime = conf->getArgPInt("--udp-steptime",it.getProp("steptime"), 100);
	minBufSize = conf->getArgPInt("--udp-minbufsize",it.getProp("minBufSize"), 30);
	maxProcessingCount = conf->getArgPInt("--udp-maxprocessingcount",it.getProp("maxProcessingCount"), 100);

	// -------------------------------
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

		maxHeartBeat = conf->getArgPInt("--udp-heartbeat-max", it.getProp("heartbeat_max"), 10);
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

	timeout_t msec = conf->getArgPInt("--udp-timeout",it.getProp("timeout"), 3000);

	dlog[Debug::INFO] << myname << "(init): udp-timeout=" << msec << " msec" << endl;
}
// -----------------------------------------------------------------------------
UDPReceiver::~UDPReceiver()
{
	delete udp;
	delete shm;
	delete thr;
}
// -----------------------------------------------------------------------------
void UDPReceiver::waitSMReady()
{
	// waiting for SM is ready...
	int ready_timeout = conf->getArgInt("--udp-sm-ready-timeout","15000");
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
void UDPReceiver::timerInfo( TimerMessage *tm )
{
	if( !activated )
		return;

	if( tm->id == tmStep )
		step();
	else if( tm->id == tmUpdate )
		update();
}
// -----------------------------------------------------------------------------
void UDPReceiver::step()
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
void UDPReceiver::update()
{
	UniSetUDP::UDPMessage p;
	bool buf_ok = false;
	{
		uniset_mutex_lock l(packMutex);
		if( qpack.size() <= minBufSize )
			return;

		buf_ok = true;
	}

	int k = maxProcessingCount;
	while( buf_ok && k>0)
	{
		{
			uniset_mutex_lock l(packMutex);
			p = qpack.top();
			qpack.pop();
		}

		if( labs(p.msg.header.num - pnum) > 1 )
		{
			dlog[Debug::CRIT] << "************ FAILED! ORDER PACKETS! recv.num=" << pack.msg.header.num
				  << " num=" << pnum << endl;
		}

		pnum = p.msg.header.num;
		k--;

		{
			uniset_mutex_lock l(packMutex);
			buf_ok =  ( qpack.size() > minBufSize );
		}

		// cerr << myname << "(step): recv DATA OK. header: " << p.msg.header << endl;
		for( int i=0; i<p.msg.header.dcount; i++ )
		{
			try
			{
				UniSetUDP::UDPData& d = p.msg.dat[i];
				shm->setValue(d.id,d.val);
			}
			catch( UniSetTypes::Exception& ex)
			{
				dlog[Debug::CRIT] << myname << "(update): " << ex << std::endl;
			}
			catch(...)
			{
				dlog[Debug::CRIT] << myname << "(update): catch ..." << std::endl;
			}
		}
	}
}

// -----------------------------------------------------------------------------

void UDPReceiver::poll()
{
	cerr << "******************* pool start" << endl;
	while( activated )
	{
		try
		{
			recv();
		}
		catch( ost::SockException& e )
		{
			cerr  << e.getString() << ": " << e.getSystemErrorString() << endl;
		}
		catch( UniSetTypes::Exception& ex)
		{
			cerr << myname << "(poll): " << ex << std::endl;
		}
		catch(...)
		{
			cerr << myname << "(poll): catch ..." << std::endl;
		}	

		msleep(polltime);
	}

	cerr << "************* execute FINISH **********" << endl;
}
// -----------------------------------------------------------------------------
void UDPReceiver::recv()
{
	if( udp->isInputReady(recvTimeout) )
	{
		ssize_t ret = udp->UDPReceive::receive(&(pack.msg),sizeof(pack.msg));
		if( ret < sizeof(UniSetUDP::UDPHeader) )
		{
			cerr << myname << "(receive): FAILED header ret=" << ret << " sizeof=" << sizeof(UniSetUDP::UDPHeader) << endl;
			return;
		}

		ssize_t sz = pack.msg.header.dcount * sizeof(UniSetUDP::UDPData) + sizeof(UniSetUDP::UDPHeader);
		if( ret < sz )
		{
			cerr << myname << "(receive): FAILED data ret=" << ret << " sizeof=" << sz << endl;
			return;
		}


//		cerr << myname << "(receive): recv DATA OK. ret=" << ret << " sizeof=" << sz
//			  << " header: " << pack.msg.header << endl;
/*
		if( labs(pack.msg.header.num - pnum) > 1 )
		{
			cerr << "************ FAILED! ORDER PACKETS! recv.num=" << pack.msg.header.num
					<< " num=" << pnum << endl;
		}

		pnum = pack.msg.header.num;
*/
		{
			uniset_mutex_lock l(packMutex);
//			qpack[pack.msg.header.num] = pack;
			qpack.push(pack);
		}

		return;
	}
}
// -----------------------------------------------------------------------------
void UDPReceiver::processingMessage(UniSetTypes::VoidMessage *msg)
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
void UDPReceiver::sysCommand( UniSetTypes::SystemMessage *sm )
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
			askTimer(tmUpdate,updatetime);
			askTimer(tmStep,steptime);
		}

		case SystemMessage::FoldUp:
		case SystemMessage::Finish:
			askSensors(UniversalIO::UIODontNotify);
			break;
		
		case SystemMessage::WatchDog:
		{
			// ОПТИМИЗАЦИЯ (защита от двойного перезаказа при старте)
			// Если идёт локальная работа 
			// (т.е. UDPReceiver  запущен в одном процессе с SharedMemory2)
			// то обрабатывать WatchDog не надо, т.к. мы и так ждём готовности SM
			// при заказе датчиков, а если SM вылетит, то вместе с этим процессом(UDPReceiver)
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
void UDPReceiver::askSensors( UniversalIO::UIOCommand cmd )
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
void UDPReceiver::sensorInfo( UniSetTypes::SensorMessage* sm )
{
}
// ------------------------------------------------------------------------------------------
bool UDPReceiver::activateObject()
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
void UDPReceiver::sigterm( int signo )
{
	cerr << myname << ": ********* SIGTERM(" << signo <<") ********" << endl;
	activated = false;
	udp->disconnect();
	UniSetObject_LT::sigterm(signo);
}
// ------------------------------------------------------------------------------------------
void UDPReceiver::initIterators()
{
	shm->initAIterator(aitHeartBeat);
}
// -----------------------------------------------------------------------------
void UDPReceiver::help_print( int argc, char* argv[] )
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
UDPReceiver* UDPReceiver::init_udpreceiver( int argc, char* argv[], UniSetTypes::ObjectId icID, SharedMemory* ic )
{
	string name = conf->getArgParam("--udp-name","UDPReceiver1");
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
	return new UDPReceiver(ID,icID,ic);
}
// -----------------------------------------------------------------------------
