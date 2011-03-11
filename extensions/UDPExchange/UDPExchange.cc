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
shm(0),
initPause(0),
udp(0),
activated(false),
dlist(100),
maxItem(0),
packetnum(1)
{
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
	s_host	= conf->getArgParam("--udp-host",it.getProp("host"));
	if( s_host.empty() )
		throw UniSetTypes::SystemError(myname+"(UDPExchange): Unknown host. Use --udp-host" );

	host = s_host.c_str();

	buildReceiverList();

//	port = conf->getArgInt("--udp-port",it.getProp("port"));
	if( port <= 0 || port == DefaultObjectId )
		throw UniSetTypes::SystemError(myname+"(UDPExchange): Unknown port address" );

	if( dlog.debugging(Debug::INFO) )
		dlog[Debug::INFO] << "(UDPExchange): UDP set to " << s_host << ":" << port << endl;
	
	try
	{
		udp = new ost::UDPBroadcast(host,port);	  
	}
	catch( ost::SockException& e )
	{
		ostringstream s;
		s << e.getString() << ": " << e.getSystemErrorString() << endl;
		throw SystemError(s.str());
	}

	thr = new ThreadCreator<UDPExchange>(this, &UDPExchange::poll);

	recvTimeout = conf->getArgPInt("--udp-recv-timeout",it.getProp("recvTimeout"), 5000);
	sendTimeout = conf->getArgPInt("--udp-send-timeout",it.getProp("sendTimeout"), 5000);
	polltime = conf->getArgPInt("--udp-polltime",it.getProp("polltime"), 100);

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
UDPExchange::~UDPExchange()
{
	for( ReceiverList::iterator it=rlist.begin(); it!=rlist.end(); it++ )
		delete (*it);

	delete udp;
	delete shm;
	delete thr;
}
// -----------------------------------------------------------------------------
void UDPExchange::waitSMReady()
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
void UDPExchange::step()
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
void UDPExchange::poll()
{
	dlist.resize(maxItem);
	dlog[Debug::INFO] << myname << "(init): dlist size = " << dlist.size() << endl;

	for( ReceiverList::iterator it=rlist.begin(); it!=rlist.end(); it++ )
	{
		(*it)->setReceiveTimeout(recvTimeout);		
		if( dlog.debugging(Debug::INFO) )
			dlog[Debug::INFO] << myname << "(poll): start exchange for " << (*it)->getName() << endl;
		(*it)->start();
	}

	ost::IPV4Broadcast h = s_host.c_str();
	try
	{			
		udp->setPeer(h,port);
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

		msleep(polltime);
	}

	cerr << "************* execute FINISH **********" << endl;
}
// -----------------------------------------------------------------------------
void UDPExchange::send()
{
	cout << myname << ": send..." << endl;
	UniSetUDP::UDPHeader h;
	h.nodeID = conf->getLocalNode();
	h.procID = getId();
	h.dcount = mypack.msg.header.dcount;
	h.num = packetnum++;

	mypack.msg.header = h;
/*
	int ind = 0;
	memcpy(udpbuf,(char*)(&h),sizeof(h));
	ind += sizeof(h);
	memcpy( &(udpbuf[ind]),&(mypack.data),mypack.size());
	ind += mypack.size();
*/
	cout << "************* send header: " << mypack.msg.header << endl;
	int sz = mypack.size() * sizeof(UniSetUDP::UDPHeader);
	if( udp->isPending(ost::Socket::pendingOutput) )
	{
//		ssize_t ret = udp->send( (char*)&(mypack.msg),sizeof(mypack.msg));
//		if( ret<sizeof(mypack.msg) )
		ssize_t ret = udp->send( (char*)&(mypack.msg),sz);
		if( ret < sz )
		{
			cerr << myname << "(send data header): ret=" << ret << " sizeof=" << sz << endl;
			return;
		}

		cout << "send OK. byte count=" << ret << endl;
	}

#if 0
/*
	if( udp->isPending(ost::Socket::pendingOutput) )
	{
		size_t ret = udp->send((char*)(&h),sizeof(h));
		if( ret<(size_t)sizeof(h) )
		{
			cerr << myname << "(send data header): ret=" << ret << " sizeof=" << sizeof(h) << endl;
			return;
		}
*/
/*! \todo Подумать нужен ли здесь mutex */
		UniSetUDP::UDPMessage::UDPDataList::iterator it = mypack.dlist.begin();
		
		for( ; it!=mypack.dlist.end(); ++it )
		{
//			while( !udp->isPending(ost::Socket::pendingOutput) )
//				msleep(30);
        	cout << myname << "(send): " << (*it) << endl;
			size_t ret = udp->send((char*)(&(*it)),sizeof(UniSetUDP::UDPData));
			if( ret<(size_t)sizeof(UniSetUDP::UDPData) )
			{
				cerr << myname << "(send data): ret=" << ret << " sizeof=" << sizeof(UniSetUDP::UDPData) << endl;
				break;
			}
		}
//	}
#endif
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
/*
			if( it->pack_ind != -1 )
				mypack.it->pack_it->val = sm->value;
*/
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
	for( ReceiverList::iterator it=rlist.begin(); it!=rlist.end(); it++ )
		(*it)->stop();

	UniSetObject_LT::sigterm(signo);
}
// ------------------------------------------------------------------------------------------
void UDPExchange::readConfiguration()
{
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
	p.pack_ind = mypack.size()-1;

	if( maxItem >= mypack.size() )
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
	cout << " Настройки протокола UDP: " << endl;
	cout << "--udp-host [ip|hostname]  - Адрес сервера" << endl;
	cout << "--udp-send-timeout - Таймаут на посылку ответа." << endl;
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
void UDPExchange::buildReceiverList()
{
	xmlNode* n = conf->getXMLNodesSection();
	if( !n )
	{
		dlog[Debug::WARN] << myname << "(buildReceiverList): <nodes> not found! ignore..." << endl;
		return;
	}
	
	UniXML_iterator it(n);
	if( !it.goChildren() )
	{
		dlog[Debug::WARN] << myname << "(buildReceiverList): <nodes> is empty?! ignore..." << endl;
		return;
	}
	
	for( ; it.getCurrent(); it.goNext() )
	{
		ObjectId n_id = conf->getNodeID( it.getProp("name") );
		if( n_id == conf->getLocalNode() )
		{
			port = it.getIntProp("udp_port");
			if( port<=0 )
				port = n_id;
			dlog[Debug::INFO] << myname << "(buildReceiverList): init myport port=" << port << endl;
			continue;
		}

		int p = it.getIntProp("udp_port");
		if( p <=0 )
			p = n_id;

		if( p == DefaultObjectId )
		{
			dlog[Debug::WARN] << myname << "(buildReceiverList): node=" << it.getProp("name") << " unknown port. ignore..." << endl;
			continue;
		}

		UDPNReceiver* r = new UDPNReceiver(p,host,shm->getSMID(),shm->SM());
		rlist.push_back(r);
	}
}
// ------------------------------------------------------------------------------------------
