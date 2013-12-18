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

	signal_change_value().connect(sigc::mem_fun(*this, &SharedMemory::updateHistory));
	for( History::iterator i=hist.begin(); i!=hist.end(); ++i )
		histmap[i->fuse_id].push_back(i);

	// ----------------------
	restorer = NULL;
	NCRestorer_XML* rxml = new NCRestorer_XML(datafile);
	
	string s_field = conf->getArgParam("--s-filter-field");
	string s_fvalue = conf->getArgParam("--s-filter-value");
	string c_field = conf->getArgParam("--c-filter-field");
	string c_fvalue = conf->getArgParam("--c-filter-value");
	string t_field = conf->getArgParam("--t-filter-field");
	string t_fvalue = conf->getArgParam("--t-filter-value");
	
	heartbeat_node = conf->getArgParam("--heartbeat-node");
	if( heartbeat_node.empty() && dlog.is_warn() )
		dlog.warn() << myname << "(init): --heartbeat-node NULL ===> heartbeat NOT USED..." << endl;
	else if( dlog.is_info() )
		dlog.info() << myname << "(init): heartbeat-node: " << heartbeat_node << endl;

	heartbeatCheckTime = conf->getArgInt("--heartbeat-check-time","1000");

	rxml->setItemFilter(s_field, s_fvalue);
	rxml->setConsumerFilter(c_field, c_fvalue);
	rxml->setThresholdsFilter(t_field, t_fvalue);

	restorer = rxml;
	rxml->setReadItem( sigc::mem_fun(this,&SharedMemory::readItem) );
	
	string wdt_dev = conf->getArgParam("--wdt-device");
	if( !wdt_dev.empty() )
		wdt = new WDTInterface(wdt_dev);
	else if( dlog.is_warn() )
		dlog.warn() << myname << "(init): watchdog timer NOT USED (--wdt-device NULL)" << endl;


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
			if( dlog.is_crit() )
				dlog.crit() << myname << "(init): " << err.str() << endl;
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
				//dlog.warn() << myname << ": неизвестное сообщение  " << msg->type << endl;
				break;
		}	
	}
	catch( Exception& ex )
	{
		if( dlog.is_crit() )
			dlog.crit()  << myname << "(processingMessage): " << ex << endl;
	}
	catch(CORBA::SystemException& ex)
    {
		if( dlog.is_warn() )
			dlog.warn() << myname << "(processingMessage): CORBA::SystemException: " << ex.NP_minorString() << endl;
  	}
	catch(CORBA::Exception& ex)
    {
		if( dlog.is_warn() )
			dlog.warn() << myname << "(processingMessage): CORBA::Exception: " << ex._name() << endl;
	}
	catch( omniORB::fatalException& fe ) 
	{
		if( dlog.is_crit() )
		{
			dlog.crit() << myname << "(processingMessage): Caught omniORB::fatalException:" << endl;
		    dlog.crit() << myname << "(processingMessage): file: " << fe.file()
				<< " line: " << fe.line()
			<< " mesg: " << fe.errmsg() << endl;
		}
	}
	catch(...)
	{
		if( dlog.is_crit() )
			dlog.crit()  << myname << "(processingMessage): catch..." << endl;
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
			bool st = (bool)localGetValue(itPulsar,siPulsar);
			st ^= true;
			localSetValue(itPulsar,siPulsar, (st ? 1:0), getId() );
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
			
			if( !isActivated() && dlog.is_crit() )
				dlog.crit() << myname << "(sysCommand): ************* don`t activate?! ************" << endl;
		
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
				dlog.crit() << myname << "(askSensors): " << ex << endl;
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
			it->ioit = myioEnd();
		}
		
		itPulsar = myioEnd();

		for( History::iterator it=hist.begin();  it!=hist.end(); ++it )
		{
			for( HistoryList::iterator hit=it->hlst.begin(); hit!=it->hlst.end(); ++hit )
				hit->ioit = myioEnd();
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
			long val = localGetValue(it->ioit,si);
			val --;
			if( val < -1 )
				val = -1;
			localSetValue(it->ioit,si,val,getId());

			si.id = it->d_sid;
			localSetValue(it->ioit,si,( val >= 0 ? true:false),getId());

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
			if( dlog.is_crit() )
				dlog.crit() << myname << "(checkHeartBeat): " << ex << endl;
		}
		catch(...)
		{
			if( dlog.is_crit() )
				dlog.crit() << myname << "(checkHeartBeat): ..." << endl;
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
	
		if( dlog.is_crit() )
			dlog.crit() << msg.str() << endl;
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
			if( dlog.is_warn() )
				dlog.warn() << msg.str() << endl;
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
			if( dlog.is_crit() )
				dlog.crit() << msg.str() << endl;
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

		if( dlog.is_crit() )
			dlog.crit() << msg.str() << endl;
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
	
	if( dlog.is_info() )
		dlog.info() << "(smemory): datfile = " << dfile << endl;
	
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
		if( dlog.is_warn() )
			dlog.warn() << myname << "(readEventList): " << oname << " не найден..." << endl;
		return;
	}

	UniXML_iterator it(enode);
	if( !it.goChildren() )
	{
		if( dlog.is_warn() )
			dlog.warn() << myname << "(readEventList): <eventlist> пустой..." << endl;
		return;
	}

	for( ;it.getCurrent(); it.goNext() )
	{
		if( it.getProp(e_filter).empty() )
			continue;

		ObjectId oid = it.getIntProp("id");
		if( oid != 0 )
		{
			if( dlog.is_info() )
				dlog.info() << myname << "(readEventList): add " << it.getProp("name") << endl;
			elst.push_back(oid);
		}
		else if( dlog.is_crit() )
			dlog.crit() << myname << "(readEventList): Не найден ID для "
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
		
		if(!ok && dlog.is_crit() )
			dlog.crit() << myname << "(sendEvent): Объект " << (*it) << " НЕДОСТУПЕН" << endl;
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
	if( dlog.is_info() )
		dlog.info() << myname << "(buildHistoryList): ..."  << endl;

	UniXML* xml = conf->getConfXML();
	if( !xml )
	{
		if( dlog.is_warn() )
			dlog.warn() << myname << "(buildHistoryList): xml=NULL?!" << endl;
		return;
	}

	xmlNode* n = xml->extFindNode(cnode,1,1,"History","");
	if( !n )
	{
		if( dlog.is_warn() )
			dlog.warn() << myname << "(buildHistoryList): <History> not found. ignore..." << endl;
		hist.clear();
		return;
	}

	UniXML_iterator it(n);

	bool no_history = conf->getArgInt("--sm-no-history",it.getProp("no_history"));
	if( no_history )
	{
		if( dlog.is_warn() )
			dlog.warn() << myname << "(buildHistoryList): no_history='1'.. history skipped..." << endl;
		hist.clear();
		return;
	}
	
	histSaveTime = it.getIntProp("savetime");
	if( histSaveTime <= 0 )
		histSaveTime = 0;
	
	if( !it.goChildren() )
	{
		if( dlog.is_warn() )
			dlog.warn() << myname << "(buildHistoryList): <History> empty. ignore..." << endl;
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
			if( dlog.is_warn() )
				dlog.warn() << myname << "(buildHistory): not found sensor ID for "
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

		if( dlog.is_info() )
			dlog.info() << myname << "(buildHistory): add fuse_id=" << hi.fuse_id
				<< " fuse_val=" << hi.fuse_val
				<< " fuse_use_val=" << hi.fuse_use_val
				<< " fuse_invert=" << hi.fuse_invert
				<< endl;


		// WARNING: no check duplicates...
		hist.push_back(hi);
	}

	if( dlog.is_info() )
		dlog.info() << myname << "(buildHistoryList): history logs count=" << hist.size() << endl;
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
			if( dlog.is_warn() )
				dlog.warn() << myname << "(checkHistoryFilter): not found sensor ID for " << xit.getProp("name") << endl;
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
//	if( dlog.is_info() )
//		dlog.info() << myname << "(saveHistory): ..." << endl;

	for( History::iterator it=hist.begin();  it!=hist.end(); ++it )
	{
		for( HistoryList::iterator hit=it->hlst.begin(); hit!=it->hlst.end(); ++hit )
		{
			if( hit->ioit != myioEnd() )
				hit->add( localGetValue( hit->ioit, hit->ioit->second.si ), it->size );
			else
			{
				IOController_i::SensorInfo si;
				si.id 	= hit->id;
				si.node	= conf->getLocalNode();
				try
				{
					
					hit->add( localGetValue( hit->ioit, si ), it->size );
					continue;
				}
				catch(...){}
			}
		}
	}
}
// -----------------------------------------------------------------------------
void SharedMemory::updateHistory( IOStateList::iterator& s_it, IOController* )
{
	if( hist.empty() )
		return;

	HistoryFuseMap::iterator i = histmap.find(s_it->second.si.id);
	if( i == histmap.end() )
		return;

	long value = 0;
	long sm_tv_sec = 0;
	long sm_tv_usec = 0;
	{
		uniset_rwmutex_rlock lock(s_it->second.val_lock);
		value = s_it->second.value;
		sm_tv_sec = s_it->second.tv_sec;
		sm_tv_usec = s_it->second.tv_usec;
	}

	if( dlog.is_info() )
	{
		dlog.info() << myname << "(updateHistory): "
			<< " sid=" << s_it->second.si.id
			<< " value=" << value
			<< endl;
	}

	for( HistoryItList::iterator it1=i->second.begin(); it1!=i->second.end(); ++it1 )
	{
		History::iterator it( (*it1) );

		if( s_it->second.type == UniversalIO::DI ||
			s_it->second.type == UniversalIO::DO )
		{
			bool st = (bool)value;

			if( it->fuse_invert )
				st^=true;

			if( st )
			{
				if( dlog.is_info() )
					dlog.info() << myname << "(updateHistory): HISTORY EVENT for " << (*it) << endl;
		
				it->fuse_sec = sm_tv_sec;
				it->fuse_usec = sm_tv_usec;
				m_historySignal.emit( &(*it) );
			}
		}
		else if( s_it->second.type == UniversalIO::AI ||
				 s_it->second.type == UniversalIO::AO )
		{
			if( sm->sensor_type == UniversalIO::DigitalInput ||
				sm->sensor_type == UniversalIO::DigitalOutput )
			{
				bool st = (bool)value;

				if( it->fuse_invert )
					st^=true;

				if( !st )
				{
					if( dlog.is_info() )
						dlog.info() << myname << "(updateHistory): HISTORY EVENT for " << (*it) << endl;

					it->fuse_sec = sm_tv_sec;
					it->fuse_usec = sm_tv_usec;
					m_historySignal.emit( &(*it) );
				}
			}
			else if( sm->sensor_type == UniversalIO::AnalogInput ||
					 sm->sensor_type == UniversalIO::AnalogOutput )
			{
				if( value == it->fuse_val )
				{
					if( dlog.is_info() )
						dlog.info() << myname << "(updateHistory): HISTORY EVENT for " << (*it) << endl;

					it->fuse_sec = sm_tv_sec;
					it->fuse_usec = sm_tv_usec;
					m_historySignal.emit( &(*it) );
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
