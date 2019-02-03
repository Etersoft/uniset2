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
#include <string>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <Poco/Net/NetException.h>
#include "modbus/ModbusTCPSession.h"
#include "modbus/ModbusTCPCore.h"
#include "UniSetTypes.h"
// glibc..
#include <netinet/tcp.h>
// -------------------------------------------------------------------------
namespace uniset
{
	// -------------------------------------------------------------------------
	using namespace std;
	using namespace ModbusRTU;
	using namespace uniset;
	// -------------------------------------------------------------------------
	ModbusTCPSession::~ModbusTCPSession()
	{
		cancelled = true;

		if( io.is_active() )
			io.stop();

		if( ioTimeout.is_active() )
			ioTimeout.stop();
	}
	// -------------------------------------------------------------------------
	ModbusTCPSession::ModbusTCPSession(const Poco::Net::StreamSocket& s, const std::unordered_set<ModbusAddr>& a, timeout_t timeout ):
		vaddr(a),
		timeout(timeout),
		peername(""),
		caddr(""),
		cancelled(false)
	{
		try
		{
			sock = make_shared<UTCPStream>(s);

			// если стремиться к "оптимизации по скорости"
			// то getpeername "медленная" операция и может стоит от неё отказаться.
			Poco::Net::SocketAddress  iaddr = sock->peerAddress();

			if( iaddr.host().toString().empty() )
			{
				ostringstream err;
				err << "(ModbusTCPSession): unknonwn ip(0.0.0.0) client disconnected?!";

				if( dlog->is_crit() )
					dlog->crit() << err.str() << endl;

				sock.reset();
				throw SystemError(err.str());
			}

			caddr = iaddr.host().toString();
			ostringstream s;
			s << caddr << ":" << iaddr.port();
			peername = s.str();
		}
		catch( const Poco::Net::NetException& ex )
		{
			if( dlog->is_crit() )
				dlog->crit() << "(ModbusTCPSession): err: " << ex.displayText() << endl;

			sock.reset();
			throw SystemError(ex.message());
		}

		sock->setBlocking(false); // fcntl(sfd, F_SETFL, fcntl(sfd, F_GETFL, 0) | O_NONBLOCK);
		setCRCNoCheckit(true);

		timeout_t tout = timeout / 1000;

		if( tout <= 0 )
			tout = 3;

		io.set<ModbusTCPSession, &ModbusTCPSession::callback>(this);
		ioTimeout.set<ModbusTCPSession, &ModbusTCPSession::onTimeout>(this);
	}
	// -------------------------------------------------------------------------
	void ModbusTCPSession::setSessionTimeout( double t )
	{
		sessTimeout = t;

		if( t <= 0 )
			ioTimeout.stop();
		else if( ioTimeout.is_active() )
			ioTimeout.start(t);
	}
	// -------------------------------------------------------------------------
	void ModbusTCPSession::iowait( timeout_t msec )
	{
		ptWait.setTiming(msec);

		while( !ptWait.checkTime() )
			io.loop.iteration();
	}
	// -------------------------------------------------------------------------
	void ModbusTCPSession::run( ev::loop_ref& loop )
	{
		if( dlog->is_info() )
			dlog->info() << peername << "(run): run session.." << endl;

		io.set(loop);
		io.start(sock->getSocket(), ev::READ);

		ioTimeout.set(loop);

		if( sessTimeout > 0 )
			ioTimeout.start(sessTimeout);
	}
	// -------------------------------------------------------------------------
	bool ModbusTCPSession::isActive() const
	{
		return io.is_active();
	}
	// -------------------------------------------------------------------------
	void ModbusTCPSession::callback( ev::io& watcher, int revents )
	{
		if( EV_ERROR & revents )
		{
			if( dlog->is_crit() )
				dlog->crit() << peername << "(callback): EVENT ERROR.." << endl;

			return;
		}

		if (revents & EV_READ)
			readEvent(watcher);

		if (revents & EV_WRITE)
			writeEvent(watcher);

		if( qsend.empty() )
			io.set(ev::READ);
		else
			io.set(ev::READ | ev::WRITE);

		if( cancelled )
		{
			if( dlog->is_info() )
				dlog->info() << peername << ": stop session... disconnect.." << endl;

			terminate();
		}
		else if( sessTimeout > 0 )
			ioTimeout.start(sessTimeout); // restart timer..
	}
	// -------------------------------------------------------------------------
	void ModbusTCPSession::onTimeout( ev::timer& t, int revents )
	{
		if( dlog->is_info() )
			dlog->info() << peername << ": timeout connection activity..(terminate session)" << endl;

		t.stop();
		terminate();
	}
	// -------------------------------------------------------------------------
	void ModbusTCPSession::readEvent( ev::io& watcher )
	{
		if( receive(vaddr, timeout) == erSessionClosed )
			cancelled = true;
	}
	// -------------------------------------------------------------------------
	void ModbusTCPSession::writeEvent( ev::io& watcher )
	{
		if( qsend.empty() )
			return;

		UTCPCore::Buffer* buffer = qsend.front();

		ssize_t ret = write(watcher.fd, buffer->dpos(), buffer->nbytes());

		if( ret < 0 )
		{
			if( dlog->is_warn() )
				dlog->warn() << peername << "(writeEvent): write to socket error: " << strerror(errno) << endl;

			return;
		}

		buffer->pos += ret;

		if( buffer->nbytes() == 0 )
		{
			qsend.pop();
			delete buffer;
		}
	}
	// -------------------------------------------------------------------------
	ModbusRTU::mbErrCode ModbusTCPSession::realReceive( const std::unordered_set<ModbusAddr>& vmbaddr, timeout_t msec )
	{
		ModbusRTU::mbErrCode res = erTimeOut;
		ptTimeout.setTiming(msec);

		try
		{
			buf.clear();
			res = tcp_processing(buf.mbaphead);

			if( res != erNoError )
				return res;

			if( msec != UniSetTimer::WaitUpTime )
			{
				msec = ptTimeout.getLeft(msec);

				if( msec == 0 ) // времени для ответа не осталось..
					return erTimeOut;
			}

			if( qrecv.empty() )
				return erTimeOut;

			if( cancelled )
				return erSessionClosed;

			// запоминаем принятый заголовок,
			// для формирования ответа (см. make_adu_header)
			curQueryHeader = buf.mbaphead;

			if( dlog->is_level9() )
				dlog->level9() << "(ModbusTCPSession::recv): ADU len=" << curQueryHeader.len << endl;

			res = recv( vmbaddr, buf, msec );

			if( cancelled )
				return erSessionClosed;

			if( res != erNoError ) // && res!=erBadReplyNodeAddress )
			{
				if( res < erInternalErrorCode )
				{
					ErrorRetMessage em( buf.addr(), buf.func(), res );
					buf = em.transport_msg();
					send(buf);
					printProcessingTime();
				}
				else if( aftersend_msec > 0 )
					iowait(aftersend_msec);

				return res;
			}

			if( msec != UniSetTimer::WaitUpTime )
			{
				msec = ptTimeout.getLeft(msec);

				if( msec == 0 )
					return erTimeOut;
			}

			if( cancelled )
				return erSessionClosed;

			// processing message...
			res = processing(buf);
		}
		catch( uniset::CommFailed& ex )
		{
			cancelled = true;
			return erSessionClosed;
		}

		return res;
	}
	// -------------------------------------------------------------------------
	void ModbusTCPSession::final()
	{
		slFin(this);
	}
	// -------------------------------------------------------------------------
	mbErrCode ModbusTCPSession::sendData( unsigned char* buf, int len )
	{
		qsend.emplace( new UTCPCore::Buffer(buf, len) );
		return erNoError;
	}
	// -------------------------------------------------------------------------
	size_t ModbusTCPSession::getNextData( unsigned char* buf, int len )
	{
		ssize_t res = ModbusTCPCore::getDataFD( sock->getSocket(), qrecv, buf, len );

		try
		{
			if( res > 0 )
				return res;

			if( res < 0 )
			{
				int errnum = errno;

				if( errnum != EAGAIN && dlog->is_warn() )
					dlog->warn() << peername << "(getNextData): read from socket error(" << errnum << "): " << strerror(errnum) << endl;

				return 0;
			}
		}
		catch( const uniset::CommFailed& ex ){}

		if( !cancelled && dlog->is_info() )
			dlog->info() << peername << "(getNextData): client disconnected" << endl;

		cancelled = true;
		return 0;
	}
	// --------------------------------------------------------------------------------
	mbErrCode ModbusTCPSession::tcp_processing( ModbusRTU::MBAPHeader& mhead )
	{
		// чистим очередь
		while( !qrecv.empty() )
			qrecv.pop();

		size_t len = getNextData((unsigned char*)(&mhead), sizeof(mhead));
		//size_t len = ModbusTCPCore::getDataFD( sfd, qrecv, (unsigned char*)(&mhead), sizeof(mhead) );

		if( len < sizeof(mhead) )
		{
			if( len == 0 )
				return erSessionClosed;

			return erInvalidFormat;
		}

		mhead.swapdata();

		if( dlog->is_level9() )
		{
			dlog->level9() << peername << "(tcp_processing): recv tcp header(" << len << "): ";
			mbPrintMessage( dlog->level9(false), (ModbusByte*)(&mhead), sizeof(mhead));
			dlog->level9(false) << endl;
		}

		// check header
		if( mhead.pID != 0 )
			return erUnExpectedPacketType; // erTimeOut;

		if( mhead.len == 0 )
		{
			if( dlog->is_warn() )
				dlog->warn() << "(ModbusTCPServer::tcp_processing): BAD FORMAT: len=0!" << endl;

			return erInvalidFormat;
		}

		if( mhead.len > ModbusRTU::MAXLENPACKET )
		{
			if( dlog->is_warn() )
				dlog->warn() << "(ModbusTCPServer::tcp_processing): len(" << (int)mhead.len
							 << ") < MAXLENPACKET(" << ModbusRTU::MAXLENPACKET << ")" << endl;

			return erInvalidFormat;
		}

		pt.setTiming(10);

		try
		{
			do
			{
				len = ModbusTCPCore::readDataFD( sock->getSocket(), qrecv, mhead.len );

				if( len == 0 )
					io.loop.iteration();
			}
			while( len == 0 && !pt.checkTime() );
		}
		catch( const uniset::CommFailed& ex ){}

		if( len < mhead.len )
		{
			if( dlog->is_warn() )
				dlog->warn() << peername << "(tcp_processing): len(" << (int)len
							 << ") < mhead.len(" << (int)mhead.len << ")" << endl;

			return erInvalidFormat;
		}

		return erNoError;
	}
	// -------------------------------------------------------------------------
	ModbusRTU::mbErrCode ModbusTCPSession::post_send_request( ModbusRTU::ModbusMessage& request )
	{
		return erNoError;
	}
	// -------------------------------------------------------------------------
	mbErrCode ModbusTCPSession::make_adu_header( ModbusMessage& req )
	{
		req.makeMBAPHeader(curQueryHeader.tID, isCRCNoCheckit(), curQueryHeader.pID);
		return erNoError;
	}
	// -------------------------------------------------------------------------
	void ModbusTCPSession::cleanInputStream()
	{
		unsigned char tmpbuf[100];
		size_t ret = 0;

		do
		{
			ret = getNextData(tmpbuf, sizeof(tmpbuf));
		}
		while( ret > 0);
	}
	// -------------------------------------------------------------------------
	void ModbusTCPSession::cleanupChannel()
	{
		cleanInputStream();
	}
	// -------------------------------------------------------------------------
	void ModbusTCPSession::terminate()
	{
		if( dlog->is_info() )
			dlog->info() << peername << "(terminate)..." << endl;

		cancelled = true;
		io.stop();
		ioTimeout.stop();

		ModbusServer::terminate();

		sock.reset(); // close..
		final(); // после этого вызова возможно данный объект будет разрушен!
	}
	// -------------------------------------------------------------------------
	mbErrCode ModbusTCPSession::readCoilStatus( ReadCoilMessage& query,
			ReadCoilRetMessage& reply )
	{
		if( !slReadCoil )
			return erOperationFailed;

		return slReadCoil(query, reply);
	}

