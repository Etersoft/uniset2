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
#include <cmath>
#include <limits>
#include <sstream>
#include <Exceptions.h>
#include <extensions/Extensions.h>
#include "unisetstd.h"
#include "MBTCPMaster.h"
#include "modbus/MBLogSugar.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
using namespace uniset::extensions;
// -----------------------------------------------------------------------------
MBTCPMaster::MBTCPMaster(uniset::ObjectId objId, uniset::ObjectId shmId,
						 const std::shared_ptr<SharedMemory>& ic, const std::string& prefix ):
	MBExchange(objId, shmId, ic, prefix),
	force_disconnect(true)
{
	if( objId == DefaultObjectId )
		throw uniset::SystemError("(MBTCPMaster): objId=-1?!! Use --" + prefix + "-name" );

	auto conf = uniset_conf();

	// префикс для "свойств" - по умолчанию "tcp_";
	prop_prefix = initPropPrefix("tcp_");
	mbinfo << myname << "(init): prop_prefix=" << prop_prefix << endl;

	UniXML::iterator it(cnode);

	// ---------- init MBTCP ----------
	string pname("--" + prefix + "-gateway-iaddr");
	iaddr    = conf->getArgParam(pname, it.getProp("gateway_iaddr"));

	if( iaddr.empty() )
		throw uniset::SystemError(myname + "(MBMaster): Unknown inet addr...(Use: " + pname + ")" );

	string tmp("--" + prefix + "-gateway-port");
	port = conf->getArgInt(tmp, it.getProp("gateway_port"));

	if( port <= 0 )
		throw uniset::SystemError(myname + "(MBMaster): Unknown inet port...(Use: " + tmp + ")" );

	mbinfo << myname << "(init): gateway " << iaddr << ":" << port << endl;

	force_disconnect = conf->getArgInt("--" + prefix + "-persistent-connection", it.getProp("persistent_connection")) ? false : true;
	mbinfo << myname << "(init): persisten-connection=" << (!force_disconnect) << endl;

	if( shm->isLocalwork() )
	{
		readConfiguration();
		rtuQueryOptimization(devices);
		initDeviceList();
	}
	else
		ic->addReadItem( sigc::mem_fun(this, &MBTCPMaster::readItem) );

	pollThread = unisetstd::make_unique<ThreadCreator<MBTCPMaster>>(this, &MBTCPMaster::poll_thread);
	pollThread->setFinalAction(this, &MBTCPMaster::final_thread);

	if( mblog->is_info() )
		printMap(devices);
}
// -----------------------------------------------------------------------------
MBTCPMaster::~MBTCPMaster()
{
	if( pollThread )
	{
		try
		{
			pollThread->stop();

			if( pollThread->isRunning() )
				pollThread->join();
		}
		catch( Poco::NullPointerException& ex )
		{

		}
	}
}
// -----------------------------------------------------------------------------
std::shared_ptr<ModbusClient> MBTCPMaster::initMB( bool reopen )
{
	if( mbtcp )
	{
		if( !reopen )
			return mbtcp;

		ptInitChannel.reset();

		mbtcp->forceDisconnect();
		mbtcp->connect(iaddr, port);
		mbinfo << myname << "(init): ipaddr=" << iaddr << " port=" << port
			   << " connection=" << (mbtcp->isConnection() ? "OK" : "FAIL" ) << endl;
		mb = mbtcp;
		return mbtcp;
	}

	mbtcp = std::make_shared<ModbusTCPMaster>();
	mbtcp->connect(iaddr, port);
	mbtcp->setForceDisconnect(force_disconnect);

	if( recv_timeout > 0 )
		mbtcp->setTimeout(recv_timeout);

	mbtcp->setSleepPause(sleepPause_msec);
	mbtcp->setAfterSendPause(aftersend_pause);

	mbinfo << myname << "(init): ipaddr=" << iaddr << " port=" << port
		   << " connection=" << (mbtcp->isConnection() ? "OK" : "FAIL" ) << endl;

	auto l = loga->create(myname + "-exchangelog");
	mbtcp->setLog(l);

	if( ic )
		ic->logAgregator()->add(loga);

	mb = mbtcp;
	return mbtcp;
}
// -----------------------------------------------------------------------------
void MBTCPMaster::sysCommand( const uniset::SystemMessage* sm )
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
	while( !isProcActive() )
	{
		uniset::uniset_rwmutex_rlock l(mutex_start);
	}

	// работаем
	while( isProcActive() )
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

		if( !isProcActive() )
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
bool MBTCPMaster::deactivateObject()
{
	setProcActive(false);

	if( pollThread )
	{
		pollThread->stop();

		if( pollThread->isRunning() )
			pollThread->join();
	}

	return MBExchange::deactivateObject();
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
std::shared_ptr<MBTCPMaster> MBTCPMaster::init_mbmaster(int argc, const char* const* argv,
		uniset::ObjectId icID, const std::shared_ptr<SharedMemory>& ic,
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

	if( ID == uniset::DefaultObjectId )
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
uniset::SimpleInfo* MBTCPMaster::getInfo( const char* userparam )
{
	uniset::SimpleInfo_var i = MBExchange::getInfo(userparam);

	ostringstream inf;

	inf << i->info << endl;
	inf << "poll: " << iaddr << ":" << port << " pesrsistent-connection=" << ( force_disconnect ? "NO" : "YES" ) << endl;

	i->info = inf.str().c_str();
	return i._retn();
}
// ----------------------------------------------------------------------------
