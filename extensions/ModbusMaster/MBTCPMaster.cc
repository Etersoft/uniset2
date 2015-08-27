// -----------------------------------------------------------------------------
#include <cmath>
#include <limits>
#include <sstream>
#include <Exceptions.h>
#include <extensions/Extensions.h>
#include "MBTCPMaster.h"
#include "modbus/MBLogSugar.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// -----------------------------------------------------------------------------
MBTCPMaster::MBTCPMaster( UniSetTypes::ObjectId objId, UniSetTypes::ObjectId shmId,
						  const std::shared_ptr<SharedMemory> ic, const std::string& prefix ):
	MBExchange(objId, shmId, ic, prefix),
	force_disconnect(true)
{
	if( objId == DefaultObjectId )
		throw UniSetTypes::SystemError("(MBTCPMaster): objId=-1?!! Use --" + prefix + "-name" );

	auto conf = uniset_conf();

	// префикс для "свойств" - по умолчанию "tcp_";
	prop_prefix = initPropPrefix("tcp_");
	mbinfo << myname << "(init): prop_prefix=" << prop_prefix << endl;

	UniXML::iterator it(cnode);

	// ---------- init MBTCP ----------
	string pname("--" + prefix + "-gateway-iaddr");
	iaddr    = conf->getArgParam(pname, it.getProp("gateway_iaddr"));

	if( iaddr.empty() )
		throw UniSetTypes::SystemError(myname + "(MBMaster): Unknown inet addr...(Use: " + pname + ")" );

	string tmp("--" + prefix + "-gateway-port");
	port = conf->getArgInt(tmp, it.getProp("gateway_port"));

	if( port <= 0 )
		throw UniSetTypes::SystemError(myname + "(MBMaster): Unknown inet port...(Use: " + tmp + ")" );

	mbinfo << myname << "(init): gateway " << iaddr << ":" << port << endl;

	force_disconnect = conf->getArgInt("--" + prefix + "-persistent-connection", it.getProp("persistent_connection")) ? false : true;
	mbinfo << myname << "(init): persisten-connection=" << (!force_disconnect) << endl;

	if( shm->isLocalwork() )
	{
		readConfiguration();
		rtuQueryOptimization(rmap);
		initDeviceList();
	}
	else
		ic->addReadItem( sigc::mem_fun(this, &MBTCPMaster::readItem) );

	pollThread = make_shared<ThreadCreator<MBTCPMaster>>(this, &MBTCPMaster::poll_thread);
	pollThread->setFinalAction(this, &MBTCPMaster::final_thread);

	if( mblog->is_info() )
		printMap(rmap);
}
// -----------------------------------------------------------------------------
MBTCPMaster::~MBTCPMaster()
{
	if( pollThread )
	{
		pollThread->stop();

		if( pollThread->isRunning() )
			pollThread->join();
	}
}
// -----------------------------------------------------------------------------
std::shared_ptr<ModbusClient> MBTCPMaster::initMB( bool reopen )
{
	if( mbtcp )
	{
		if( !reopen )
			return mbtcp;

		mbtcp.reset();
		mb.reset();
		ptInitChannel.reset();
	}

	try
	{
		ost::Thread::setException(ost::Thread::throwException);
		mbtcp = std::make_shared<ModbusTCPMaster>();

		ost::InetAddress ia(iaddr.c_str());
		mbtcp->connect(ia, port);
		mbtcp->setForceDisconnect(force_disconnect);

		if( recv_timeout > 0 )
			mbtcp->setTimeout(recv_timeout);

		mbtcp->setSleepPause(sleepPause_usec);
		mbtcp->setAfterSendPause(aftersend_pause);

		mbinfo << myname << "(init): ipaddr=" << iaddr << " port=" << port << endl;

		auto l = loga->create(myname + "-exchangelog");
		mbtcp->setLog(l);

		if( ic )
			ic->logAgregator()->add(loga);
	}
	catch( ModbusRTU::mbException& ex )
	{
		mbwarn << "(init): " << ex << endl;
		mb = nullptr;
		mbtcp = nullptr;
	}
	catch( const ost::Exception& e )
	{
		mbwarn << myname << "(init): Can`t create socket " << iaddr << ":" << port << " err: " << e.getString() << endl;
		mb = nullptr;
		mbtcp = nullptr;
	}
	catch(...)
	{
		mb = nullptr;
		mbtcp = nullptr;
	}

	mb = mbtcp;
	return mbtcp;
}
// -----------------------------------------------------------------------------
void MBTCPMaster::sysCommand( const UniSetTypes::SystemMessage* sm )
{
	MBExchange::sysCommand(sm);

	if( sm->command == SystemMessage::StartUp )
		pollThread->start();
}
// -----------------------------------------------------------------------------
void MBTCPMaster::final_thread()
{
	setProcActive(false);
}
// -----------------------------------------------------------------------------
void MBTCPMaster::poll_thread()
{
	// ждём начала работы..(см. MBExchange::activateObject)
	while( !checkProcActive() )
	{
		UniSetTypes::uniset_rwmutex_rlock l(mutex_start);
	}

	// работаем
	while( checkProcActive() )
	{
		try
		{
			if( sidExchangeMode != DefaultObjectId && force )
				exchangeMode = shm->localGetValue(itExchangeMode, sidExchangeMode);
		}
		catch(...)
		{
			throw;
		}

		try
		{
			poll();
		}
		catch(...)
		{
			//            if( !checkProcActive() )
			throw;
		}

		if( !checkProcActive() )
			break;

		msleep(polltime);
	}
}
// -----------------------------------------------------------------------------
void MBTCPMaster::sigterm( int signo )
{
	setProcActive(false);

	if( pollThread )
	{
		pollThread->stop();

		if( pollThread->isRunning() )
			pollThread->join();
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
void MBTCPMaster::help_print( int argc, const char* const* argv )
{
	cout << "Default: prefix='mbtcp'" << endl;
	MBExchange::help_print(argc, argv);
	cout << endl;
	cout << " Настройки протокола TCP: " << endl;
	cout << "--prefix-gateway-iaddr hostname,IP     - IP опрашиваемого узла" << endl;
	cout << "--prefix-gateway-port num              - port на опрашиваемом узле" << endl;
	cout << "--prefix-persistent-connection 0,1     - Не закрывать соединение на каждом цикле опроса" << endl;
}
// -----------------------------------------------------------------------------
std::shared_ptr<MBTCPMaster> MBTCPMaster::init_mbmaster( int argc, const char* const* argv,
		UniSetTypes::ObjectId icID, const std::shared_ptr<SharedMemory> ic,
		const std::string& prefix )
{
	auto conf = uniset_conf();
	string name = conf->getArgParam("--" + prefix + "-name", "MBTCPMaster1");

	if( name.empty() )
	{
		dcrit << "(MBTCPMaster): Не задан name'" << endl;
		return 0;
	}

	ObjectId ID = conf->getObjectID(name);

	if( ID == UniSetTypes::DefaultObjectId )
	{
		dcrit << "(MBTCPMaster): идентификатор '" << name
			  << "' не найден в конф. файле!"
			  << " в секции " << conf->getObjectsSection() << endl;
		return 0;
	}

	dinfo << "(MBTCPMaster): name = " << name << "(" << ID << ")" << endl;
	return make_shared<MBTCPMaster>(ID, icID, ic, prefix);
}
// -----------------------------------------------------------------------------
UniSetTypes::SimpleInfo* MBTCPMaster::getInfo()
{
	UniSetTypes::SimpleInfo_var i = MBExchange::getInfo();

	ostringstream inf;

	inf << i->info << endl;
	inf << "poll: " << iaddr << ":" << port << " pesrsistent-connection=" << ( force_disconnect ? "NO" : "YES" ) << endl;

	i->info = inf.str().c_str();
	return i._retn();
}
// ----------------------------------------------------------------------------