	// -------------------------------------------------------------------------
	mbErrCode ModbusTCPSession::readInputStatus( ReadInputStatusMessage& query,
			ReadInputStatusRetMessage& reply )
	{
		if( !slReadInputStatus )
			return erOperationFailed;

		return slReadInputStatus(query, reply);
	}

	// -------------------------------------------------------------------------

	mbErrCode ModbusTCPSession::readOutputRegisters( ReadOutputMessage& query,
			ReadOutputRetMessage& reply )
	{
		if( !slReadOutputs )
			return erOperationFailed;

		return slReadOutputs(query, reply);
	}

	// -------------------------------------------------------------------------
	mbErrCode ModbusTCPSession::readInputRegisters( ReadInputMessage& query,
			ReadInputRetMessage& reply )
	{
		if( !slReadInputs )
			return erOperationFailed;

		return slReadInputs(query, reply);
	}

	// -------------------------------------------------------------------------
	mbErrCode ModbusTCPSession::forceMultipleCoils( ForceCoilsMessage& query,
			ForceCoilsRetMessage& reply )
	{
		if( !slForceCoils )
			return erOperationFailed;

		return slForceCoils(query, reply);
	}

	// -------------------------------------------------------------------------

	mbErrCode ModbusTCPSession::writeOutputRegisters( WriteOutputMessage& query,
			WriteOutputRetMessage& reply )
	{
		if( !slWriteOutputs )
			return erOperationFailed;

		return slWriteOutputs(query, reply);
	}

