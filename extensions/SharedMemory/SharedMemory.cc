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
#include <iomanip>
#include <sstream>
#include "UniXML.h"
#include "NCRestorer.h"
#include "SharedMemory.h"
#include "Extensions.h"
#include "ORepHelpers.h"
#include "SMLogSugar.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// -----------------------------------------------------------------------------
void SharedMemory::help_print( int argc, const char* const* argv )
{
	cout << "--smemory-id           - SharedMemeory ID" << endl;
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
	cout << endl;
	cout << " Logs: " << endl;
	cout << "--sm-log-...            - log control" << endl;
	cout << "         add-levels ..." << endl;
	cout << "         del-levels ..." << endl;
	cout << "         set-levels ..." << endl;
	cout << "         logfile filaname" << endl;
	cout << "         no-debug " << endl;
	cout << " LogServer: " << endl;
	cout << "--sm-run-logserver       - run logserver. Default: localhost:id" << endl;
	cout << "--sm-logserver-host ip   - listen ip. Default: localhost" << endl;
	cout << "--sm-logserver-port num  - listen port. Default: ID" << endl;
	cout << LogServer::help_print("sm-logserver") << endl;
}
// -----------------------------------------------------------------------------
SharedMemory::SharedMemory( ObjectId id, const std::string& datafile, const std::string& confname ):
	IONotifyController(id),
	heartbeatCheckTime(5000),
	histSaveTime(0),
	activated(false),
	workready(false),
	dblogging(false),
	msecPulsar(0),
	confnode(0)
{
	mutex_start.setName(myname + "_mutex_start");

	auto conf = uniset_conf();

	string cname(confname);

	if( cname.empty() )
		cname = ORepHelpers::getShortName( conf->oind->getMapName(id));

	confnode = conf->getNode(cname);

	if( confnode == NULL )
		throw SystemError("Not found conf-node for " + cname );

	string prefix = "sm";

	smlog = make_shared<DebugStream>();
	smlog->setLogName(myname);
	conf->initLogStream(smlog, prefix + "-log");

	loga = make_shared<LogAgregator>();
	loga->add(smlog);
	loga->add(ulog());
	loga->add(dlog());

	UniXML::iterator it(confnode);

	logserv = make_shared<LogServer>(loga);
	logserv->init( prefix + "-logserver", confnode );

	// ----------------------
	if( findArgParam("--" + prefix + "-run-logserver", conf->getArgc(), conf->getArgv()) != -1 )
	{
		logserv_host = conf->getArg2Param("--" + prefix + "-logserver-host", it.getProp("logserverHost"), "localhost");
		logserv_port = conf->getArgPInt("--" + prefix + "-logserver-port", it.getProp("logserverPort"), getId());
	}

	// ----------------------
	buildHistoryList(confnode);
	signal_change_value().connect(sigc::mem_fun(*this, &SharedMemory::updateHistory));

	for( auto i = hist.begin(); i != hist.end(); ++i )
		histmap[i->fuse_id].push_back(i);

	// ----------------------
	auto rxml = make_shared<NCRestorer_XML>(datafile);

	string s_field(conf->getArgParam("--s-filter-field"));
	string s_fvalue(conf->getArgParam("--s-filter-value"));
	string c_field(conf->getArgParam("--c-filter-field"));
	string c_fvalue(conf->getArgParam("--c-filter-value"));
	string t_field(conf->getArgParam("--t-filter-field"));
	string t_fvalue(conf->getArgParam("--t-filter-value"));

	heartbeat_node = conf->getArgParam("--heartbeat-node");

	if( heartbeat_node.empty() )
	{
		smwarn << myname << "(init): --heartbeat-node NULL ===> heartbeat NOT USED..." << endl;
	}
	else
		sminfo << myname << "(init): heartbeat-node: " << heartbeat_node << endl;

	heartbeatCheckTime = conf->getArgInt("--heartbeat-check-time", "1000");

	rxml->setItemFilter(s_field, s_fvalue);
	rxml->setConsumerFilter(c_field, c_fvalue);
	rxml->setThresholdsFilter(t_field, t_fvalue);

	restorer = std::static_pointer_cast<NCRestorer>(rxml);
	rxml->setReadItem( sigc::mem_fun(this, &SharedMemory::readItem) );

	string wdt_dev = conf->getArgParam("--wdt-device");

	if( !wdt_dev.empty() )
		wdt = make_shared<WDTInterface>(wdt_dev);
	else
		smwarn << myname << "(init): watchdog timer NOT USED (--wdt-device NULL)" << endl;

	dblogging = conf->getArgInt("--db-logging");

	e_filter = conf->getArgParam("--e-filter");
	buildEventList(confnode);

	evntPause = conf->getArgPInt("--e-startup-pause", 5000);

	activateTimeout = conf->getArgPInt("--activate-timeout", 30000);

	sidPulsar = DefaultObjectId;
	string p = conf->getArgParam("--pulsar-id", it.getProp("pulsar_id"));

	if( !p.empty() )
	{
		sidPulsar = conf->getSensorID(p);

		if( sidPulsar == DefaultObjectId )
		{
			ostringstream err;
			err << myname << ": ID not found ('pulsar') for " << p;
			smcrit << myname << "(init): " << err.str() << endl;
			throw SystemError(err.str());
		}

		msecPulsar = conf->getArgPInt("--pulsar-msec", it.getProp("pulsar_msec"), 5000);
	}

	// Мониторинг переменных
	vmonit(sidPulsar);
	vmonit(msecPulsar);
	vmonit(activateTimeout);
	vmonit(evntPause);
	vmonit(heartbeatCheckTime);
	vmonit(histSaveTime);
	vmonit(dblogging);
	vmonit(heartbeatCheckTime);
	vmonit(heartbeat_node);
}

