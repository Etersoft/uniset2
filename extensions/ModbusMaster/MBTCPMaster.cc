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
	if( objId == DefaultObjectId )
		throw UniSetTypes::SystemError("(MBTCPMaster): objId=-1?!! Use --" + prefix + "-name" );

	// префикс для "свойств" - по умолчанию
	prop_prefix = "tcp_";
	// если задано поле для "фильтрации"
	// то в качестве префикса используем его
	if( !s_field.empty() )
		prop_prefix = s_field + "_";
	// если "принудительно" задан префикс
	// используем его.
	{
		string p("--" + prefix + "-set-prop-prefix");
		string v = conf->getArgParam(p,"");
		if( !v.empty() && v[0] != '-' )
			prop_prefix = v;
		// если параметр всё-таки указан, считаем, что это попытка задать "пустой" префикс
		else if( findArgParam(p,conf->getArgc(),conf->getArgv()) != -1 )
			prop_prefix = "";
	}

	dlog.info() << myname << "(init): prop_prefix=" << prop_prefix << endl;

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
	dlog.info() << myname << "(init): persisten-connection=" << (!force_disconnect) << endl;

	if( shm->isLocalwork() )
	{
		readConfiguration();
		rtuQueryOptimization(rmap);
		initDeviceList();
	}
	else
		ic->addReadItem( sigc::mem_fun(this,&MBTCPMaster::readItem) );

	pollThread = new ThreadCreator<MBTCPMaster>(this, &MBTCPMaster::poll_thread);

	if( dlog.is_info() )
		printMap(rmap);
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

		mbtcp->setAfterSendPause(aftersend_pause);

		if( dlog.is_info() )
			dlog.info() << myname << "(init): ipaddr=" << iaddr << " port=" << port << endl;
		
		if( dlog.debugging(Debug::LEVEL9) )
			mbtcp->setLog(dlog);
	}
	catch( ModbusRTU::mbException& ex )
	{
		if( dlog.is_warn() )
			dlog.warn() << "(init): " << ex << endl;
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
			if( sidExchangeMode != DefaultObjectId && force )
				exchangeMode = shm->localGetValue(itExchangeMode,sidExchangeMode);
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
	cout << endl;
	cout << " Настройки протокола TCP: " << endl;
	cout << "--prefix-gateway-iaddr hostname,IP     - IP опрашиваемого узла" << endl;
	cout << "--prefix-gateway-port num              - port на опрашиваемом узле" << endl;
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
		if( dlog.debugging(Debug::CRIT) )
			dlog.crit() << "(MBTCPMaster): Не задан name'" << endl;
		return 0;
	}

	ObjectId ID = conf->getObjectID(name);
	if( ID == UniSetTypes::DefaultObjectId )
	{
		if( dlog.debugging(Debug::CRIT) )
			dlog.crit() << "(MBTCPMaster): идентификатор '" << name
				<< "' не найден в конф. файле!"
				<< " в секции " << conf->getObjectsSection() << endl;
		return 0;
	}

	if( dlog.is_info() )
		dlog.info() << "(MBTCPMaster): name = " << name << "(" << ID << ")" << endl;
	return new MBTCPMaster(ID,icID,ic,prefix);
}
// -----------------------------------------------------------------------------
