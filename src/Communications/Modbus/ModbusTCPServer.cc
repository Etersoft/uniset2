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
#include <iostream>
#include <sstream>
#include "Exceptions.h"
#include "modbus/ModbusTCPServer.h"
#include "modbus/ModbusTCPCore.h"
// -------------------------------------------------------------------------
using namespace std;
using namespace ModbusRTU;
using namespace UniSetTypes;
// -------------------------------------------------------------------------
ModbusTCPServer::ModbusTCPServer( ost::InetAddress& ia, int _port ):
	port(_port),
	iaddr(ia),
	myname(""),
	ignoreAddr(false),
	maxSessions(10),
	sessCount(0),
	sessTimeout(10000),
	cancelled(false)
{
	setCRCNoCheckit(true);
	{
		ostringstream s;
		s << iaddr << ":" << port;
		myname = s.str();
	}
}

// -------------------------------------------------------------------------
ModbusTCPServer::~ModbusTCPServer()
{
	if( cancelled )
		finish();
}
// -------------------------------------------------------------------------
void ModbusTCPServer::setMaxSessions( unsigned int num )
{
	if( num < sessCount )
	{
		uniset_mutex_lock l(sMutex);

		int k = sessCount - num;
		int d = 0;

		for( SessionList::reverse_iterator i = slist.rbegin(); d < k && i != slist.rend(); ++i, d++ )
			delete *i;

		sessCount = num;
	}

	maxSessions = num;
}
// -------------------------------------------------------------------------
size_t ModbusTCPServer::getCountSessions()
{
	return sessCount;
}
// -------------------------------------------------------------------------
void ModbusTCPServer::setSessionTimeout( timeout_t msec )
{
	sessTimeout = msec;
}
// -------------------------------------------------------------------------
void ModbusTCPServer::run( const std::unordered_set<ModbusAddr>& _vmbaddr, bool thread )
{
	vmbaddr = &_vmbaddr;

	if( !thread )
	{
		mainLoop();
		return;
	}

	thrMainLoop = make_shared< ThreadCreator<ModbusTCPServer> >(this, &ModbusTCPServer::mainLoop);
	thrMainLoop->start();
}
// -------------------------------------------------------------------------
void ModbusTCPServer::mainLoop()
{
	try
	{
		sock = make_shared<UTCPSocket>(iaddr, port);
	}
	catch( const ost::SockException& ex )
	{
		ostringstream err;
		err << "(ModbusTCPServer::mainLoop): connect " << iaddr << ":" << port << " err: " << ex.what();
		dlog->crit() << err.str() << endl;
		throw SystemError(err.str());
	}

	sock->setCompletion(false);

	io.set<ModbusTCPServer, &ModbusTCPServer::ioAccept>(this);
	io.start(sock->getSocket(), ev::READ);

	ioTimer.set<ModbusTCPServer, &ModbusTCPServer::onTimer>(this);

	if( tmTime != TIMEOUT_INF )
		ioTimer.start(tmTime);

	if( dlog->is_info() )
		dlog->info() << myname << "(ModbusTCPServer): run main loop.." << endl;

	{
		evloop = DefaultEventLoop::inst();
		evloop->run(this, false);
	}

	if( dlog->is_info() )
		dlog->info() << myname << "(ModbusTCPServer): main loop exit.." << endl;

	cancelled = true;
}
// -------------------------------------------------------------------------
void ModbusTCPServer::terminate()
{
	finish();
}
// -------------------------------------------------------------------------
void ModbusTCPServer::finish()
{
	if( cancelled )
		return;

	cancelled = true;

	if( dlog->is_info() )
		dlog->info() << myname << "(ModbusTCPServer): terminate..." << endl;

	ioTimer.stop();
	io.stop();

	auto lst(slist);

	// Копируем сперва себе список сессий..
	// т.к при вызове terminate()
	// у Session будет вызван сигнал "final"
	// который приведёт к вызову sessionFinished()..в котором список будет меняться..
	for( const auto& s : lst )
	{
		try
		{
			s->terminate();
		}
		catch( std::exception& ex ) {}
	}

	if( evloop )
		evloop->terminate(this);
}
// -------------------------------------------------------------------------
void ModbusTCPServer::sessionFinished( ModbusTCPSession* s )
{
	uniset_mutex_lock l(sMutex);

	for( auto i = slist.begin(); i != slist.end(); ++i )
	{
		if( (*i) == s )
		{
			slist.erase(i);
			sessCount--;
			return;
		}
	}
}
// -------------------------------------------------------------------------
void ModbusTCPServer::getSessions( Sessions& lst )
{
	uniset_mutex_lock l(sMutex);

	for( const auto& i : slist )
	{
		SessionInfo inf( i->getClientAddress(), i->getAskCount() );
		lst.push_back(inf);
	}
}
// -------------------------------------------------------------------------
bool ModbusTCPServer::isActive()
{
	return !cancelled;
}
// -------------------------------------------------------------------------
ModbusTCPServer::TimerSignal ModbusTCPServer::signal_timer()
{
	return m_timer_signal;
}
// -------------------------------------------------------------------------
void ModbusTCPServer::setTimer( timeout_t msec )
{
	tmTime = msec;

	if( msec == TIMEOUT_INF )
	{
		tmTime = 0;
		ioTimer.stop();
	}
	else
	{
		tmTime = (double)msec / 1000.;
		ioTimer.start( tmTime );
	}
}
// -------------------------------------------------------------------------
void ModbusTCPServer::ioAccept(ev::io& watcher, int revents)
{
	if (EV_ERROR & revents)
	{
		if( dlog->is_crit() )
			dlog->crit() << myname << "(ModbusTCPServer::ioAccept): invalid event" << endl;

		return;
	}

	if( cancelled )
	{
		if( dlog->is_crit() )
			dlog->crit() << myname << "(ModbusTCPServer::ioAccept): terminate work.." << endl;

		sock->reject();
		return;
	}

	if( sessCount >= maxSessions )
	{
		if( dlog->is_crit() )
			dlog->crit() << myname << "(ModbusTCPServer::ioAccept): session limit(" << maxSessions << ")" << endl;

		sock->reject();
		return;
	}

	if( dlog->is_info() )
		dlog->info() << myname << "(ModbusTCPServer): new connection.." << endl;

	try
	{
		ModbusTCPSession* s = new ModbusTCPSession(watcher.fd, *vmbaddr, sessTimeout);
		s->connectReadCoil( sigc::mem_fun(this, &ModbusTCPServer::readCoilStatus) );
		s->connectReadInputStatus( sigc::mem_fun(this, &ModbusTCPServer::readInputStatus) );
		s->connectReadOutput( sigc::mem_fun(this, &ModbusTCPServer::readOutputRegisters) );
		s->connectReadInput( sigc::mem_fun(this, &ModbusTCPServer::readInputRegisters) );
		s->connectForceSingleCoil( sigc::mem_fun(this, &ModbusTCPServer::forceSingleCoil) );
		s->connectForceCoils( sigc::mem_fun(this, &ModbusTCPServer::forceMultipleCoils) );
		s->connectWriteOutput( sigc::mem_fun(this, &ModbusTCPServer::writeOutputRegisters) );
		s->connectWriteSingleOutput( sigc::mem_fun(this, &ModbusTCPServer::writeOutputSingleRegister) );
		s->connectMEIRDI( sigc::mem_fun(this, &ModbusTCPServer::read4314) );
		s->connectSetDateTime( sigc::mem_fun(this, &ModbusTCPServer::setDateTime) );
		s->connectDiagnostics( sigc::mem_fun(this, &ModbusTCPServer::diagnostics) );
		s->connectFileTransfer( sigc::mem_fun(this, &ModbusTCPServer::fileTransfer) );
		s->connectJournalCommand( sigc::mem_fun(this, &ModbusTCPServer::journalCommand) );
		s->connectRemoteService( sigc::mem_fun(this, &ModbusTCPServer::remoteService) );
		s->connectFileTransfer( sigc::mem_fun(this, &ModbusTCPServer::fileTransfer) );
		s->setAfterSendPause(aftersend_msec);
		s->setReplyTimeout(replyTimeout_ms);
		s->setRecvTimeout(recvTimeOut_ms);
		s->setSleepPause(sleepPause_usec);
		s->setCleanBeforeSend(cleanBeforeSend);
		s->setSessionTimeout( (double)sessTimeout / 1000. );
		s->signal_post_receive().connect( sigc::mem_fun(this, &ModbusTCPServer::postReceiveEvent) );
		s->signal_pre_receive().connect( sigc::mem_fun(this, &ModbusTCPServer::preReceiveEvent) );

		s->setLog(dlog);
		s->connectFinalSession( sigc::mem_fun(this, &ModbusTCPServer::sessionFinished) );

		{
			uniset_mutex_lock l(sMutex);
			slist.push_back(s);
		}

		s->run();
		sessCount++;
	}
	catch( Exception& ex )
	{
		if( dlog->is_crit() )
			dlog->crit() << myname << "(ModbusTCPServer): new connection error: " << ex << endl;
	}
}
// -------------------------------------------------------------------------
void ModbusTCPServer::onTimer( ev::timer& t, int revents )
{
	if (EV_ERROR & revents)
	{
		if( dlog->is_crit() )
			dlog->crit() << myname << "(ModbusTCPServer::ioTimer): invalid event" << endl;

		return;
	}

	try
	{
		m_timer_signal.emit();
	}
	catch( std::exception& ex )
	{
		if( dlog->is_crit() )
			dlog->crit() << myname << "(onTimer): " << ex.what() << endl;
	}

	ioTimer.start(tmTime); // restart timer
}
// -------------------------------------------------------------------------

mbErrCode  ModbusTCPServer::realReceive( const std::unordered_set<ModbusAddr>& vaddr, timeout_t msecTimeout )
{
	return ModbusRTU::erOperationFailed;
}
// -------------------------------------------------------------------------
void ModbusTCPServer::postReceiveEvent( mbErrCode res )
{
	m_post_signal.emit(res);
}
// -------------------------------------------------------------------------
mbErrCode ModbusTCPServer::preReceiveEvent(const std::unordered_set<ModbusAddr> vaddr, timeout_t tout)
{
	if( m_pre_signal.empty() )
		return ModbusRTU::erNoError;

	return m_pre_signal.emit(vaddr, tout);
}
// -------------------------------------------------------------------------
