// -----------------------------------------------------------------------------
#include <cmath>
#include <limits>
#include <sstream>
#include <Exceptions.h>
#include <extensions/Extensions.h>
#include "MBTCPMultiMaster.h"
#include "modbus/MBLogSugar.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// -----------------------------------------------------------------------------
MBTCPMultiMaster::MBTCPMultiMaster( UniSetTypes::ObjectId objId, UniSetTypes::ObjectId shmId,
									const std::shared_ptr<SharedMemory> ic, const std::string& prefix ):
	MBExchange(objId, shmId, ic, prefix),
	force_disconnect(true)
{
	tcpMutex.setName(myname + "_tcpMutex");

	if( objId == DefaultObjectId )
		throw UniSetTypes::SystemError("(MBTCPMultiMaster): objId=-1?!! Use --" + prefix + "-name" );

	auto conf = uniset_conf();

	prop_prefix = initPropPrefix("tcp_");
	mbinfo << myname << "(init): prop_prefix=" << prop_prefix << endl;

	UniXML::iterator it(cnode);

	checktime = conf->getArgPInt("--" + prefix + "-checktime", it.getProp("checktime"), 5000);
	force_disconnect = conf->getArgInt("--" + prefix + "-persistent-connection", it.getProp("persistent_connection")) ? false : true;

	int ignore_timeout = conf->getArgPInt("--" + prefix + "-ignore-timeout", it.getProp("ignoreTimeout"), ptReopen.getInterval());

	UniXML::iterator it1(it);

	if( !it1.find("GateList") )
	{
		ostringstream err;
		err << myname << "(init): not found <GateList>";
		mbcrit << err.str() << endl;
		throw UniSetTypes::SystemError(err.str());
	}

	if( !it1.goChildren() )
	{
		ostringstream err;
		err << myname << "(init): empty <GateList> ?!";
		mbcrit << err.str() << endl;
		throw UniSetTypes::SystemError(err.str());
	}

	for( ; it1.getCurrent(); it1++ )
	{
		MBSlaveInfo sinf;
		sinf.ip = it1.getProp("ip");

		if( sinf.ip.empty() )
		{
			ostringstream err;
			err << myname << "(init): ip='' in <GateList>";
			mbcrit << err.str() << endl;
			throw UniSetTypes::SystemError(err.str());
		}

		sinf.port = it1.getIntProp("port");

		if( sinf.port <= 0 )
		{
			ostringstream err;
			err << myname << "(init): ERROR: port=''" << sinf.port << " for ip='" << sinf.ip << "' in <GateList>";
			mbcrit << err.str() << endl;
			throw UniSetTypes::SystemError(err.str());
		}

		if( !it1.getProp("respondSensor").empty() )
		{
			sinf.respond_id = conf->getSensorID( it1.getProp("respondSensor") );

			if( sinf.respond_id == DefaultObjectId )
			{
				ostringstream err;
				err << myname << "(init): ERROR: Unknown SensorID for '" << it1.getProp("respondSensor") << "' in <GateList>";
				mbcrit << err.str() << endl;
				throw UniSetTypes::SystemError(err.str());
			}
		}

		sinf.priority = it1.getIntProp("priority");
		sinf.mbtcp = std::make_shared<ModbusTCPMaster>();

		sinf.ptIgnoreTimeout.setTiming( it1.getPIntProp("ignore_timeout", ignore_timeout) );
		sinf.recv_timeout = it1.getPIntProp("recv_timeout", recv_timeout);
		sinf.aftersend_pause = it1.getPIntProp("aftersend_pause", aftersend_pause);
		sinf.sleepPause_usec = it1.getPIntProp("sleepPause_usec", sleepPause_usec);
		sinf.respond_invert = it1.getPIntProp("invert", 0);
		sinf.respond_force = it1.getPIntProp("force", 0);

		sinf.force_disconnect = it.getPIntProp("persistent_connection", !force_disconnect) ? false : true;

		ostringstream n;
		n << sinf.ip << ":" << sinf.port;
		sinf.myname = n.str();

		auto l = loga->create(sinf.myname);
		sinf.mbtcp->setLog(l);

		mblist.push_back(sinf);
		mbinfo << myname << "(init): add slave channel " << sinf.myname << endl;
	}

	if( ic )
		ic->logAgregator()->add(loga);


	if( mblist.empty() )
	{
		ostringstream err;
		err << myname << "(init): empty <GateList>!";
		mbcrit << err.str() << endl;
		throw UniSetTypes::SystemError(err.str());
	}

	mblist.sort();
	mbi = mblist.rbegin();

	if( shm->isLocalwork() )
	{
		readConfiguration();
		rtuQueryOptimization(rmap);
		initDeviceList();
	}
	else
		ic->addReadItem( sigc::mem_fun(this, &MBTCPMultiMaster::readItem) );

	pollThread = make_shared<ThreadCreator<MBTCPMultiMaster>>(this, &MBTCPMultiMaster::poll_thread);
	pollThread->setFinalAction(this, &MBTCPMultiMaster::final_thread);
	checkThread = make_shared<ThreadCreator<MBTCPMultiMaster>>(this, &MBTCPMultiMaster::check_thread);
	checkThread->setFinalAction(this, &MBTCPMultiMaster::final_thread);


	// Т.к. при "многоканальном" доступе к slave, смена канала должна происходит сразу после
	// неудачной попытки запросов по одному из каналов, то ПЕРЕОПРЕДЕЛЯЕМ reopen, на timeout..
	ptReopen.setTiming(default_timeout);

	if( mblog->is_info() )
		printMap(rmap);
}
// -----------------------------------------------------------------------------
MBTCPMultiMaster::~MBTCPMultiMaster()
{
	if( pollThread )
	{
		pollThread->stop();

		if( pollThread->isRunning() )
			pollThread->join();
	}

	if( checkThread )
	{
		checkThread->stop();

		if( checkThread->isRunning() )
			checkThread->join();
	}

	mbi = mblist.rend();
}
// -----------------------------------------------------------------------------
std::shared_ptr<ModbusClient> MBTCPMultiMaster::initMB( bool reopen )
{
	// просто движемся по кругу (т.к. связь не проверяется)
	// движемся в обратном порядке, т.к. сортировка по возрастанию приоритета
	if( checktime <= 0 )
	{
		++mbi;

		if( mbi == mblist.rend() )
			mbi = mblist.rbegin();

		mbi->init(mblog);
		mb = mbi->mbtcp;
		return mbi->mbtcp;
	}

	{
		uniset_rwmutex_wrlock l(tcpMutex);

		// если reopen=true - значит почему текущему каналу нет (хотя соединение есть)
		// тогда выставляем ему признак игнорирования
		if( mbi != mblist.rend() && reopen )
		{
			mbi->setUse(false);
			mbi->ignore = true;
			mbi->ptIgnoreTimeout.reset();
			mbwarn << myname << "(initMB): set ignore=true for " << mbi->ip << ":" << mbi->port << endl;
		}

		// Если по текущему каналу связь есть, то возвращаем его
		if( mbi != mblist.rend() && !mbi->ignore && mbi->respond )
		{
			if( mbi->mbtcp->isConnection() || ( !mbi->mbtcp->isConnection() && mbi->init(mblog)) )
			{
				if( !mbi->ignore  )
				{
					mb = mbi->mbtcp;
					mbi->setUse(true);
					return mbi->mbtcp;
				}

				if( mbi->ptIgnoreTimeout.checkTime() )
				{
					mbi->ignore = false;
					mbi->setUse(true);
					mb = mbi->mbtcp;
					return mbi->mbtcp;
				}

				mbi->setUse(false);
			}
		}

		if( mbi != mblist.rend() )
			mbi->mbtcp->disconnect();
	}

	// проходим по списку (в обратном порядке, т.к. самый приоритетный в конце)
	for( auto it = mblist.rbegin(); it != mblist.rend(); ++it )
	{
		uniset_rwmutex_wrlock l(tcpMutex);

		if( it->respond && it->init(mblog) )
		{
			if( it->ignore )
			{
				if( !it->ptIgnoreTimeout.checkTime() )
					continue;

				it->ignore = false;
			}

			mbi = it;
			mb = mbi->mbtcp;
			mbi->setUse(true);
			return it->mbtcp;
		}
	}

	{
		uniset_rwmutex_wrlock l(tcpMutex);
		mbi = mblist.rend();
		mb = nullptr;
	}

	return 0;
}
// -----------------------------------------------------------------------------
void MBTCPMultiMaster::final_thread()
{
	setProcActive(false);
}
// -----------------------------------------------------------------------------

