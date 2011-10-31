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
	for( MBTCPMaster::RTUDeviceMap::iterator it1=rmap.begin(); it1!=rmap.end(); ++it1 )
	{
		RTUDevice* d(it1->second);
		for( MBTCPMaster::RegMap::iterator it=d->regmap.begin(); it!=d->regmap.end(); ++it )
			delete it->second;

		delete it1->second;
	}

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
void MBTCPMaster::step()
{
	updateRespondSensors();
	MBExchange::step();
}
// -----------------------------------------------------------------------------
void MBTCPMaster::updateRespondSensors()
{
	bool tcpIsTimeout = false;
	{
		uniset_mutex_lock l(tcpMutex);
		tcpIsTimeout = pollActivated && ptTimeout.checkTime();
	}

	if( dlog.debugging(Debug::LEVEL4) )
		dlog[Debug::LEVEL4] << myname << ": tcpTimeout=" << tcpIsTimeout << endl;

	for( MBTCPMaster::RTUDeviceMap::iterator it1=rmap.begin(); it1!=rmap.end(); ++it1 )
	{
		RTUDevice* d(it1->second);

		if( tcpIsTimeout )
			d->resp_real = false;

		if( dlog.debugging(Debug::LEVEL4) )
		{
			dlog[Debug::LEVEL4] << myname << ": check respond addr=" << ModbusRTU::addr2str(d->mbaddr)
				<< " respond_id=" << d->resp_id
				<< " real=" << d->resp_real
				<< " state=" << d->resp_state
				<< endl;
		}

		if( d->checkRespond() && d->resp_id != DefaultObjectId  )
		{
			try
			{
				bool set = d->resp_invert ? !d->resp_state : d->resp_state;
				shm->localSaveState(d->resp_dit,d->resp_id,set,getId());
			}
			catch( Exception& ex )
			{
				dlog[Debug::CRIT] << myname << "(step): (respond) " << ex << std::endl;
			}
		}
	}
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
bool MBTCPMaster::RTUDevice::checkRespond()
{
	bool prev = resp_state;

	if( resp_ptTimeout.getInterval() <= 0 )
	{
		resp_state = resp_real;
		return (prev != resp_state);
	}
	
	if( resp_trTimeout.hi(resp_state && !resp_real) )
		resp_ptTimeout.reset();
	
	if( resp_real )
		resp_state = true;
	else if( resp_state && !resp_real && resp_ptTimeout.checkTime() )
		resp_state = false; 
	
	// если ещё не инициализировали значение в SM
	// то возвращаем true, чтобы оно принудительно сохранилось
	if( !resp_init )
	{
		resp_state = resp_real;
		resp_init = true;
		prev = resp_state;
		return true;
	}

	return ( prev != resp_state );
}
// -----------------------------------------------------------------------------
void MBTCPMaster::sigterm( int signo )
{
	dlog[Debug::WARN] << myname << ": ********* SIGTERM(" << signo <<") ********" << endl;
	setProcActive(false);

/*! \todo Доделать выставление безопасного состояния на выходы. 
          И нужно ли это. Ведь может не хватить времени на "обмен" 
*/
	
	// выставление безопасного состояния на выходы....
/*
	RSMap::iterator it=rsmap.begin();
	for( ; it!=rsmap.end(); ++it )
	{
//		if( it->stype!=UniversalIO::DigitalOutput && it->stype!=UniversalIO::AnalogOutput )
//			continue;
		
		if( it->safety == NoSafetyState )
			continue;

		try
		{
		}
		catch( UniSetTypes::Exception& ex )
		{
			dlog[Debug::WARN] << myname << "(sigterm): " << ex << std::endl;
		}
		catch(...){}
	}
*/	
	UniSetObject_LT::sigterm(signo);
}
// ------------------------------------------------------------------------------------------
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
void MBTCPMaster::execute()
{
	no_extimer = true;

	try
	{
		askTimer(tmExchange,0);
	}
	catch(...){}
	
	initMB(false);

	while(1)
	{
		try
		{
			step();
		}
		catch( Exception& ex )
		{
			dlog[Debug::CRIT] << myname << "(execute): " << ex << std::endl;
		}
		catch(...)
		{
			dlog[Debug::CRIT] << myname << "(execute): catch ..." << endl;
		}

		msleep(polltime);
	}
}
// -----------------------------------------------------------------------------
