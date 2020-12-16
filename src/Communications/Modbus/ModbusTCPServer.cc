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
#include <Poco/Net/NetException.h>
#include "Exceptions.h"
#include "modbus/ModbusTCPServer.h"
#include "modbus/ModbusTCPCore.h"
// -------------------------------------------------------------------------
namespace uniset
{
	// -------------------------------------------------------------------------
	using namespace std;
	using namespace ModbusRTU;
	using namespace uniset;
	// -------------------------------------------------------------------------
	ModbusTCPServer::ModbusTCPServer( const std::string& ia, int _port ):
		port(_port),
		iaddr(ia),
		myname(""),
		ignoreAddr(false),
		maxSessions(10),
		sessCount(0),
		sessTimeout(10000)
	{
		setCRCNoCheckit(true);
		{
			ostringstream s;
			s << iaddr << ":" << port;
			myname = s.str();
		}

		io.set<ModbusTCPServer, &ModbusTCPServer::ioAccept>(this);
		ioTimer.set<ModbusTCPServer, &ModbusTCPServer::onTimer>(this);
	}

	// -------------------------------------------------------------------------
	ModbusTCPServer::~ModbusTCPServer()
	{
	}
	// -------------------------------------------------------------------------
	void ModbusTCPServer::setMaxSessions( size_t num )
	{
		if( num < sessCount )
		{
			std::lock_guard<std::mutex> l(sMutex);

			int k = sessCount - num;
			int d = 0;

			for( SessionList::reverse_iterator i = slist.rbegin(); d < k && i != slist.rend(); ++i, d++ )
				(*i).reset();

			sessCount = num;
		}

		maxSessions = num;
	}
	// -------------------------------------------------------------------------
	size_t ModbusTCPServer::getMaxSessions() const noexcept
	{
		return maxSessions;
	}
	// -------------------------------------------------------------------------
	size_t ModbusTCPServer::getCountSessions() const noexcept
	{
		return sessCount;
	}
	// -------------------------------------------------------------------------
	void ModbusTCPServer::setSessionTimeout( timeout_t msec )
	{
		sessTimeout = msec;
	}
	// -------------------------------------------------------------------------
	timeout_t ModbusTCPServer::getSessionTimeout() const noexcept
	{
		return sessTimeout;
	}
	// -------------------------------------------------------------------------
	bool ModbusTCPServer::run( const std::unordered_set<ModbusAddr>& _vmbaddr )
	{
		vmbaddr = _vmbaddr;
		return evrun();
	}
	// -------------------------------------------------------------------------
	bool ModbusTCPServer::async_run( const std::unordered_set<ModbusAddr>& _vmbaddr )
	{
		vmbaddr = _vmbaddr;
		return async_evrun();
	}
	// -------------------------------------------------------------------------
	bool ModbusTCPServer::isActive() const
	{
		return evIsActive() && sock;
	}
	// -------------------------------------------------------------------------
	void ModbusTCPServer::evprepare()
	{
		try
		{
			sock = make_shared<UTCPSocket>(iaddr, port);
		}
		catch( const Poco::Net::NetException& ex )
		{
			ostringstream err;
			err << "(ModbusTCPServer::evprepare): connect " << iaddr << ":" << port << " err: " << ex.what();
			dlog->crit() << err.str() << endl;
			throw uniset::SystemError(err.str());
		}
		catch( const std::exception& ex )
		{
			ostringstream err;
			err << "(ModbusTCPServer::evprepare): connect " << iaddr << ":" << port << " err: " << ex.what();
			dlog->crit() << err.str() << endl;
			throw uniset::SystemError(err.str());
		}

		sock->setBlocking(false);

		io.set(loop);
		io.start(sock->getSocket(), ev::READ);

		ioTimer.set(loop);

		if( tmTime_msec != UniSetTimer::WaitUpTime )
			ioTimer.start(0, tmTime);
	}
	// -------------------------------------------------------------------------
	void ModbusTCPServer::terminate()
	{
		evstop();
	}
	// -------------------------------------------------------------------------
	void ModbusTCPServer::evfinish()
	{
		if( !io.is_active() )
			return;

		if( dlog->is_info() )
			dlog->info() << myname << "(ModbusTCPServer): terminate..." << endl;

		ioTimer.stop();
		io.stop();

		auto lst(slist);

		// Копируем сперва себе список сессий..
		// т.к при вызове terminate()
		// у Session будет вызван сигнал "final"
		// который приведёт к вызову sessionFinished()..
		// в котором этот список будет меняться (удалится сессия из списка)
		for( const auto& s : lst )
		{
			try
			{
				s->terminate();
			}
			catch( std::exception& ex ) {}
		}
	}
	// -------------------------------------------------------------------------
	void ModbusTCPServer::sessionFinished( const ModbusTCPSession* s )
	{
		std::lock_guard<std::mutex> l(sMutex);

		for( auto i = slist.begin(); i != slist.end(); ++i )
		{
			if( (*i).get() == s )
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
		std::lock_guard<std::mutex> l(sMutex);

		for( const auto& i : slist )
			lst.emplace_back( i->getClientAddress(), i->getAskCount() );
	}
	// -------------------------------------------------------------------------
	string ModbusTCPServer::getInetAddress() const noexcept
	{
		return iaddr;
	}
	// -------------------------------------------------------------------------
	int ModbusTCPServer::getInetPort() const noexcept
	{
		return port;
	}
	// -------------------------------------------------------------------------
	size_t ModbusTCPServer::getConnectionCount() const noexcept
	{
		return connCount;
	}
	// -------------------------------------------------------------------------
	ModbusTCPServer::TimerSignal ModbusTCPServer::signal_timer()
	{
		return m_timer_signal;
	}
	// -------------------------------------------------------------------------
	void ModbusTCPServer::setTimer( timeout_t msec )
	{
		tmTime_msec = msec;

		if( msec == UniSetTimer::WaitUpTime )
		{
			tmTime = 0;

			if( ioTimer.is_active() )
				ioTimer.stop();
		}
		else
		{
			tmTime = (double)msec / 1000.;

			if( ioTimer.is_active() )
				ioTimer.start( 0, tmTime );
		}
	}
	// -------------------------------------------------------------------------
	timeout_t ModbusTCPServer::getTimer() const noexcept
	{
		return tmTime;
	}
	// -------------------------------------------------------------------------
	void ModbusTCPServer::iowait( timeout_t msec )
	{
		ptWait.setTiming(msec);

		while( !ptWait.checkTime() )
			io.loop.iteration();
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

		if( !evIsActive() )
		{
			if( dlog->is_crit() )
				dlog->crit() << myname << "(ModbusTCPServer::ioAccept): terminate work.." << endl;

			sock->close();
			return;
		}

		if( sessCount >= maxSessions )
		{
			if( dlog->is_crit() )
				dlog->crit() << myname << "(ModbusTCPServer::ioAccept): session limit(" << maxSessions << ")" << endl;

			sock->close();
			return;
		}

		if( dlog->is_info() )
			dlog->info() << myname << "(ModbusTCPServer): new connection.." << endl;

		try
		{
			Poco::Net::StreamSocket ss = sock->acceptConnection();

			connCount++;

			auto s = make_shared<ModbusTCPSession>(ss, vmbaddr, sessTimeout);
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
			s->setSleepPause(sleepPause_msec);
			s->setCleanBeforeSend(cleanBeforeSend);
			s->setSessionTimeout( (double)sessTimeout / 1000. );
			s->signal_post_receive().connect( sigc::mem_fun(this, &ModbusTCPServer::postReceiveEvent) );
			s->signal_pre_receive().connect( sigc::mem_fun(this, &ModbusTCPServer::preReceiveEvent) );

			s->setLog(dlog);
			s->connectFinalSession( sigc::mem_fun(this, &ModbusTCPServer::sessionFinished) );

			{
				std::lock_guard<std::mutex> l(sMutex);
				slist.push_back(s);
			}

			s->run(loop);
			sessCount++;
		}
		catch( std::exception& ex )
		{
			if( dlog->is_crit() )
				dlog->crit() << myname << "(ModbusTCPServer): new connection error: " << ex.what() << endl;
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
} // end of namespace uniset
