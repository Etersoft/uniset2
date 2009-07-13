#include <iomanip>
#include <sstream>
#include "UniXML.h"
#include "NCRestorer.h"
#include "SharedMemory.h"
#include "Extensions.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// -----------------------------------------------------------------------------
SharedMemory::SharedMemory( ObjectId id, string datafile ):
	IONotifyController_LT(id),
	heartbeatCheckTime(5000),
	wdt(0),
	activated(false),
	workready(false),
	dblogging(false)
{
	cout << "$Id: SharedMemory.cc,v 1.4 2009/01/24 11:20:19 vpashka Exp $" << endl;

	xmlNode* cnode = conf->getNode("SharedMemory");
	if( cnode == NULL )
		throw SystemError("Not find conf-node for SharedMemory");

	UniXML_iterator it(cnode);	
	
	// ----------------------
	restorer = NULL;
	NCRestorer_XML* rxml = new NCRestorer_XML(datafile);
	// rxml = new NCRestorer_XML1(datafile);
	
	string s_field = conf->getArgParam("--s-filter-field");
	string s_fvalue = conf->getArgParam("--s-filter-value");
	string c_field = conf->getArgParam("--c-filter-field");
	string c_fvalue = conf->getArgParam("--c-filter-value");
	string d_field = conf->getArgParam("--d-filter-field");
	string d_fvalue = conf->getArgParam("--d-filter-value");
	
	int lock_msec = atoi(conf->getArgParam("--lock-value-pause").c_str());
	if( lock_msec < 0 )
		lock_msec = 0;
	setCheckLockValuePause(lock_msec);
	
	heartbeat_node = conf->getArgParam("--heartbeat-node");
	if( heartbeat_node.empty() )
		dlog[Debug::WARN] << myname << "(init): --heartbeat-node NULL ===> heartbeat NOT USED..." << endl;
	else
		dlog[Debug::INFO] << myname << "(init): heartbeat-node: " << heartbeat_node << endl;

	heartbeatCheckTime = atoi(conf->getArgParam("--heartbeat-check-time","1000").c_str());

	
//	rxml->setSensorFilter(s_filterField, s_filterValue);
//#warning Намеренно отключаем обработку списка заказчиков (в данном проекте)...
	// для отключения просто укажем несуществующие поля для фильтра
//	rxml->setConsumerFilter("dummy","yes");

	rxml->setItemFilter(s_field, s_fvalue);
	rxml->setConsumerFilter(c_field, c_fvalue);
	rxml->setDependsFilter(d_field, d_fvalue);

	restorer = rxml;

	rxml->setReadItem( sigc::mem_fun(this,&SharedMemory::readItem) );
	
	string wdt_dev = conf->getArgParam("--wdt-device");
	if( !wdt_dev.empty() )
		wdt = new WDTInterface(wdt_dev);
	else
		dlog[Debug::WARN] << myname << "(init): watchdog timer NOT USED (--wdt-device NULL)" << endl;


	dblogging = atoi(conf->getArgParam("--db-logging").c_str());

	e_filter = conf->getArgParam("--e-filter");
	buildEventList(cnode);

	evntPause = atoi(conf->getArgParam("--e-startup-pause").c_str());
	if( evntPause<=0 )
		evntPause = 5000;
		
	activateTimeout	= atoi(conf->getArgParam("--activate-timeout").c_str());
	if( activateTimeout<=0 )
		activateTimeout = 10000;
}

// --------------------------------------------------------------------------------

SharedMemory::~SharedMemory()
{
	if( restorer )
	{
		delete restorer;
		restorer = NULL;
	}
	
	delete wdt;
}

// --------------------------------------------------------------------------------
void SharedMemory::processingMessage( UniSetTypes::VoidMessage *msg )
{
	try
	{
		switch( msg->type )
		{
			case Message::SensorInfo:
			{
				SensorMessage sm( msg );
				sensorInfo( &sm );
				break;
			}

			case Message::SysCommand:
			{
				SystemMessage sm( msg );
				sysCommand( &sm );
				break;
			}

			case Message::Timer:
			{
				TimerMessage tm(msg);
				timerInfo(&tm);
				break;
			}


			default:
				//dlog[Debug::WARN] << myname << ": неизвестное сообщение  " << msg->type << endl;
				break;
		}	
	}
	catch( Exception& ex )
	{
		dlog[Debug::CRIT]  << myname << "(processingMessage): " << ex << endl;
	}
}

