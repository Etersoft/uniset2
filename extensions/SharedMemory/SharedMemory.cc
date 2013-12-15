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
void SharedMemory::help_print( int argc, const char* const* argv )
{
	cout << "--smemory-id           - SharedMemeory ID" << endl;
	cout << "--logfile fname        - выводить логи в файл fname. По умолчанию smemory.log" << endl;
	cout << "--datfile fname        - Файл с картой датчиков. По умолчанию configure.xml" << endl;
	cout << "--s-filter-field       - Фильтр для загрузки списка датчиков." << endl;
	cout << "--s-filter-value       - Значение фильтра для загрузки списка датчиков." << endl;
	cout << "--c-filter-field       - Фильтр для загрузки списка заказчиков." << endl;
	cout << "--c-filter-value       - Значение фильтр для загрузки списка заказчиков." << endl;
	cout << "--d-filter-field       - Фильтр для загрузки списка зависимостей." << endl;
	cout << "--d-filter-value       - Значение фильтр для загрузки списка зависимостей." << endl;
	cout << "--wdt-device           - Использовать в качестве WDT указанный файл." << endl;
	cout << "--heartbeat-node       - Загружать heartbeat датчики для указанного узла." << endl;
	cout << "--heartbeat-check-time - период проверки 'счётчиков'. По умолчанию 1000 мсек" << endl;
	cout << "--lock-rvalue-pause-msec - пауза между проверкой rw-блокировки на разрешение чтения" << endl;
	cout << "--e-filter             - фильтр для считывания <eventlist>" << endl;
	cout << "--e-startup-pause      - пауза перед посылкой уведомления о старте SM. (По умолчанию: 1500 мсек)." << endl;
	cout << "--activate-timeout     - время ожидания активизации (По умолчанию: 15000 мсек)." << endl;
	cout << "--sm-no-history        - отключить ведение истории (аварийного следа)" << endl;
	cout << "--pulsar-id            - датчик 'мигания'" << endl;
    cout << "--pulsar-msec          - период 'мигания'. По умолчанию: 5000." << endl;
    cout << "--pulsar-iotype        - [DI|DO]тип датчика для 'мигания'. По умолчанию DI." << endl;
}
// -----------------------------------------------------------------------------
SharedMemory::SharedMemory( ObjectId id, string datafile, std::string confname ):
	IONotifyController_LT(id),
	heartbeatCheckTime(5000),
	histSaveTime(0),
	wdt(0),
	activated(false),
	workready(false),
	dblogging(false),
	iotypePulsar(UniversalIO::DI),
	msecPulsar(0)
{
	mutex_start.setName(myname + "_mutex_start");
	mutex_act.setName(myname + "_mutex_act");

	string cname(confname);
	if( cname.empty() )
		cname = ORepHelpers::getShortName(conf->oind->getMapName(id));

	xmlNode* cnode =  conf->getNode(cname);
	if( cnode == NULL )
		throw SystemError("Not found conf-node for " + cname );

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
	string t_field = conf->getArgParam("--t-filter-field");
	string t_fvalue = conf->getArgParam("--t-filter-value");
	
	heartbeat_node = conf->getArgParam("--heartbeat-node");
	if( heartbeat_node.empty() )
		dlog[Debug::WARN] << myname << "(init): --heartbeat-node NULL ===> heartbeat NOT USED..." << endl;
	else
		dlog[Debug::INFO] << myname << "(init): heartbeat-node: " << heartbeat_node << endl;

	heartbeatCheckTime = conf->getArgInt("--heartbeat-check-time","1000");

	rxml->setItemFilter(s_field, s_fvalue);
	rxml->setConsumerFilter(c_field, c_fvalue);
	rxml->setDependsFilter(d_field, d_fvalue);
	rxml->setThresholdsFilter(t_field, t_fvalue);

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
		
		string t = conf->getArgParam("--pulsar-iotype",it.getProp("pulsar_iotype"));
		if( !t.empty() )
		{
			iotypePulsar = UniSetTypes::getIOType(t);
			if( iotypePulsar == UniversalIO::UnknownIOType || 
				iotypePulsar == UniversalIO::AI || iotypePulsar == UniversalIO::AO )
			{
				ostringstream err;
				err << myname << ": Invalid iotype '" << t << "' for pulsar. Must be 'DI' or 'DO'";
				dlog[Debug::CRIT] << myname << "(init): " << err.str() << endl;
				throw SystemError(err.str());
			}
		}
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
	catch(CORBA::SystemException& ex)
    {
		dlog[Debug::WARN] << myname << "(processingMessage): CORBA::SystemException: " << ex.NP_minorString() << endl;
  	}
	catch(CORBA::Exception& ex)
    {
		dlog[Debug::WARN] << myname << "(processingMessage): CORBA::Exception: " << ex._name() << endl;
	}
	catch( omniORB::fatalException& fe ) 
	{
		dlog[Debug::CRIT] << myname << "(processingMessage): Caught omniORB::fatalException:" << endl;
	    dlog[Debug::CRIT] << myname << "(processingMessage): file: " << fe.file()
			<< " line: " << fe.line()
	    	<< " mesg: " << fe.errmsg() << endl;
	}
	catch(...)
	{
		dlog[Debug::CRIT]  << myname << "(processingMessage): catch..." << endl;
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
		if( siPulsar.id != DefaultObjectId )
		{
			bool st = localGetState(ditPulsar,siPulsar);
			st ^= true;
			if( iotypePulsar == UniversalIO::DI )
				localSaveState(ditPulsar,siPulsar,st,getId());
			else if( iotypePulsar == UniversalIO::DO )
				localSetState(ditPulsar,siPulsar,st,getId());
		}
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
			while( !isActivated() && !ptAct.checkTime() )
			{	
				cout << myname << "(sysCommand): wait activate..." << endl;
				msleep(100);
			}
			
			if( !isActivated() )
				dlog[Debug::CRIT] << myname << "(sysCommand): ************* don`t activate?! ************" << endl;
		
			// подождать пока пройдёт инициализация
			// см. activateObject()
			UniSetTypes::uniset_rwmutex_rlock l(mutex_start);
			askTimer(tmHeartBeatCheck,heartbeatCheckTime);
			askTimer(tmEvent,evntPause,1);

			if( histSaveTime > 0 && !hist.empty() )
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
		{
			uniset_rwmutex_wrlock l(mutex_act);
			activated = false;
		}
		
		UniSetTypes::uniset_rwmutex_wrlock l(mutex_start);
		res = IONotifyController_LT::activateObject();

		// инициализируем указатели		
		for( HeartBeatList::iterator it=hlist.begin(); it!=hlist.end(); ++it )
		{
			it->ait = myioEnd();
			it->dit = mydioEnd();
		}
		
		ditPulsar = mydioEnd();

//		cerr << "history count=" << hist.size() << endl;
		for( History::iterator it=hist.begin();  it!=hist.end(); ++it )
		{
//			cerr << "history for id=" << it->id << " count=" << it->hlst.size() << endl;
			for( HistoryList::iterator hit=it->hlst.begin(); hit!=it->hlst.end(); ++hit )
			{
				hit->ait = myioEnd();
				hit->dit = mydioEnd();
			}
		}

		{
			uniset_rwmutex_wrlock l(mutex_act);
			activated = true;
		}
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
	if( hlist.empty() )
	{
		if( wdt && workready )
			wdt->ping();
		return;
	}

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
		}
	}
	else
	{
		hi.d_sid = conf->getSensorID(it.getProp("heartbeat_ds_name"));
		if( hi.d_sid == DefaultObjectId )
		{
			ostringstream msg;
			msg << "(SharedMemory::readItem): Не найден ID для дискретного датчика (heartbeat_ds_name) связанного с " << it.getProp("name");
			
			// Если уж задали имя для датчика, то он должен существовать..
			// поэтому завершаем процесс, если не нашли..
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

	string cname = conf->getArgParam("--smemory--confnode", ORepHelpers::getShortName(conf->oind->getMapName(ID)) );

	return new SharedMemory(ID,dfile,cname);
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

	UniXML* xml = conf->getConfXML();
	if( !xml )
	{
		dlog[Debug::WARN] << myname << "(buildHistoryList): xml=NULL?!" << endl;
		return;
	}

	xmlNode* n = xml->extFindNode(cnode,1,1,"History","");
	if( !n )
	{
		dlog[Debug::WARN] << myname << "(buildHistoryList): <History> not found. ignore..." << endl;
		hist.clear();
		return;
	}

	UniXML_iterator it(n);

	bool no_history = conf->getArgInt("--sm-no-history",it.getProp("no_history"));
	if( no_history )
	{
		dlog[Debug::WARN] << myname << "(buildHistoryList): no_history='1'.. history skipped..." << endl;
		hist.clear();
		return;
	}
	
	histSaveTime = it.getIntProp("savetime");
	if( histSaveTime <= 0 )
		histSaveTime = 0;
	
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
		
		hi.fuse_use_val = false;
		if( !it.getProp("fuse_value").empty() )
		{
			hi.fuse_use_val = true;
			hi.fuse_val	= it.getIntProp("fuse_value");
		}

		dlog[Debug::INFO] << myname << "(buildHistory): add fuse_id=" << hi.fuse_id 
				<< " fuse_val=" << hi.fuse_val
				<< " fuse_use_val=" << hi.fuse_use_val
				<< " fuse_invert=" << hi.fuse_invert
				<< endl;


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
	if( hist.empty() )
		return;
//	if( dlog.debugging(Debug::INFO) )
//		dlog[Debug::INFO] << myname << "(saveHistory): ..." << endl;

	for( History::iterator it=hist.begin();  it!=hist.end(); ++it )
	{
		for( HistoryList::iterator hit=it->hlst.begin(); hit!=it->hlst.end(); ++hit )
		{
			if( hit->ait != myioEnd() )
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
		History::iterator it( (*it1) );

		if( sm->sensor_type == UniversalIO::DI ||
			sm->sensor_type == UniversalIO::DO )
		{
			bool st = it->fuse_invert ? !sm->state : sm->state;
			if( st )
			{
				if( dlog.debugging(Debug::INFO) )
					dlog[Debug::INFO] << myname << "(updateHistory): HISTORY EVENT for " << (*it) << endl;
		
				it->fuse_sec = sm->sm_tv_sec;
				it->fuse_usec = sm->sm_tv_usec;
				m_historySignal.emit( &(*it) );
			}
		}
		else if( sm->sensor_type == UniversalIO::AI ||
				 sm->sensor_type == UniversalIO::AO )
>>>>>>> Первый этап переделок в связи с переходом на getValue/setValue
		{
			if( sm->sensor_type == UniversalIO::DigitalInput ||
				sm->sensor_type == UniversalIO::DigitalOutput )
			{
				bool st = it->fuse_invert ? !sm->state : sm->state;
				if( st )
				{
					if( dlog.debugging(Debug::INFO) )
						dlog[Debug::INFO] << myname << "(updateHistory): HISTORY EVENT for " << (*it) << endl;
			
					it->fuse_tm = sm->tm;
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

						it->fuse_tm = sm->tm;			
						m_historySignal.emit( &(*it) );
					}
				}
				else
				{
					if( sm->value == it->fuse_val )
					{
						if( dlog.debugging(Debug::INFO) )
							dlog[Debug::INFO] << myname << "(updateHistory): HISTORY EVENT for " << (*it) << endl;
			
						it->fuse_tm = sm->tm;
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
bool SharedMemory::isActivated()
{
	uniset_rwmutex_rlock l(mutex_act);
	return activated;
}
// ------------------------------------------------------------------------------------------