bool MBTCPMultiMaster::MBSlaveInfo::check()
{
	return mbtcp->checkConnection(ip, port, recv_timeout);
}
// -----------------------------------------------------------------------------
bool MBTCPMultiMaster::MBSlaveInfo::init( std::shared_ptr<DebugStream>& mblog )
{
	try
	{
		ost::Thread::setException(ost::Thread::throwException);

		mbinfo << myname << "(init): connect..." << endl;

		mbtcp->connect(ip, port);
		mbtcp->setForceDisconnect(force_disconnect);

		if( recv_timeout > 0 )
			mbtcp->setTimeout(recv_timeout);

		// if( !initOK )
		{
			mbtcp->setSleepPause(sleepPause_usec);
			mbtcp->setAfterSendPause(aftersend_pause);

			if( mbtcp->isConnection() )
				mbinfo << "(init): " << myname << " connect OK" << endl;

			initOK = true;
		}
		return mbtcp->isConnection();
	}
	catch( ModbusRTU::mbException& ex )
	{
		mbwarn << "(init): " << ex << endl;
	}
	catch( const ost::Exception& e )
	{
		mbwarn << myname << "(init): Can`t create socket " << ip << ":" << port << " err: " << e.getString() << endl;
	}
	catch(...)
	{
		mbwarn << "(init): " << myname << " catch ..." << endl;
	}

	initOK = false;
	return false;
}
// -----------------------------------------------------------------------------
void MBTCPMultiMaster::sysCommand( const UniSetTypes::SystemMessage* sm )
{
	MBExchange::sysCommand(sm);

	if( sm->command == SystemMessage::StartUp )
	{
		pollThread->start();

		if( checktime > 0 )
			checkThread->start();
	}
}
// -----------------------------------------------------------------------------
void MBTCPMultiMaster::poll_thread()
{
	// ждём начала работы..(см. MBExchange::activateObject)
	while( !checkProcActive() )
	{
		UniSetTypes::uniset_rwmutex_rlock l(mutex_start);
	}

	// работаем..
	while( checkProcActive() )
	{
		try
		{
			if( sidExchangeMode != DefaultObjectId && force )
				exchangeMode = shm->localGetValue(itExchangeMode, sidExchangeMode);
		}
		catch(...) {}

		try
		{
			poll();
		}
		catch(...) {}

		if( !checkProcActive() )
			break;

		msleep(polltime);
	}
}
// -----------------------------------------------------------------------------
void MBTCPMultiMaster::check_thread()
{
	while( checkProcActive() )
	{
		for( auto it = mblist.begin(); it != mblist.end(); ++it )
		{
			try
			{
				if( it->use ) // игнорируем текущий mbtcp
					continue;

				bool r = it->check();
				mbinfo << myname << "(check): " << it->myname << " " << ( r ? "OK" : "FAIL" ) << endl;

				try
				{
					if( it->respond_id != DefaultObjectId && (it->respond_force || !it->respond_init || r != it->respond) )
					{
						bool set = it->respond_invert ? !it->respond : it->respond;
						shm->localSetValue(it->respond_it, it->respond_id, (set ? 1 : 0), getId());
						it->respond_init = true;
					}
				}
				catch( const Exception& ex )
				{
					mbcrit << myname << "(check): (respond) " << ex << std::endl;
				}
				catch( const std::exception& ex )
				{
					mbcrit << myname << "(check): (respond) " << ex.what() << std::endl;
				}


				{
					uniset_rwmutex_wrlock l(tcpMutex);
					it->respond = r;
				}
			}
			catch( const std::exception& ex )
			{
				mbcrit << myname << "(check): (respond) " << ex.what() << std::endl;
			}

			if( !checkProcActive() )
				break;
		}

		if( !checkProcActive() )
			break;

		msleep(checktime);
	}
}
// -----------------------------------------------------------------------------
void MBTCPMultiMaster::initIterators()
{
	MBExchange::initIterators();

	for( auto& it : mblist )
		shm->initIterator(it.respond_it);
}
// -----------------------------------------------------------------------------
void MBTCPMultiMaster::sigterm( int signo )
{
	if( pollThread )
	{
		pollThread->stop();

		if( pollThread->isRunning() )
			pollThread->join();
	}

	if( checkThread )
	{
		checkThread->stop();

		if( checkThread->isRunning() )
			checkThread->join();
	}

	try
	{
		MBExchange::sigterm(signo);
	}
	catch( const std::exception& ex )
	{
		cerr << "catch: " << ex.what() << endl;
	}
	catch( ... )
	{
		std::exception_ptr p = std::current_exception();
		std::clog << (p ? p.__cxa_exception_type()->name() : "null") << std::endl;
	}
}