// ------------------------------------------------------------------------------------------

void SharedMemory::sensorInfo( SensorMessage *sm )
{
}

// ------------------------------------------------------------------------------------------

void SharedMemory::timerInfo( TimerMessage *tm )
{
	if( tm->id == tmHeartBeatCheck )
		checkHeartBeat();
	else if( tm->id == tmEvent )
	{
		workready = true;
		// рассылаем уведомление, о том, чтобы стартанули
		SystemMessage sm1(SystemMessage::WatchDog);
		sendEvent(sm1);
		askTimer(tm->id,0);
	}
}

// ------------------------------------------------------------------------------------------

void SharedMemory::sysCommand( SystemMessage *sm )
{
	switch( sm->command )
	{
		case SystemMessage::StartUp:
		{
			PassiveTimer ptAct(activateTimeout);
			while( !activated && !ptAct.checkTime() )
			{	
				cout << myname << "(sysCommand): wait activate..." << endl;
				msleep(100);
				if( activated )
					break;
			}
			
			if( !activated )
				dlog[Debug::CRIT] << myname << "(sysCommand): ************* don`t activate?! ************" << endl;
		
			// подождать пока пройдёт инициализация
			// см. activateObject()
			UniSetTypes::uniset_mutex_lock l(mutex_start, 10000);
			askTimer(tmHeartBeatCheck,heartbeatCheckTime);
			askTimer(tmEvent,evntPause,1);
		}
		break;
		
		case SystemMessage::FoldUp:								
		case SystemMessage::Finish:
			break;
		
		case SystemMessage::WatchDog:
			break;

		case SystemMessage::LogRotate:
		break;

		default:
			break;
	}
}

// ------------------------------------------------------------------------------------------

void SharedMemory::askSensors( UniversalIO::UIOCommand cmd )
{
	try
	{
//		ui.askState( SID, cmd);
	}
	catch(Exception& ex)
    {
		dlog[Debug::CRIT] << myname << "(askSensors): " << ex << endl;
	}
}

