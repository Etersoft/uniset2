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
#include <string>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <cc++/socket.h>
#include "modbus/ModbusTCPSession.h"
#include "modbus/ModbusTCPCore.h"
#include "UniSetTypes.h"
// glibc..
#include <netinet/tcp.h>
// -------------------------------------------------------------------------
using namespace std;
using namespace ModbusRTU;
using namespace UniSetTypes;
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
ModbusTCPSession::ModbusTCPSession( int sfd, const std::unordered_set<ModbusAddr>& a, timeout_t timeout ):
	vaddr(a),
	timeout(timeout),
	peername(""),
	caddr(""),
	cancelled(false)
{
	try
	{
		sock = make_shared<USocket>(sfd);
		ost::tpport_t p;

		// если стремиться к "оптимизации по скорости"
		// то resolve ip "медленная" операция и может стоит от неё отказаться.
		ost::InetAddress iaddr = sock->getIPV4Peer(&p);

		if( !iaddr.isInetAddress() )
		{
			ostringstream err;
			err << "(ModbusTCPSession): unknonwn ip(0.0.0.0) client disconnected?!";
			if( dlog->is_crit() )
				dlog->crit() << err.str() << endl;
			sock.reset();
			throw SystemError(err.str());
		}

		caddr = string( iaddr.getHostname() );
		ostringstream s;
		s << iaddr << ":" << p;
		peername = s.str();
	}
	catch( const ost::SockException& ex )
	{
		ostringstream err;
		err << ex.what();
		if( dlog->is_crit() )
			dlog->crit() << "(ModbusTCPSession): err: " << err.str() << endl;

		sock.reset();
		throw SystemError(err.str());
	}

	sock->setCompletion(false); // fcntl(sfd, F_SETFL, fcntl(sfd, F_GETFL, 0) | O_NONBLOCK);
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

	if( ioTimeout.is_active() )
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
	ioTimeout.start(sessTimeout);
}
// -------------------------------------------------------------------------
bool ModbusTCPSession::isActive()
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
	else
		ioTimeout.start(sessTimeout); // restart timer..
}
// -------------------------------------------------------------------------
void ModbusTCPSession::onTimeout( ev::timer& watcher, int revents )
{
	if( dlog->is_info() )
		dlog->info() << peername << ": timeout connection activity..(terminate session)" << endl;

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

	{
		memset(&curQueryHeader, 0, sizeof(curQueryHeader));
		res = tcp_processing(curQueryHeader);

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

		unsigned char q_addr = qrecv.front();

		if( cancelled )
			return erSessionClosed;

		memset(&buf, 0, sizeof(buf));
		res = recv( vmbaddr, buf, msec );

		if( cancelled )
			return erSessionClosed;

		if( res != erNoError ) // && res!=erBadReplyNodeAddress )
		{
			if( res < erInternalErrorCode )
			{
				ErrorRetMessage em( q_addr, buf.func, res );
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
	qsend.push( new UTCPCore::Buffer(buf, len) );
	return erNoError;
}
// -------------------------------------------------------------------------
size_t ModbusTCPSession::getNextData( unsigned char* buf, int len )
{
	ssize_t res = ModbusTCPCore::getDataFD( sock->getSocket(), qrecv, buf, len );

	if( res > 0 )
		return res;

	if( res < 0 )
	{
		if( errno != EAGAIN && dlog->is_warn() )
			dlog->warn() << peername << "(getNextData): read from socket error(" << errno << "): " << strerror(errno) << endl;

		return 0;
	}

	if( !cancelled && dlog->is_info() )
		dlog->info() << peername << "(getNextData): client disconnected" << endl;

	cancelled = true;
	return 0;
}
// --------------------------------------------------------------------------------
mbErrCode ModbusTCPSession::tcp_processing( ModbusTCP::MBAPHeader& mhead )
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

	if( dlog->is_info() )
	{
		//dlog->info() << peername << "(tcp_processing): recv tcp header(" << len << "): " << mhead << endl;
		dlog->info() << peername << "(tcp_processing): recv tcp header(" << len << "): ";
		mbPrintMessage( dlog->info(false), (ModbusByte*)(&mhead), sizeof(mhead));
		dlog->info(false) << endl;
	}

	// check header
	if( mhead.pID != 0 )
		return erUnExpectedPacketType; // erTimeOut;

	if( mhead.len > ModbusRTU::MAXLENPACKET )
	{
		if( dlog->is_info() )
			dlog->info() << "(ModbusTCPServer::tcp_processing): len(" << (int)mhead.len
						 << ") < MAXLENPACKET(" << ModbusRTU::MAXLENPACKET << ")" << endl;

		return erInvalidFormat;
	}

	pt.setTiming(10);

	do
	{
		len = ModbusTCPCore::readDataFD( sock->getSocket(), qrecv, mhead.len );

		if( len == 0 )
			io.loop.iteration();
	}
	while( len == 0 && !pt.checkTime() );

	if( len < mhead.len )
	{
		if( dlog->is_info() )
			dlog->info() << peername << "(tcp_processing): len(" << (int)len
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
mbErrCode ModbusTCPSession::pre_send_request( ModbusMessage& request )
{
	curQueryHeader.len = request.len + szModbusHeader;

	if( crcNoCheckit )
		curQueryHeader.len -= szCRC;

	curQueryHeader.swapdata();

	if( dlog->is_info() )
	{
		dlog->info() << peername << "(pre_send_request): send tcp header: ";
		mbPrintMessage( dlog->info(false), (ModbusByte*)(&curQueryHeader), sizeof(curQueryHeader));
		dlog->info(false) << endl;
	}

	sendData((unsigned char*)(&curQueryHeader), sizeof(curQueryHeader));
	curQueryHeader.swapdata();

	return erNoError;
}
// -------------------------------------------------------------------------
void ModbusTCPSession::cleanInputStream()
{
	unsigned char buf[100];
	int ret = 0;

	do
	{
		ret = getNextData(buf, sizeof(buf));
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