	// -------------------------------------------------------------------------
	mbErrCode ModbusTCPSession::diagnostics( DiagnosticMessage& query,
			DiagnosticRetMessage& reply )
	{
		if( !slDiagnostics )
			return erOperationFailed;

		return slDiagnostics(query, reply);
	}
	// -------------------------------------------------------------------------
	ModbusRTU::mbErrCode ModbusTCPSession::read4314( ModbusRTU::MEIMessageRDI& query,
			ModbusRTU::MEIMessageRetRDI& reply )
	{
		if( !slMEIRDI )
			return erOperationFailed;

		return slMEIRDI(query, reply);
	}
	// -------------------------------------------------------------------------
	mbErrCode ModbusTCPSession::forceSingleCoil( ForceSingleCoilMessage& query,
			ForceSingleCoilRetMessage& reply )
	{
		if( !slForceSingleCoil )
			return erOperationFailed;

		return slForceSingleCoil(query, reply);
	}

	// -------------------------------------------------------------------------
	mbErrCode ModbusTCPSession::writeOutputSingleRegister( WriteSingleOutputMessage& query,
			WriteSingleOutputRetMessage& reply )
	{
		if( !slWriteSingleOutputs )
			return erOperationFailed;

		return slWriteSingleOutputs(query, reply);
	}

