#include <sstream>
#include "Exceptions.h"
#include "Extensions.h"
#include "UNetSender.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// -----------------------------------------------------------------------------
UNetSender::UNetSender( UniSetTypes::ObjectId objId, UniSetTypes::ObjectId shmId, SharedMemory* ic ):
UniSetObject_LT(objId),
shm(0),
initPause(0),
tcp(0),
activated(false),
dlist(100),
maxItem(0)
{
	if( objId == DefaultObjectId )
		throw UniSetTypes::SystemError("(UNetSender): objId=-1?!! Use --unet-name" );

//	xmlNode* cnode = conf->getNode(myname);
	cnode = conf->getNode(myname);
	if( cnode == NULL )
		throw UniSetTypes::SystemError("(UNetSender): Not find conf-node for " + myname );

	shm = new SMInterface(shmId,&ui,objId,ic);

	UniXML_iterator it(cnode);

	// определяем фильтр
	s_field = conf->getArgParam("--unet-filter-field");
	s_fvalue = conf->getArgParam("--unet-filter-value");
	dlog[Debug::INFO] << myname << "(init): read fileter-field='" << s_field
						<< "' filter-value='" << s_fvalue << "'" << endl;

	// ---------- init RS ----------
//	UniXML_iterator it(cnode);
	string s_host	= conf->getArgParam("--unet-host",it.getProp("host"));
	if( s_host.empty() )
		throw UniSetTypes::SystemError(myname+"(UNetSender): Unknown host. Use --unet-host" );

	port = conf->getArgInt("--unet-port",it.getProp("port"));
	if( port <= 0 )
		throw UniSetTypes::SystemError(myname+"(UNetSender): Unknown port address. Use --unet-port" );

	bool no_broadcast = conf->getArgInt("--unet-nobroadcast",it.getProp("no_broadcast"));

	host = s_host.c_str();

	if( dlog.debugging(Debug::INFO) )
		dlog[Debug::INFO] << "(UNetSender): UNet set to " << s_host << ":" << port
			<< " broadcast=" << broadcast
			<< endl;

	try
	{
		if( no_broadcast )
			tcp = new ost::TCPSocket();
		else
			tcp = new ost::TCPBroadcast(host,port);
	}
	catch( ost::SockException& e )
	{
		ostringstream s;
		s << e.getString() << ": " << e.getSystemErrorString();
		dlog[Debug::CRIT] << myname << "(init): " << s.str() << endl;
		throw SystemError(s.str());
	}

	thr = new ThreadCreator<UNetSender>(this, &UNetSender::poll);

	sendTimeout = conf->getArgPInt("--unet-send-timeout",it.getProp("sendTimeout"), 5000);
	sendtime = conf->getArgPInt("--unet-sendtime",it.getProp("sendtime"), 100);

	// -------------------------------
	if( shm->isLocalwork() )
	{
		readConfiguration();
		dlist.resize(maxItem);
		dlog[Debug::INFO] << myname << "(init): dlist size = " << dlist.size() << endl;
	}
	else
		ic->addReadItem( sigc::mem_fun(this,&UNetSender::readItem) );

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
UNetSender::~UNetSender()
{
	delete tcp;
	delete shm;
	delete thr;
}
// -----------------------------------------------------------------------------
void UNetSender::waitSMReady()
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
void UNetSender::step()
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
			dlog[Debug::CRIT] << myname
				<< "(step): (hb) " << ex << std::endl;
		}
	}
}
// -----------------------------------------------------------------------------
void UNetSender::poll()
{
	dlist.resize(maxItem);
	dlog[Debug::INFO] << myname << "(init): dlist size = " << dlist.size() << endl;

	ost::Thread::setException(ost::Thread::throwException);
//	cerr << "create new tcp..." << endl;
	tcp = new ost::TCPStream(iaddr.c_str());
	tcp->setTimeout(500);

	try
	{
		tcp->setPeer(host,port);
	}
	catch( ost::SockException& e )
	{
		ostringstream s;
		s << e.getString() << ": " << e.getSystemErrorString();
		dlog[Debug::CRIT] << myname << "(poll): " << s.str() << endl;
		throw SystemError(s.str());
	}


	while( activated )
	{
		try
		{
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

		msleep(sendtime);
	}

	cerr << "************* execute FINISH **********" << endl;
}
// -----------------------------------------------------------------------------
void UNetSender::send()
{
	cout << myname << ": send..." << endl;

	UniSetUNet::UNetHeader h;
	h.nodeID = conf->getLocalNode();
	h.procID = getId();
	h.dcount = mypack.size();
	// receive
		ssize_t ret = tcp->send((char*)(&h),sizeof(h));
		if( ret<(ssize_t)sizeof(h) )
		{
			cerr << myname << "(send data header): ret=" << ret << " sizeof=" << sizeof(h) << endl;
			return;
		}

#warning use mutex for list!!!
		UniSetUNet::UNetMessage::UNetDataList::iterator it = mypack.dlist.begin();
		for( ; it!=mypack.dlist.end(); ++it )
		{
			cout << myname << "(send): " << (*it) << endl;
			ssize_t ret = tcp->send((char*)(&(*it)),sizeof(*it));
			if( ret<(ssize_t)sizeof(*it) )
			{
				cerr << myname << "(send data): ret=" << ret << " sizeof=" << sizeof(*it) << endl;
				break;
			}
		}
}
// -----------------------------------------------------------------------------
void UNetSender::processingMessage(UniSetTypes::VoidMessage *msg)
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
void UNetSender::sysCommand(UniSetTypes::SystemMessage *sm)
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
			// (т.е. UNetSender  запущен в одном процессе с SharedMemory2)
			// то обрабатывать WatchDog не надо, т.к. мы и так ждём готовности SM
			// при заказе датчиков, а если SM вылетит, то вместе с этим процессом(UNetSender)
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
void UNetSender::askSensors( UniversalIO::UIOCommand cmd )
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
void UNetSender::sensorInfo( UniSetTypes::SensorMessage* sm )
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
bool UNetSender::activateObject()
{
	// блокирование обработки StarUp
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
void UNetSender::sigterm( int signo )
{
	cerr << myname << ": ********* SIGTERM(" << signo <<") ********" << endl;
	activated = false;
	tcp->disconnect();
	UniSetObject_LT::sigterm(signo);
}
// ------------------------------------------------------------------------------------------
void UNetSender::readConfiguration()
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
bool UNetSender::check_item( UniXML_iterator& it )
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
bool UNetSender::readItem( UniXML& xml, UniXML_iterator& it, xmlNode* sec )
{
	if( check_item(it) )
		initItem(it);
	return true;
}
// ------------------------------------------------------------------------------------------
bool UNetSender::initItem( UniXML_iterator& it )
{
	string sname( it.getProp("name") );

	string tid = it.getProp("id");

	ObjectId sid;
	if( !tid.empty() )
	{
		sid = UniSetTypes::uni_atoi(tid);
		if( sid <= 0 )
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
void UNetSender::initIterators()
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
void UNetSender::help_print( int argc, char* argv[] )
{
	cout << "--unet-sendtime msec    - Пауза между опросами. По умолчанию 200 мсек." << endl;
	cout << "--unet-heartbeat-id     - Данный процесс связан с указанным аналоговым heartbeat-дачиком." << endl;
	cout << "--unet-heartbeat-max    - Максимальное значение heartbeat-счётчика для данного процесса. По умолчанию 10." << endl;
	cout << "--unet-ready-timeout    - Время ожидания готовности SM к работе, мсек. (-1 - ждать 'вечно')" << endl;
	cout << "--unet-initPause		- Задержка перед инициализацией (время на активизация процесса)" << endl;
	cout << "--unet-sm-ready-timeout - время на ожидание старта SM" << endl;
	cout << " Настройки протокола UNet: " << endl;
	cout << "--unet-host [ip|hostname]  - Адрес сервера" << endl;
	cout << "--unet-port         - Порт." << endl;
	cout << "--unet-send-timeout - Таймаут на посылку ответа." << endl;
}
// -----------------------------------------------------------------------------
UNetSender* UNetSender::init_udpsender( int argc, char* argv[], UniSetTypes::ObjectId icID, SharedMemory* ic )
{
	string name = conf->getArgParam("--unet-name","UNetSender1");
	if( name.empty() )
	{
		cerr << "(UNetSender): Не задан name'" << endl;
		return 0;
	}

	ObjectId ID = conf->getObjectID(name);
	if( ID == UniSetTypes::DefaultObjectId )
	{
		cerr << "(UNetSender): идентификатор '" << name
			<< "' не найден в конф. файле!"
			<< " в секции " << conf->getObjectsSection() << endl;
		return 0;
	}

	dlog[Debug::INFO] << "(rsexchange): name = " << name << "(" << ID << ")" << endl;
	return new UNetSender(ID,icID,ic);
}
// -----------------------------------------------------------------------------
std::ostream& operator<<( std::ostream& os, UNetSender::UItem& p )
{
	return os 	<< " sid=" << p.si.id;
}
// -----------------------------------------------------------------------------