// ------------------------------------------------------------------------------------------
bool SharedMemory::activateObject()
{
	PassiveTimer pt(UniSetTimer::WaitUpTime);
	bool res = true;
	// блокирование обработки Startup 
	// пока не пройдёт инициализация датчиков
	// см. sysCommand()
	{
		activated = false;
		UniSetTypes::uniset_mutex_lock l(mutex_start, 5000);
		res = IONotifyController_LT::activateObject();

		// инициализируем указатели		
		for( HeartBeatList::iterator it=hlist.begin(); it!=hlist.end(); ++it )
		{
			it->ait 	= myaioEnd();
			it->dit 	= mydioEnd();
		}

		activated = true;
	}
	cerr << "************************** activate: " << pt.getCurrent() << " msec " << endl;
	return res;
}
// ------------------------------------------------------------------------------------------
CORBA::Boolean SharedMemory::exist()
{	
//	return activated;
	return workready;
}
// ------------------------------------------------------------------------------------------
void SharedMemory::sigterm( int signo )
{
	if( signo == SIGTERM )
		wdt->stop();		
//	raise(SIGKILL);
}
// ------------------------------------------------------------------------------------------
void SharedMemory::checkHeartBeat()
{
	IOController_i::SensorInfo si;
	si.node = conf->getLocalNode();

	bool wdtpingOK = true;

	for( HeartBeatList::iterator it=hlist.begin(); it!=hlist.end(); ++it )
	{
		try
		{
			si.id = it->a_sid;
			long val = localGetValue(it->ait,si);
			val --;
			if( val < -1 )
				val = -1;
			localSaveValue(it->ait,si,val,getId());

			si.id = it->d_sid;
			if( val >= 0 )
				localSaveState(it->dit,si,true,getId());
			else
				localSaveState(it->dit,si,false,getId());

			// проверяем нужна ли "перезагрузка" по данному датчику
			if( wdt && it->ptReboot.getInterval() )
			{
				if( val > 0  )
					it->timer_running = false;
				else
				{
					if( !it->timer_running )
					{
						it->timer_running = true;
						it->ptReboot.setTiming(it->reboot_msec);
					}
					else if( it->ptReboot.checkTime() )
						wdtpingOK = false;
				}
			}
		}
		catch(Exception& ex)
	    {
			dlog[Debug::CRIT] << myname << "(checkHeartBeat): " << ex << endl;
		}
		catch(...)
		{
			dlog[Debug::CRIT] << myname << "(checkHeartBeat): ..." << endl;
		}
	}
	
	if( wdt && wdtpingOK && workready )
		wdt->ping();
}
// ------------------------------------------------------------------------------------------
bool SharedMemory::readItem( UniXML& xml, UniXML_iterator& it, xmlNode* sec )
{
	for( ReadSlotList::iterator r=lstRSlot.begin(); r!=lstRSlot.end(); ++r )
	{
		try
		{	
			(*r)(xml,it,sec);
		}
		catch(...){}
	}


	if( heartbeat_node.empty() || it.getProp("heartbeat").empty())
		return true;

	if( heartbeat_node != it.getProp("heartbeat_node") )
		return true;

	int i = atoi(it.getProp("heartbeat").c_str());
	if( i<=0 )
		return true;

	if( it.getProp("iotype") != "AI" )
	{
		ostringstream msg;
		msg << "(SharedMemory::readItem): тип для 'heartbeat' датчика (" << it.getProp("name")
			<< ") указан неверно ("
			<< it.getProp("iotype") << ") должен быть 'AI'";
	
		dlog[Debug::CRIT] << msg.str() << endl;
		kill(getpid(),SIGTERM);
//		throw NameNotFound(msg.str());
	};

	HeartBeatInfo hi;
	hi.a_sid = UniSetTypes::uni_atoi( it.getProp("id").c_str() );

	if( it.getProp("heartbeat_ds_name").empty() )
	{
		if( hi.d_sid == DefaultObjectId )
		{
			ostringstream msg;
			msg << "(SharedMemory::readItem): дискретный датчик (heartbeat_ds_name) связанный с " << it.getProp("name");
			dlog[Debug::WARN] << msg.str() << endl;
#warning Делать обязательным?!			
//			dlog[Debug::CRIT] << msg.str() << endl;
//			kill(getpid(),SIGTERM);
			// throw NameNotFound(msg.str());
		}
	}
	else
	{
		hi.d_sid = conf->getSensorID(it.getProp("heartbeat_ds_name"));
		if( hi.d_sid == DefaultObjectId )
		{
			ostringstream msg;
			msg << "(SharedMemory::readItem): Не найден ID для дискретного датчика (heartbeat_ds_name) связанного с " << it.getProp("name");
			dlog[Debug::CRIT] << msg.str() << endl;
			kill(getpid(),SIGTERM);
//			throw NameNotFound(msg.str());
		}
	}

	hi.reboot_msec = UniSetTypes::uni_atoi( it.getProp("heartbeat_reboot_msec").c_str() );
	hi.ptReboot.setTiming(UniSetTimer::WaitUpTime);

	if( hi.a_sid <= 0 )
	{
		ostringstream msg;
		msg << "(SharedMemory::readItem): НЕ УКАЗАН id для " 
			<< it.getProp("name") << " секция " << sec;

		dlog[Debug::CRIT] << msg.str() << endl;
		kill(getpid(),SIGTERM);
//		throw NameNotFound(msg.str());
	};

	// без проверки на дублирование т.к. 
	// id - гарантирует уникальность в нашем configure.xml
	hlist.push_back(hi);
	return true;		
}
// ------------------------------------------------------------------------------------------
void SharedMemory::saveValue( const IOController_i::SensorInfo& si, CORBA::Long value,
								UniversalIO::IOTypes type, UniSetTypes::ObjectId sup_id )
{
	uniset_mutex_lock l(hbmutex);
	IONotifyController_LT::saveValue(si,value,type,sup_id);
}
// ------------------------------------------------------------------------------------------
void SharedMemory::fastSaveValue(const IOController_i::SensorInfo& si, CORBA::Long value,
					UniversalIO::IOTypes type, UniSetTypes::ObjectId sup_id )
{
	uniset_mutex_lock l(hbmutex);
	IONotifyController_LT::fastSaveValue(si,value,type,sup_id);
}					
// ------------------------------------------------------------------------------------------
SharedMemory* SharedMemory::init_smemory( int argc, char* argv[] )
{
	string dfile = conf->getArgParam("--datfile",conf->getConfFileName());

	if( dfile[0]!='.' && dfile[0]!='/' )
		dfile = conf->getConfDir() + dfile;
	
	dlog[Debug::INFO] << "(smemory): datfile = " << dfile << endl;
	
	UniSetTypes::ObjectId ID = conf->getControllerID(conf->getArgParam("--smemory-id","SharedMemory"));
	if( ID == UniSetTypes::DefaultObjectId )
	{
		cerr << "(smemory): НЕ ЗАДАН идентификатор '" 
			<< " или не найден в " << conf->getControllersSection()
                        << endl;
		return 0;
	}
	return new SharedMemory(ID,dfile);
}
// -----------------------------------------------------------------------------
void SharedMemory::help_print( int argc, char* argv[] )
{
	cout << "--smemory-id           - SharedMemeory ID" << endl;
	cout << "--logfile fname	- выводить логи в файл fname. По умолчанию smemory.log" << endl;
	cout << "--datfile fname	- Файл с картой датчиков. По умолчанию configure.xml" << endl;
	cout << "--s-filter-field	- Фильтр для загрузки списка датчиков." << endl;
	cout << "--s-filter-value	- Значение фильтра для загрузки списка датчиков." << endl;
	cout << "--c-filter-field	- Фильтр для загрузки списка заказчиков." << endl;
	cout << "--c-filter-value	- Значение фильтр для загрузки списка заказчиков." << endl;
	cout << "--d-filter-field	- Фильтр для загрузки списка зависимостей." << endl;
	cout << "--d-filter-value	- Значение фильтр для загрузки списка зависимостей." << endl;
	cout << "--wdt-device		- Использовать в качестве WDT указанный файл." << endl;
	cout << "--heartbeat-node	- Загружать heartbeat датчики для указанного узла." << endl;
	cout << "--lock-value-pause - пауза между проверкой spin-блокировки на значение" << endl;
	cout << "--e-filter 		- фильтр для считывания <eventlist>" << endl;
	cout << "--e-startup-pause 	- пауза перед посылкой уведомления о старте SM. (По умолчанию: 1500 мсек)." << endl;
	cout << "--activate-timeout - время ожидания активизации (По умолчанию: 15000 мсек)." << endl;

}
// -----------------------------------------------------------------------------
void SharedMemory::buildEventList( xmlNode* cnode )
{
	readEventList("objects");
	readEventList("controllers");
	readEventList("services");
}
// -----------------------------------------------------------------------------
void SharedMemory::readEventList( std::string oname )
{
	xmlNode* enode = conf->getNode(oname);
	if( enode == NULL )
	{
		dlog[Debug::WARN] << myname << "(readEventList): " << oname << " не найден..." << endl;
		return;
	}

	UniXML_iterator it(enode);
	if( !it.goChildren() )
	{
		dlog[Debug::WARN] << myname << "(readEventList): <eventlist> пустой..." << endl;
		return;
	}

	for( ;it.getCurrent(); it.goNext() )
	{
		if( it.getProp(e_filter).empty() )
			continue;

		ObjectId oid = UniSetTypes::uni_atoi(it.getProp("id").c_str());
		if( oid != 0 )
		{
			if( dlog.debugging(Debug::INFO) )
				dlog[Debug::INFO] << myname << "(readEventList): add " << it.getProp("name") << endl;
			elst.push_back(oid);
		}
		else
			dlog[Debug::CRIT] << myname << "(readEventList): Не найден ID для " 
				<< it.getProp("name") << endl;
	}
}
// -----------------------------------------------------------------------------
void SharedMemory::sendEvent( UniSetTypes::SystemMessage& sm )
{
	TransportMessage tm(sm.transport_msg());

	for( EventList::iterator it=elst.begin(); it!=elst.end(); it++ )
	{
		bool ok = false;
		for( int i=0; i<2; i++ )
		{
			try
			{
				ui.send((*it),tm);
				ok = true;
				break;
			}
			catch(...){};
		}
		
		if(!ok)
			dlog[Debug::CRIT] << myname << "(sendEvent): Объект " << (*it) << " НЕДОСТУПЕН" << endl;
	}	
}
// -----------------------------------------------------------------------------
void SharedMemory::addReadItem( Restorer_XML::ReaderSlot sl )
{
	lstRSlot.push_back(sl);
}
// -----------------------------------------------------------------------------
void SharedMemory::loggingInfo( SensorMessage& sm )
{
	if( dblogging )
		IONotifyController_LT::loggingInfo(sm);
}
// -----------------------------------------------------------------------------