	// -------------------------------------------------------------------------
	mbErrCode ModbusTCPSession::journalCommand( JournalCommandMessage& query,
			JournalCommandRetMessage& reply )
	{
		if( !slJournalCommand )
			return erOperationFailed;

		return slJournalCommand(query, reply);
	}
	// -------------------------------------------------------------------------
	ModbusRTU::mbErrCode ModbusTCPSession::setDateTime( ModbusRTU::SetDateTimeMessage& query,
			ModbusRTU::SetDateTimeRetMessage& reply )
	{
		if( !slSetDateTime )
			return erOperationFailed;

		return slSetDateTime(query, reply);
	}
	// -------------------------------------------------------------------------
	ModbusRTU::mbErrCode ModbusTCPSession::remoteService( ModbusRTU::RemoteServiceMessage& query,
			ModbusRTU::RemoteServiceRetMessage& reply )
	{
		if( !slRemoteService )
			return erOperationFailed;

		return slRemoteService(query, reply);
	}
	// -------------------------------------------------------------------------
	ModbusRTU::mbErrCode ModbusTCPSession::fileTransfer( ModbusRTU::FileTransferMessage& query,
			ModbusRTU::FileTransferRetMessage& reply )
	{
		if( !slFileTransfer )
			return erOperationFailed;

		return slFileTransfer(query, reply);
	}
	// -------------------------------------------------------------------------
	void ModbusTCPSession::setChannelTimeout( timeout_t msec )
	{
		setSessionTimeout(msec);
	}
	// -------------------------------------------------------------------------
	void ModbusTCPSession::connectFinalSession( FinalSlot sl )
	{
		slFin = sl;
	}
	// -------------------------------------------------------------------------
	string ModbusTCPSession::getClientAddress() const
	{
		return caddr;
	}
	// -------------------------------------------------------------------------
} // end of namespace uniset