// -----------------------------------------------------------------------------
void MBTCPMultiMaster::help_print( int argc, const char* const* argv )
{
	cout << "Default: prefix='mbtcp'" << endl;
	MBExchange::help_print(argc, argv);
	cout << endl;
	cout << " Настройки протокола TCP(MultiMaster): " << endl;
	cout << "--prefix-persistent-connection 0,1 - Не закрывать соединение на каждом цикле опроса" << endl;
	cout << "--prefix-checktime                 - период проверки связи по каналам (<GateList>)" << endl;
	cout << "--prefix-ignore-timeout            - Timeout на повторную попытку использования канала после 'reopen-timeout'. По умолчанию: reopen-timeout * 3" << endl;
	cout << endl;
	cout << " ВНИМАНИЕ! '--prefix-reopen-timeout' для MBTCPMultiMaster НЕ ДЕЙСТВУЕТ! " << endl;
	cout << " Смена канала происходит по --prefix-timeout. " << endl;
	cout << " При этом следует учитывать блокировку отключаемого канала на время: --prefix-ignore-timeout" << endl;
}
// -----------------------------------------------------------------------------
std::shared_ptr<MBTCPMultiMaster> MBTCPMultiMaster::init_mbmaster( int argc, const char* const* argv,
		UniSetTypes::ObjectId icID, const std::shared_ptr<SharedMemory> ic,
		const std::string& prefix )
{
	auto conf = uniset_conf();

	string name = conf->getArgParam("--" + prefix + "-name", "MBTCPMultiMaster1");

	if( name.empty() )
	{
		dcrit << "(MBTCPMultiMaster): Не задан name'" << endl;
		return 0;
	}

	ObjectId ID = conf->getObjectID(name);

	if( ID == UniSetTypes::DefaultObjectId )
	{
		dcrit << "(MBTCPMultiMaster): идентификатор '" << name
			  << "' не найден в конф. файле!"
			  << " в секции " << conf->getObjectsSection() << endl;
		return 0;
	}

	dinfo << "(MBTCPMultiMaster): name = " << name << "(" << ID << ")" << endl;
	return make_shared<MBTCPMultiMaster>(ID, icID, ic, prefix);
}
// -----------------------------------------------------------------------------
const std::string MBTCPMultiMaster::MBSlaveInfo::getShortInfo() const
{
	ostringstream s;
	s << myname << " respond=" << respond;
	return std::move(s.str());
}
// -----------------------------------------------------------------------------
UniSetTypes::SimpleInfo* MBTCPMultiMaster::getInfo()
{
	UniSetTypes::SimpleInfo_var i = MBExchange::getInfo();

	ostringstream inf;

	inf << i->info << endl;
	inf << "Gates: " << endl;

	for( const auto& m : mblist )
		inf << "   " << m.getShortInfo() << endl;

	inf << endl;

	i->info = inf.str().c_str();
	return i._retn();
}
// ----------------------------------------------------------------------------

