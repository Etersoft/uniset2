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
	histSaveTime(200),
	wdt(0),
	activated(false),
	workready(false),
	dblogging(false),
	msecPulsar(0)
{
	cout << "$Id: SharedMemory.cc,v 1.4 2009/01/24 11:20:19 vpashka Exp $" << endl;

	xmlNode* cnode = conf->getNode("SharedMemory");
	if( cnode == NULL )
		throw SystemError("Not find conf-node for SharedMemory");

	UniXML_iterator it(cnode);

	// ----------------------
	buildHistoryList(cnode);
	signal_change_state().connect(sigc::mem_fun(*this, &SharedMemory::updateHistory));
	// ----------------------
	restorer = NULL;
	NCRestorer_XML* rxml = new NCRestorer_XML(datafile);
	
	string s_field = conf->getArgParam("--s-filter-field");
	string s_fvalue = conf->getArgParam("--s-filter-value");
	string c_field = conf->getArgParam("--c-filter-field");
	string c_fvalue = conf->getArgParam("--c-filter-value");
	string d_field = conf->getArgParam("--d-filter-field");
	string d_fvalue = conf->getArgParam("--d-filter-value");
	
	int lock_msec = conf->getArgInt("--lock-value-pause");
	if( lock_msec < 0 )
		lock_msec = 0;
	setCheckLockValuePause(lock_msec);
	
	heartbeat_node = conf->getArgParam("--heartbeat-node");
	if( heartbeat_node.empty() )
		dlog[Debug::WARN] << myname << "(init): --heartbeat-node NULL ===> heartbeat NOT USED..." << endl;
	else
		dlog[Debug::INFO] << myname << "(init): heartbeat-node: " << heartbeat_node << endl;

	heartbeatCheckTime = conf->getArgInt("--heartbeat-check-time","1000");

	
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


	dblogging = conf->getArgInt("--db-logging");

	e_filter = conf->getArgParam("--e-filter");
	buildEventList(cnode);

	evntPause = conf->getArgPInt("--e-startup-pause", 5000);

	activateTimeout	= conf->getArgPInt("--activate-timeout", 10000);

	siPulsar.id = DefaultObjectId;
	siPulsar.node = DefaultObjectId;
	string p = conf->getArgParam("--pulsar-id",it.getProp("pulsar_id"));
	if( !p.empty() )
	{
		siPulsar.id = conf->getSensorID(p);
		if( siPulsar.id == DefaultObjectId )
		{
			ostringstream err;
			err << myname << ": ID not found ('pulsar') for " << p;
			dlog[Debug::CRIT] << myname << "(init): " << err.str() << endl;
			throw SystemError(err.str());
		}
		siPulsar.node = conf->getLocalNode();

		msecPulsar = conf->getArgPInt("--pulsar-msec",it.getProp("pulsar_msec"), 5000);
	}
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
	else if( tm->id == tmHistory )
		saveHistory();
	else if( tm->id == tmPulsar )
	{
		bool st = localGetState(ditPulsar,siPulsar);
		st ^= true;
		localSaveState(ditPulsar,siPulsar,st,getId());
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
			askTimer(tmHistory,histSaveTime);
			if( msecPulsar > 0 )
				askTimer(tmPulsar,msecPulsar);
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
/*
	for( History::iterator it=hist.begin();  it!=hist.end(); ++it )
	{
		if( sm->id == it->fuse_id )
		{
			try
			{
				ui.askState( SID, cmd);
			}
			catch(Exception& ex)
		    {
				dlog[Debug::CRIT] << myname << "(askSensors): " << ex << endl;
			}
	}
*/	
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
			it->ait = myaioEnd();
			it->dit = mydioEnd();
		}
		
		ditPulsar = mydioEnd();

//		cerr << "history count=" << hist.size() << endl;
		for( History::iterator it=hist.begin();  it!=hist.end(); ++it )
		{
//			cerr << "history for id=" << it->id << " count=" << it->hlst.size() << endl;
			for( HistoryList::iterator hit=it->hlst.begin(); hit!=it->hlst.end(); ++hit )
			{
				hit->ait = myaioEnd();
				hit->dit = mydioEnd();
			}
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

	// check history filters
	checkHistoryFilter(it);


	if( heartbeat_node.empty() || it.getProp("heartbeat").empty())
		return true;

	if( heartbeat_node != it.getProp("heartbeat_node") )
		return true;

	int i = it.getIntProp("heartbeat");
	if( i <= 0 )
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
	hi.a_sid = it.getIntProp("id");

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

	hi.reboot_msec = it.getIntProp("heartbeat_reboot_msec");
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
SharedMemory* SharedMemory::init_smemory( int argc, const char* const* argv )
{
	string dfile = conf->getArgParam("--datfile", conf->getConfFileName());

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
void SharedMemory::help_print( int argc, const char* const* argv )
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

		ObjectId oid = it.getIntProp("id");
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
void SharedMemory::buildHistoryList( xmlNode* cnode )
{
	if( dlog.debugging(Debug::INFO) )
		dlog[Debug::INFO] << myname << "(buildHistoryList): ..."  << endl;

	xmlNode* n = conf->findNode(cnode,"History");
	if( !n )
	{
		dlog[Debug::WARN] << myname << "(buildHistoryList): <History> not found. ignore..." << endl;
		hist.clear();
		return;
	}

	UniXML_iterator it(n);
	
	histSaveTime = it.getIntProp("savetime");
	if( histSaveTime < 0 )
		histSaveTime = 200;
	
	if( !it.goChildren() )
	{
		dlog[Debug::WARN] << myname << "(buildHistoryList): <History> empty. ignore..." << endl;
		return;
	}

	for( ; it.getCurrent(); it.goNext() )
	{
		HistoryInfo hi;
		hi.id 		= it.getIntProp("id");
		hi.size 	= it.getIntProp("size");
		if( hi.size <= 0 )
			continue;

		hi.filter 	= it.getProp("filter");
		if( hi.filter.empty() )
			continue;

		hi.fuse_id = conf->getSensorID(it.getProp("fuse_id"));
		if( hi.fuse_id == DefaultObjectId )
		{
			dlog[Debug::WARN] << myname << "(buildHistory): not found sensor ID for " 
				<< it.getProp("fuse_id")
				<< " history item id=" << it.getProp("id") 
				<< " ..ignore.." << endl;
			continue;
		}

		hi.fuse_invert 	= it.getIntProp("fuse_invert");
		
		if( !it.getProp("fuse_value").empty() )
		{
			hi.fuse_use_val = true;
			hi.fuse_val	= it.getIntProp("fuse_value");
		}

		// WARNING: no check duplicates...
		hist.push_back(hi);
	}

	if( dlog.debugging(Debug::INFO) )
		dlog[Debug::INFO] << myname << "(buildHistoryList): history logs count=" << hist.size() << endl;
}
// -----------------------------------------------------------------------------
void SharedMemory::checkHistoryFilter( UniXML_iterator& xit )
{
	for( History::iterator it=hist.begin();  it!=hist.end(); ++it )
	{
		if( xit.getProp(it->filter).empty() )
			continue;

		HistoryItem ai;

		if( !xit.getProp("id").empty() )
		{
			ai.id = xit.getIntProp("id");
			it->hlst.push_back(ai);
			continue;
		}

		ai.id = conf->getSensorID(xit.getProp("name"));
		if( ai.id == DefaultObjectId )
		{
			dlog[Debug::WARN] << myname << "(checkHistoryFilter): not found sensor ID for " << xit.getProp("name") << endl;
			continue;
		}
		
		it->hlst.push_back(ai);
	}
}
// -----------------------------------------------------------------------------
SharedMemory::HistorySlot SharedMemory::signal_history()
{
	return m_historySignal;
}
// -----------------------------------------------------------------------------
void SharedMemory::saveHistory()
{
//	if( dlog.debugging(Debug::INFO) )
//		dlog[Debug::INFO] << myname << "(saveHistory): ..." << endl;

	for( History::iterator it=hist.begin();  it!=hist.end(); ++it )
	{
		for( HistoryList::iterator hit=it->hlst.begin(); hit!=it->hlst.end(); ++hit )
		{
			if( hit->ait != myaioEnd() )
				hit->add( localGetValue( hit->ait, hit->ait->second.si ), it->size );
			else if( hit->dit != mydioEnd() )
				hit->add( localGetState( hit->dit, hit->dit->second.si ), it->size );
			else
			{
				IOController_i::SensorInfo si;
				si.id 	= hit->id;
				si.node	= conf->getLocalNode();
				try
				{
					
					hit->add( localGetValue( hit->ait, si ), it->size );
					continue;
				}
				catch(...){}

				try
				{
					hit->add( localGetState( hit->dit, si ), it->size );
					continue;
				}
				catch(...){}
			}
		}
	}
}
// -----------------------------------------------------------------------------
void SharedMemory::updateHistory( UniSetTypes::SensorMessage* sm )
{
	if( hist.empty() )
		return;

	if( dlog.debugging(Debug::INFO) )
	{
		dlog[Debug::INFO] << myname << "(updateHistory): " 
			<< " sid=" << sm->id 
			<< " state=" << sm->state 
			<< " value=" << sm->value
			<< endl;
	}

	for( History::iterator it=hist.begin();  it!=hist.end(); ++it )
	{
		if( sm->id == it->fuse_id )
		{
			if( sm->sensor_type == UniversalIO::DigitalInput ||
				sm->sensor_type == UniversalIO::DigitalOutput )
			{
				bool st = it->fuse_invert ? !sm->state : sm->state;
				if( st )
				{
					if( dlog.debugging(Debug::INFO) )
						dlog[Debug::INFO] << myname << "(updateHistory): HISTORY EVENT for " << (*it) << endl;
			
					m_historySignal.emit( &(*it) );
				}
			}
			else if( sm->sensor_type == UniversalIO::AnalogInput ||
					 sm->sensor_type == UniversalIO::AnalogOutput )
			{
				if( !it->fuse_use_val )
				{
					bool st = it->fuse_invert ? !sm->state : sm->state;
					if( !st )
					{
						if( dlog.debugging(Debug::INFO) )
							dlog[Debug::INFO] << myname << "(updateHistory): HISTORY EVENT for " << (*it) << endl;
			
						m_historySignal.emit( &(*it) );
					}
				}
				else
				{
					if( sm->value == it->fuse_val )
					{
						if( dlog.debugging(Debug::INFO) )
							dlog[Debug::INFO] << myname << "(updateHistory): HISTORY EVENT for " << (*it) << endl;
			
						m_historySignal.emit( &(*it) );
					}
				}
			}
		}
	}
}
// -----------------------------------------------------------------------------
std::ostream& operator<<( std::ostream& os, const SharedMemory::HistoryInfo& h )
{
	os << "History id=" << h.id 
		<< " fuse_id=" << h.fuse_id
		<< " fuse_invert=" << h.fuse_invert
		<< " fuse_val=" << h.fuse_val
		<< " size=" << h.size
		<< " filter=" << h.filter << endl;
	
	for( SharedMemory::HistoryList::const_iterator it=h.hlst.begin(); it!=h.hlst.end(); ++it )
	{
		os << "    id=" << it->id << "[";
		for( SharedMemory::HBuffer::const_iterator i=it->buf.begin(); i!=it->buf.end(); ++i )
			os << " " << (*i);
		os << " ]" << endl;
	}
	
	return os;
}
// ------------------------------------------------------------------------------------------