// --------------------------------------------------------------------------------

SharedMemory::~SharedMemory()
{
}

// --------------------------------------------------------------------------------
void SharedMemory::timerInfo( const TimerMessage* tm )
{
	if( tm->id == tmHeartBeatCheck )
		checkHeartBeat();
	else if( tm->id == tmEvent )
	{
		workready = 1;
		// рассылаем уведомление, о том, чтобы стартанули
		SystemMessage sm1(SystemMessage::WatchDog);
		sendEvent(sm1);
		askTimer(tm->id, 0);
	}
	else if( tm->id == tmHistory )
		saveHistory();
	else if( tm->id == tmPulsar )
	{
		if( sidPulsar != DefaultObjectId )
		{
			bool st = (bool)localGetValue(itPulsar, sidPulsar);
			st ^= true;
			localSetValueIt(itPulsar, sidPulsar, (st ? 1 : 0), getId() );
		}
	}
}

// ------------------------------------------------------------------------------------------
void SharedMemory::sysCommand( const SystemMessage* sm )
{
	switch( sm->command )
	{
		case SystemMessage::StartUp:
		{
			if( !logserv_host.empty() && logserv_port != 0 && !logserv->isRunning() )
			{
				sminfo << myname << "(init): run log server " << logserv_host << ":" << logserv_port << endl;
				logserv->run(logserv_host, logserv_port, true);
			}

			PassiveTimer ptAct(activateTimeout);

			while( !activated && !ptAct.checkTime() )
			{
				cout << myname << "(sysCommand): wait activate..." << endl;
				msleep(100);
			}

			if( !activated  )
				smcrit << myname << "(sysCommand): ************* don`t activate?! ************" << endl;

			// подождать пока пройдёт инициализация
			// см. activateObject()
			UniSetTypes::uniset_rwmutex_rlock l(mutex_start);
			askTimer(tmHeartBeatCheck, heartbeatCheckTime);
			askTimer(tmEvent, evntPause, 1);

			if( histSaveTime > 0 && !hist.empty() )
				askTimer(tmHistory, histSaveTime);

			if( msecPulsar > 0 )
				askTimer(tmPulsar, msecPulsar);
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
bool SharedMemory::deactivateObject()
{
	workready = false;

	if( logserv && logserv->isRunning() )
		logserv->terminate();

	return IONotifyController::deactivateObject();
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

		UniSetTypes::uniset_rwmutex_wrlock l(mutex_start);
		res = IONotifyController::activateObject();

		// инициализируем указатели
		for( auto && it : hlist )
		{
			it.a_it = myioEnd();
			it.d_it = myioEnd();
		}

		itPulsar = myioEnd();

		for( auto && it : hist )
		{
			for( auto && hit : it.hlst )
				hit.ioit = myioEnd();
		}

		// здесь или в startUp?
		initFromReserv();

		activated = true;
	}

	cout << myname << ": ********** activate: " << pt.getCurrent() << " msec " << endl;
	return res;
}
// ------------------------------------------------------------------------------------------
CORBA::Boolean SharedMemory::exist()
{
	return workready;
}
// ------------------------------------------------------------------------------------------
void SharedMemory::sigterm( int signo )
{
	workready = false;

	if( signo == SIGTERM && wdt )
		wdt->stop();

	if( logserv && logserv->isRunning() )
		logserv->terminate();

	IONotifyController::sigterm(signo);
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

	bool wdtpingOK = true;

	for( auto && it : hlist )
	{
		try
		{
			long val = localGetValue(it.a_it, it.a_sid);
			val--;

			if( val < -1 )
				val = -1;

			localSetValueIt(it.a_it, it.a_sid, val, getId());

			localSetValueIt(it.d_it, it.d_sid, ( val >= 0 ? true : false), getId());

			// проверяем нужна ли "перезагрузка" по данному датчику
			if( wdt && it.ptReboot.getInterval() )
			{
				if( val > 0  )
					it.timer_running = false;
				else
				{
					if( !it.timer_running )
					{
						it.timer_running = true;
						it.ptReboot.setTiming(it.reboot_msec);
					}
					else if( it.ptReboot.checkTime() )
						wdtpingOK = false;
				}
			}
		}
		catch( const Exception& ex )
		{
			smcrit << myname << "(checkHeartBeat): " << ex << endl;
		}
		catch(...)
		{
			smcrit << myname << "(checkHeartBeat): ..." << endl;
		}
	}

	if( wdt && wdtpingOK && workready )
		wdt->ping();
}
// ------------------------------------------------------------------------------------------
bool SharedMemory::readItem( const std::shared_ptr<UniXML>& xml, UniXML::iterator& it, xmlNode* sec )
{
	for( auto && r : lstRSlot )
	{
		try
		{
			(r)(xml, it, sec);
		}
		catch(...) {}
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

		smcrit << msg.str() << endl;
		kill(getpid(), SIGTERM);
		//        throw NameNotFound(msg.str());
	};

	HeartBeatInfo hi;

	hi.a_sid = it.getIntProp("id");

	if( it.getProp("heartbeat_ds_name").empty() )
	{
		if( hi.d_sid == DefaultObjectId )
		{
			ostringstream msg;
			msg << "(SharedMemory::readItem): дискретный датчик (heartbeat_ds_name) связанный с " << it.getProp("name");
			smwarn << msg.str() << endl;
		}
	}
	else
	{
		hi.d_sid = uniset_conf()->getSensorID(it.getProp("heartbeat_ds_name"));

		if( hi.d_sid == DefaultObjectId )
		{
			ostringstream msg;
			msg << "(SharedMemory::readItem): Не найден ID для дискретного датчика (heartbeat_ds_name) связанного с " << it.getProp("name");

			// Если уж задали имя для датчика, то он должен существовать..
			// поэтому завершаем процесс, если не нашли..
			smcrit << msg.str() << endl;
			kill(getpid(), SIGTERM);
			//            throw NameNotFound(msg.str());
		}
	}

	hi.reboot_msec = it.getIntProp("heartbeat_reboot_msec");
	hi.ptReboot.setTiming(UniSetTimer::WaitUpTime);

	if( hi.a_sid <= 0 )
	{
		ostringstream msg;
		msg << "(SharedMemory::readItem): НЕ УКАЗАН id для "
			<< it.getProp("name") << " секция " << sec;

		smcrit << msg.str() << endl;
		kill(getpid(), SIGTERM);
		//        throw NameNotFound(msg.str());
	};

	// без проверки на дублирование т.к.
	// id - гарантирует уникальность в нашем configure.xml
	hlist.push_back(hi);

	return true;
}
// ------------------------------------------------------------------------------------------
shared_ptr<SharedMemory> SharedMemory::init_smemory( int argc, const char* const* argv )
{
	auto conf = uniset_conf();
	string dfile = conf->getArgParam("--datfile", conf->getConfFileName());

	// если dfile == confile, то преобразовывать имя не надо, чтобы сработала
	// оптимизация и когда NCRestorer_XML будет загружать файл, он использует conf->getUniXML()
	// т.е. не будет загружать повторно.. (см. конструктор SharedMemory и NCRestorer_XML).
	if( dfile != conf->getConfFileName() )
	{
		if( dfile[0] != '.' && dfile[0] != '/' )
			dfile = conf->getConfDir() + dfile;
	}


	dinfo << "(smemory): datfile = " << dfile << endl;
	UniSetTypes::ObjectId ID = conf->getControllerID(conf->getArgParam("--smemory-id", "SharedMemory"));

	if( ID == UniSetTypes::DefaultObjectId )
	{
		cerr << "(smemory): НЕ ЗАДАН идентификатор '"
			 << " или не найден в " << conf->getControllersSection()
			 << endl;
		return 0;
	}

	string cname = conf->getArgParam("--smemory--confnode", ORepHelpers::getShortName(conf->oind->getMapName(ID)) );
	return make_shared<SharedMemory>(ID, dfile, cname);
}
// -----------------------------------------------------------------------------
void SharedMemory::buildEventList( xmlNode* cnode )
{
	readEventList("objects");
	readEventList("controllers");
	readEventList("services");
}
// -----------------------------------------------------------------------------
void SharedMemory::readEventList( const std::string& oname )
{
	xmlNode* enode = uniset_conf()->getNode(oname);

	if( enode == NULL )
	{
		smwarn << myname << "(readEventList): " << oname << " не найден..." << endl;
		return;
	}

	UniXML::iterator it(enode);

	if( !it.goChildren() )
	{
		smwarn << myname << "(readEventList): <eventlist> пустой..." << endl;
		return;
	}

	for( ; it.getCurrent(); it.goNext() )
	{
		if( it.getProp(e_filter).empty() )
			continue;

		ObjectId oid = it.getIntProp("id");

		if( oid != 0 )
		{
			sminfo << myname << "(readEventList): add " << it.getProp("name") << endl;
			elst.push_back(oid);
		}
		else
			smcrit << myname << "(readEventList): Не найден ID для "
				   << it.getProp("name") << endl;
	}
}
// -----------------------------------------------------------------------------
void SharedMemory::sendEvent( UniSetTypes::SystemMessage& sm )
{
	TransportMessage tm(sm.transport_msg());

	for( const auto& it : elst )
	{
		bool ok = false;
		tm.consumer = it;

		for( unsigned int i = 0; i < 2; i++ )
		{
			try
			{
				ui->send(it, tm);
				ok = true;
				break;
			}
			catch(...) {};
		}

		if(!ok)
			smcrit << myname << "(sendEvent): Объект " << it << " НЕДОСТУПЕН" << endl;
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
		IONotifyController::loggingInfo(sm);
}
// -----------------------------------------------------------------------------
void SharedMemory::buildHistoryList( xmlNode* cnode )
{
	sminfo << myname << "(buildHistoryList): ..."  << endl;

	const std::shared_ptr<UniXML> xml = uniset_conf()->getConfXML();

	if( !xml )
	{
		smwarn << myname << "(buildHistoryList): xml=NULL?!" << endl;
		return;
	}

	xmlNode* n = xml->extFindNode(cnode, 1, 1, "History", "");

	if( !n )
	{
		smwarn << myname << "(buildHistoryList): <History> not found. ignore..." << endl;
		hist.clear();
		return;
	}

	UniXML::iterator it(n);

	bool no_history = uniset_conf()->getArgInt("--sm-no-history", it.getProp("no_history"));

	if( no_history )
	{
		smwarn << myname << "(buildHistoryList): no_history='1'.. history skipped..." << endl;
		hist.clear();
		return;
	}

	histSaveTime = it.getIntProp("savetime");

	if( histSaveTime <= 0 )
		histSaveTime = 0;

	if( !it.goChildren() )
	{
		smwarn << myname << "(buildHistoryList): <History> empty. ignore..." << endl;
		return;
	}

	for( ; it.getCurrent(); it.goNext() )
	{
		HistoryInfo hi;
		hi.id       = it.getIntProp("id");
		hi.size     = it.getIntProp("size");

		if( hi.size <= 0 )
			continue;

		hi.filter     = it.getProp("filter");

		if( hi.filter.empty() )
			continue;

		hi.fuse_id = uniset_conf()->getSensorID(it.getProp("fuse_id"));

		if( hi.fuse_id == DefaultObjectId )
		{
			smwarn << myname << "(buildHistory): not found sensor ID for "
				   << it.getProp("fuse_id")
				   << " history item id=" << it.getProp("id")
				   << " ..ignore.." << endl;
			continue;
		}

		hi.fuse_invert     = it.getIntProp("fuse_invert");

		hi.fuse_use_val = false;

		if( !it.getProp("fuse_value").empty() )
		{
			hi.fuse_use_val = true;
			hi.fuse_val    = it.getIntProp("fuse_value");
		}

		sminfo << myname << "(buildHistory): add fuse_id=" << hi.fuse_id
			   << " fuse_val=" << hi.fuse_val
			   << " fuse_use_val=" << hi.fuse_use_val
			   << " fuse_invert=" << hi.fuse_invert
			   << endl;

		// WARNING: no check duplicates...
		hist.push_back(hi);
	}

	sminfo << myname << "(buildHistoryList): history logs count=" << hist.size() << endl;
}
// -----------------------------------------------------------------------------
void SharedMemory::checkHistoryFilter( UniXML::iterator& xit )
{
	for( auto && it : hist )
	{
		if( xit.getProp(it.filter).empty() )
			continue;

		if( !xit.getProp("id").empty() )
		{
			HistoryItem ai(xit.getIntProp("id"), it.size, xit.getIntProp("default") );
			it.hlst.push_back( std::move(ai) );
			continue;
		}

		ObjectId id = uniset_conf()->getSensorID(xit.getProp("name"));

		if( id == DefaultObjectId )
		{
			smwarn << myname << "(checkHistoryFilter): not found sensor ID for " << xit.getProp("name") << endl;
			continue;
		}

		HistoryItem ai(id, it.size, xit.getIntProp("default") );
		it.hlst.push_back( std::move(ai) );
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

	for( auto && it : hist )
	{
		for( auto && hit : it.hlst )
		{
			try
			{
				hit.add( localGetValue( hit.ioit, hit.id ), it.size );
			}
			catch( IOController_i::Undefined& ex )
			{
				hit.add( numeric_limits<long>::max(), it.size );
			}
			catch(...) {}
		}
	}
}
// -----------------------------------------------------------------------------
void SharedMemory::updateHistory( std::shared_ptr<USensorInfo>& s_it, IOController* )
{
	if( hist.empty() )
		return;

	auto i = histmap.find(s_it->si.id);

	if( i == histmap.end() )
		return;

	long value = 0;
	long sm_tv_sec = 0;
	long sm_tv_usec = 0;
	{
		uniset_rwmutex_rlock lock(s_it->val_lock);
		value = s_it->value;
		sm_tv_sec = s_it->tv_sec;
		sm_tv_usec = s_it->tv_usec;
	}

	sminfo << myname << "(updateHistory): "
		   << " sid=" << s_it->si.id
		   << " value=" << value
		   << endl;

	for( auto && it1 : i->second )
	{
		History::iterator it = it1;

		if( s_it->type == UniversalIO::DI ||
				s_it->type == UniversalIO::DO )
		{
			bool st = (bool)value;

			if( it->fuse_invert )
				st ^= true;

			if( st )
			{
				sminfo << myname << "(updateHistory): HISTORY EVENT for " << (*it) << endl;

				it->fuse_sec = sm_tv_sec;
				it->fuse_usec = sm_tv_usec;
				m_historySignal.emit( (*it) );
			}
		}
		else if( s_it->type == UniversalIO::AI ||
				 s_it->type == UniversalIO::AO )
		{
			if( !it->fuse_use_val )
			{
				bool st = (bool)value;

				if( it->fuse_invert )
					st ^= true;

				if( !st )
				{
					sminfo << myname << "(updateHistory): HISTORY EVENT for " << (*it) << endl;

					it->fuse_sec = sm_tv_sec;
					it->fuse_usec = sm_tv_usec;
					m_historySignal.emit( (*it) );
				}
			}
			else
			{
				if( value == it->fuse_val )
				{
					sminfo << myname << "(updateHistory): HISTORY EVENT for " << (*it) << endl;

					it->fuse_sec = sm_tv_sec;
					it->fuse_usec = sm_tv_usec;
					m_historySignal.emit( (*it) );
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

	for( SharedMemory::HistoryList::const_iterator it = h.hlst.begin(); it != h.hlst.end(); ++it )
	{
		os << "    id=" << it->id << "[";

		for( SharedMemory::HBuffer::const_iterator i = it->buf.begin(); i != it->buf.end(); ++i )
			os << " " << (*i);

		os << " ]" << endl;
	}

	return os;
}
// ----------------------------------------------------------------------------
void SharedMemory::initFromReserv()
{
	UniXML::iterator it(confnode);

	if( !it.find("ReservList") )
	{
		sminfo << myname << "(initFromReserv): <ReservList> not found... ignore.. " << endl;
		return;
	}

	if( !it.goChildren() )
	{
		smwarn << myname << "(initFromReserv): <ReservList> EMPTY?... ignore.. " << endl;
		return;
	}

	auto conf = uniset_conf();

	for( ; it.getCurrent(); it++ )
	{
		ObjectId sm_id = DefaultObjectId;
		ObjectId sm_node = DefaultObjectId;

		std::string smName(it.getProp("name"));

		if( !smName.empty() )
			sm_id = conf->getControllerID(smName);
		else
			sm_id = getId();

		if( sm_id == DefaultObjectId )
		{
			ostringstream err;
			err << myname << "(initFromReserv): Not found ID for '" << smName << "'";
			smcrit << err.str() << endl;
			// throw SystemError(err.str());
			raise(SIGTERM);
		}

		std::string smNode(it.getProp("node"));

		if( !smNode.empty() )
			sm_node = conf->getNodeID(smNode);
		else
			sm_node = conf->getLocalNode();

		if( sm_node == DefaultObjectId )
		{
			ostringstream err;
			err << myname << "(initFromReserv): Not found NodeID for '" << smNode << "'";
			smcrit << err.str() << endl;
			// throw SystemError(err.str());
			raise(SIGTERM);
		}


		if( sm_id == getId() && sm_node == conf->getLocalNode() )
		{
			smcrit << myname << "(initFromReserv): Initialization of himself?!  ignore.." << endl;
			continue;
		}

		if( initFromSM(sm_id, sm_node) )
		{
			sminfo << myname << "(initFromReserv): init from sm_id='" << smName << "' sm_node='" << smNode << "' [OK]" << endl;
			return;
		}

		sminfo << myname << "(initFromReserv): init from sm_id='" << smName << "' sm_node='" << smNode << "' [FAILED]" << endl;
	}

	smwarn << myname << "(initFromReserv): FAILED INIT FROM <ReservList>" << endl;
}
// ----------------------------------------------------------------------------
bool SharedMemory::initFromSM( UniSetTypes::ObjectId sm_id, UniSetTypes::ObjectId sm_node )
{
	sminfo << myname << "(initFromSM): init from sm_id='" << sm_id << "' sm_node='" << sm_node << "'" << endl;

	// SENSORS MAP
	try
	{
		IOController_i::SensorInfoSeq_var amap = ui->getSensorsMap(sm_id, sm_node);
		int size = amap->length();

		for( int i = 0; i < size; i++ )
		{
			IOController_i::SensorIOInfo& ii(amap[i]);

			try
			{
#if 1
				// Вариант через setValue...(заодно внтури проверяются пороги)
				setValue(ii.si.id, ii.value, getId());
#else

				// Вариант с прямым обновлением внутреннего состояния
				IOStateList::iterator io = myiofind(ii.si.id);

				if( io == myioEnd() )
				{
					smcrit << myname << "(initFromSM): not found sensor id=" << ii.si.id << "'" << endl;
					continue;
				}

				io->second->init(ii);

				// проверка порогов
				try
				{
					checkThreshold(io, ii.si.id, true);
				}
				catch(...) {}

#endif
			}
			catch( const Exception& ex )
			{
				smcrit << myname << "(initFromSM): " << ex << endl;
			}
			catch( const IOController_i::NameNotFound& ex )
			{
				smcrit << myname << "(initFromSM): not found sensor id=" << ii.si.id << "'" << endl;
			}
		}

		return true;
	}
	catch( const UniSetTypes::Exception& ex )
	{
		smwarn << myname << "(initFromSM): " << ex << endl;
	}

	return false;
}
// ----------------------------------------------------------------------------
UniSetTypes::SimpleInfo* SharedMemory::getInfo( CORBA::Long userparam )
{
	UniSetTypes::SimpleInfo_var i = IONotifyController::getInfo(userparam);

	ostringstream inf;

	inf << i->info << endl;
	inf << vmon.pretty_str() << endl;
	inf << "LogServer:  " << logserv_host << ":" << logserv_port << endl;

	i->info = inf.str().c_str();
	return i._retn();
}
// ----------------------------------------------------------------------------
