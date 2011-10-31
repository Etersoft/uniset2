// -----------------------------------------------------------------------------
#include <cmath>
#include <limits>
#include <sstream>
#include <Exceptions.h>
#include <extensions/Extensions.h>
#include "MBTCPMaster.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// -----------------------------------------------------------------------------
MBTCPMaster::MBTCPMaster( UniSetTypes::ObjectId objId, UniSetTypes::ObjectId shmId,
							SharedMemory* ic, const std::string prefix ):
MBExchange(objId,shmId,ic,prefix),
force_disconnect(true),
mbtcp(0),
pollThread(0)
{
//	cout << "$ $" << endl;
	prop_prefix = "tcp_";

	if( objId == DefaultObjectId )
		throw UniSetTypes::SystemError("(MBTCPMaster): objId=-1?!! Use --" + prefix + "-name" );

	UniXML_iterator it(cnode);

	// ---------- init MBTCP ----------
	string pname("--" + prefix + "-gateway-iaddr");
	iaddr	= conf->getArgParam(pname,it.getProp("gateway_iaddr"));
	if( iaddr.empty() )
		throw UniSetTypes::SystemError(myname+"(MBMaster): Unknown inet addr...(Use: " + pname +")" );

	string tmp("--" + prefix + "-gateway-port");
	port = conf->getArgInt(tmp,it.getProp("gateway_port"));
	if( port <= 0 )
		throw UniSetTypes::SystemError(myname+"(MBMaster): Unknown inet port...(Use: " + tmp +")" );


	force_disconnect = conf->getArgInt("--" + prefix + "-persistent-connection",it.getProp("persistent_connection")) ? false : true;
	dlog[Debug::INFO] << myname << "(init): persisten-connection=" << (!force_disconnect) << endl;

	recv_timeout = conf->getArgPInt("--" + prefix + "-recv-timeout",it.getProp("recv_timeout"), 500);

	int tout = conf->getArgPInt("--" + prefix + "-timeout",it.getProp("timeout"), 5000);
	ptTimeout.setTiming(tout);

	if( shm->isLocalwork() )
	{
		readConfiguration();
		rtuQueryOptimization(rmap);
		initDeviceList();
	}
	else
		ic->addReadItem( sigc::mem_fun(this,&MBTCPMaster::readItem) );

	pollThread = new ThreadCreator<MBTCPMaster>(this, &MBTCPMaster::poll_thread);
}
// -----------------------------------------------------------------------------
MBTCPMaster::~MBTCPMaster()
{
	delete pollThread;
	delete mbtcp;
}
// -----------------------------------------------------------------------------
ModbusClient* MBTCPMaster::initMB( bool reopen )
{
	if( mbtcp )
	{
		if( !reopen )
			return mbtcp;

		delete mbtcp;
		mb = 0;
		mbtcp = 0;
	}

	try
	{
		ost::Thread::setException(ost::Thread::throwException);
		mbtcp = new ModbusTCPMaster();

		ost::InetAddress ia(iaddr.c_str());
		mbtcp->connect(ia,port);
		mbtcp->setForceDisconnect(force_disconnect);

		if( recv_timeout > 0 )
			mbtcp->setTimeout(recv_timeout);

		mbtcp->setSleepPause(sleepPause_usec);

		dlog[Debug::INFO] << myname << "(init): ipaddr=" << iaddr << " port=" << port << endl;

		if( dlog.debugging(Debug::LEVEL9) )
			mbtcp->setLog(dlog);
	}
	catch( ModbusRTU::mbException& ex )
	{
		dlog[Debug::WARN] << "(init): " << ex << endl;
	}
	catch(...)
	{
		if( mbtcp )
			delete mbtcp;
		mb = 0;
		mbtcp = 0;
	}

	mb = mbtcp;
	return mbtcp;
}
// -----------------------------------------------------------------------------
void MBTCPMaster::sysCommand( UniSetTypes::SystemMessage *sm )
{
	MBExchange::sysCommand(sm);
	if( sm->command == SystemMessage::StartUp )
		pollThread->start();
}
// -----------------------------------------------------------------------------
void MBTCPMaster::poll_thread()
{
	{
		uniset_mutex_lock l(pollMutex,300);
		ptTimeout.reset();
	}

	while( checkProcActive() )
	{
		try
		{
			if( sidExchangeMode != DefaultObjectId && force_out )
				exchangeMode = shm->localGetValue(aitExchangeMode,sidExchangeMode);
		}
		catch(...){}
		try
		{
			poll();
		}
		catch(...){}

		if( !checkProcActive() )
			break;

		msleep(polltime);
	}
}
// -----------------------------------------------------------------------------
void MBTCPMaster::help_print( int argc, const char* const* argv )
{
	cout << "Default: prefix='mbtcp'" << endl;
	MBExchange::help_print(argc,argv);
	// ---------- init MBTCP ----------
//	cout << "--prefix-sm-ready-timeout - время на ожидание старта SM" << endl;
	cout << " Настройки протокола TCP: " << endl;
	cout << "--prefix-gateway hostname,IP           - IP опрашиваемого узла" << endl;
	cout << "--prefix-gateway-port num              - port на опрашиваемом узле" << endl;
	cout << "--prefix-recv-timeout msec             - Таймаут на приём одного сообщения" << endl;
	cout << "--prefix-timeout msec                  - Таймаут для определения отсутсвия соединения" << endl;
	cout << "--prefix-persistent-connection 0,1     - Не закрывать соединение на каждом цикле опроса" << endl;
}
// -----------------------------------------------------------------------------
MBTCPMaster* MBTCPMaster::init_mbmaster( int argc, const char* const* argv,
											UniSetTypes::ObjectId icID, SharedMemory* ic,
											const std::string prefix )
{
	string name = conf->getArgParam("--" + prefix + "-name","MBTCPMaster1");
	if( name.empty() )
	{
		dlog[Debug::CRIT] << "(MBTCPMaster): Не задан name'" << endl;
		return 0;
	}

	ObjectId ID = conf->getObjectID(name);
	if( ID == UniSetTypes::DefaultObjectId )
	{
		dlog[Debug::CRIT] << "(MBTCPMaster): идентификатор '" << name
			<< "' не найден в конф. файле!"
			<< " в секции " << conf->getObjectsSection() << endl;
		return 0;
	}

	dlog[Debug::INFO] << "(MBTCPMaster): name = " << name << "(" << ID << ")" << endl;
	return new MBTCPMaster(ID,icID,ic,prefix);
}
// -----------------------------------------------------------------------------
