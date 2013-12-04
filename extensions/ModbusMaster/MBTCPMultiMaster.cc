// -----------------------------------------------------------------------------
#include <cmath>
#include <limits>
#include <sstream>
#include <Exceptions.h>
#include <extensions/Extensions.h>
#include "MBTCPMultiMaster.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// -----------------------------------------------------------------------------
MBTCPMultiMaster::MBTCPMultiMaster( UniSetTypes::ObjectId objId, UniSetTypes::ObjectId shmId,
							SharedMemory* ic, const std::string prefix ):
MBExchange(objId,shmId,ic,prefix),
force_disconnect(true),
pollThread(0),
checkThread(0)
{
	if( objId == DefaultObjectId )
		throw UniSetTypes::SystemError("(MBTCPMultiMaster): objId=-1?!! Use --" + prefix + "-name" );

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

	dlog[Debug::INFO] << myname << "(init): prop_prefix=" << prop_prefix << endl;

	UniXML_iterator it(cnode);

	checktime = conf->getArgPInt("--" + prefix + "-checktime",it.getProp("checktime"), 5000);

	UniXML_iterator it1(it);
	if( !it1.find("SlaveList") )
	{
		ostringstream err;
		err << myname << "(init): not found <SlaveList>";
		dlog[Debug::CRIT] << err.str() << endl;
		throw UniSetTypes::SystemError(err.str());
	}

	if( !it1.goChildren() )
	{
		ostringstream err;
		err << myname << "(init): empty <SlaveList> ?!";
		dlog[Debug::CRIT] << err.str() << endl;
		throw UniSetTypes::SystemError(err.str());
	}

	for( ;it1.getCurrent(); it1++ )
	{
		MBSlaveInfo sinf;
		sinf.ip = it1.getProp("ip");
		if( sinf.ip.empty() )
		{
			ostringstream err;
			err << myname << "(init): ip='' in <SlaveList>";
			dlog[Debug::CRIT] << err.str() << endl;
			throw UniSetTypes::SystemError(err.str());
		}

		sinf.port = it1.getIntProp("port");
		if( sinf.port <=0 )
		{
			ostringstream err;
			err << myname << "(init): ERROR: port=''" << sinf.port << " for ip='" << sinf.ip << "' in <SlaveList>";
			dlog[Debug::CRIT] << err.str() << endl;
			throw UniSetTypes::SystemError(err.str());
		}

		if( !it1.getProp("respond").empty() )
		{
			sinf.respond_id = conf->getSensorID( it1.getProp("respond") );
			if( sinf.respond_id == DefaultObjectId )
			{
				ostringstream err;
				err << myname << "(init): ERROR: Unknown SensorID for '" << it1.getProp("respond") << "' in <SlaveList>";
				dlog[Debug::CRIT] << err.str() << endl;
				throw UniSetTypes::SystemError(err.str());
			}
		}

		sinf.priority = it1.getIntProp("priority");
		sinf.mbtcp = new ModbusTCPMaster();

		sinf.recv_timeout = it1.getPIntProp("recv_timeout",recv_timeout);
		sinf.aftersend_pause = it1.getPIntProp("aftersend_pause",aftersend_pause);
		sinf.sleepPause_usec = it1.getPIntProp("sleepPause_usec",sleepPause_usec);
		sinf.respond_invert = it1.getPIntProp("respond_invert",0);

//		sinf.force_disconnect = it.getProp("persistent_connection") ? false : true;

		ostringstream n;
		n << sinf.ip << ":" << sinf.port;
		sinf.myname = n.str();

		mblist.push_back(sinf);

		if( dlog.debugging(Debug::INFO) )
			dlog[Debug::INFO] << myname << "(init): add slave channel " << sinf.myname << endl;
	}

	if( mblist.empty() )
	{
		ostringstream err;
		err << myname << "(init): empty <SlaveList>!";
		dlog[Debug::CRIT] << err.str() << endl;
		throw UniSetTypes::SystemError(err.str());
	}

	mblist.sort();
	mbi = mblist.begin();

	if( shm->isLocalwork() )
	{
		readConfiguration();
		rtuQueryOptimization(rmap);
		initDeviceList();
	}
	else
		ic->addReadItem( sigc::mem_fun(this,&MBTCPMultiMaster::readItem) );

	pollThread = new ThreadCreator<MBTCPMultiMaster>(this, &MBTCPMultiMaster::poll_thread);
	checkThread = new ThreadCreator<MBTCPMultiMaster>(this, &MBTCPMultiMaster::check_thread);

	if( dlog.debugging(Debug::INFO) )
		printMap(rmap);
}
// -----------------------------------------------------------------------------
MBTCPMultiMaster::~MBTCPMultiMaster()
{
	delete pollThread;
	delete checkThread;
	for( MBSlaveList::iterator it=mblist.begin(); it!=mblist.end(); ++it )
	{
		delete it->mbtcp;
		it->mbtcp = 0;
		mbi = mblist.end();
	}
}
// -----------------------------------------------------------------------------
ModbusClient* MBTCPMultiMaster::initMB( bool reopen )
{
	if( mbi!=mblist.end() && mbi->respond )
	{
		cerr << "******** initMB(0): " << mbi->myname << endl;
		mb = mbi->mbtcp;
		return mbi->mbtcp;
	}

	if( mbi != mblist.end() )
		mbi->mbtcp->disconnect();

	// проходим по списку (в обратном порядке, т.к. самый приоритетный в конце)
	for( MBSlaveList::reverse_iterator it=mblist.rbegin(); it!=mblist.rend(); ++it )
	{
		if( it->respond )
		{
			mb = mbi->mbtcp;
			mbi = it.base();
			cerr << "******** initMB(L): " << mbi->myname << endl;
			return it->mbtcp;
		}
	}

	cout << myname << "(initMB): return 0..." << endl;
	mbi = mblist.end();
	mb = 0;
	return 0;
}
// -----------------------------------------------------------------------------
bool MBTCPMultiMaster::MBSlaveInfo::check()
{
	return mbtcp->checkConnection(ip,port);
}
// -----------------------------------------------------------------------------
bool MBTCPMultiMaster::MBSlaveInfo::init()
{
	try
	{
		ost::Thread::setException(ost::Thread::throwException);
		ost::InetAddress ia(ip.c_str());
		mbtcp->connect(ia,port);
		// mbtcp->setForceDisconnect(force_disconnect);

		if( recv_timeout > 0 )
			mbtcp->setTimeout(recv_timeout);

		mbtcp->setSleepPause(sleepPause_usec);
		mbtcp->setAfterSendPause(aftersend_pause);

		if( dlog.debugging(Debug::INFO) )
			dlog[Debug::INFO] << "(init): " << myname << endl;

		if( dlog.debugging(Debug::LEVEL9) )
			mbtcp->setLog(dlog);

		return true;
	}
	catch( ModbusRTU::mbException& ex )
	{
		if( dlog.debugging(Debug::WARN) )
			dlog[Debug::WARN] << "(init): " << ex << endl;
	}
	catch(...)
	{
		if( dlog.debugging(Debug::WARN) )
			dlog[Debug::WARN] << "(init): " << myname << " catch ..." << endl;
	}

	return false;
}
// -----------------------------------------------------------------------------
void MBTCPMultiMaster::sysCommand( UniSetTypes::SystemMessage *sm )
{
	MBExchange::sysCommand(sm);
	if( sm->command == SystemMessage::StartUp )
	{
		pollThread->start();
		checkThread->start();
	}
}
// -----------------------------------------------------------------------------
void MBTCPMultiMaster::poll_thread()
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
void MBTCPMultiMaster::check_thread()
{
	while( checkProcActive() )
	{
		for( MBSlaveList::iterator it=mblist.begin(); it!=mblist.end(); ++it )
		{
			try
			{
				bool r = it->check();
				if( dlog.debugging(Debug::INFO) )
					dlog[Debug::INFO] << myname << "(check): " << it->myname << " " << ( r ? "OK" : "FAIL" ) << endl;

				try
				{
					if( it->respond_id != DefaultObjectId && (force_out || r != it->respond) )
					{
						bool set = it->respond_invert ? !it->respond : it->respond;
						shm->localSaveState(it->respond_dit,it->respond_id,set,getId());
					}
				}
				catch( Exception& ex )
				{
					if( dlog.debugging(Debug::CRIT) )
						dlog[Debug::CRIT] << myname << "(check): (respond) " << ex << std::endl;
				}
				catch(...){}

				it->respond = r;
			}
			catch(...){}

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
	for( MBSlaveList::iterator it=mblist.begin(); it!=mblist.end(); ++it )
		shm->initDIterator(it->respond_dit);
}
// -----------------------------------------------------------------------------
void MBTCPMultiMaster::help_print( int argc, const char* const* argv )
{
	cout << "Default: prefix='mbtcp'" << endl;
	MBExchange::help_print(argc,argv);
	// ---------- init MBTCP ----------
//	cout << "--prefix-sm-ready-timeout - время на ожидание старта SM" << endl;
	cout << " Настройки протокола TCP: " << endl;
	cout << "--prefix-gateway-iaddr hostname,IP     - IP опрашиваемого узла" << endl;
	cout << "--prefix-gateway-port num              - port на опрашиваемом узле" << endl;
	cout << "--prefix-persistent-connection 0,1     - Не закрывать соединение на каждом цикле опроса" << endl;
}
// -----------------------------------------------------------------------------
MBTCPMultiMaster* MBTCPMultiMaster::init_mbmaster( int argc, const char* const* argv,
											UniSetTypes::ObjectId icID, SharedMemory* ic,
											const std::string prefix )
{
	string name = conf->getArgParam("--" + prefix + "-name","MBTCPMultiMaster1");
	if( name.empty() )
	{
		dlog[Debug::CRIT] << "(MBTCPMultiMaster): Не задан name'" << endl;
		return 0;
	}

	ObjectId ID = conf->getObjectID(name);
	if( ID == UniSetTypes::DefaultObjectId )
	{
		dlog[Debug::CRIT] << "(MBTCPMultiMaster): идентификатор '" << name
			<< "' не найден в конф. файле!"
			<< " в секции " << conf->getObjectsSection() << endl;
		return 0;
	}

	dlog[Debug::INFO] << "(MBTCPMultiMaster): name = " << name << "(" << ID << ")" << endl;
	return new MBTCPMultiMaster(ID,icID,ic,prefix);
}
// -----------------------------------------------------------------------------
