#include <iostream>
#include <string>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <cc++/socket.h>
#include "modbus/ModbusTCPSession.h"
#include "modbus/ModbusTCPCore.h"
#include "UniSetTypes.h"
// -------------------------------------------------------------------------
using namespace std;
using namespace ModbusRTU;
using namespace UniSetTypes;
// -------------------------------------------------------------------------
ModbusTCPSession::~ModbusTCPSession()
{
	cancelled = true;

	if( isRunning() )
		ost::Thread::join();
}
// -------------------------------------------------------------------------
ModbusTCPSession::ModbusTCPSession( ost::TCPSocket& server, ModbusRTU::ModbusAddr a, timeout_t timeout ):
	TCPSession(server),
	addr(a),
	timeout(timeout),
	peername(""),
	caddr(""),
	cancelled(false),
	askCount(0)
{
	setCRCNoCheckit(true);
}
// -------------------------------------------------------------------------
unsigned int ModbusTCPSession::getAskCount()
{
	uniset_rwmutex_rlock l(mAsk);
	return askCount;
}
// -------------------------------------------------------------------------
void ModbusTCPSession::run()
{
	if( cancelled )
		return;

	{
		ost::tpport_t p;
		ost::InetAddress iaddr = getIPV4Peer(&p);

		// resolve..
		caddr = string( iaddr.getHostname() );

		ostringstream s;
		s << iaddr << ":" << p;
		peername = s.str();

		//      struct in_addr a = iaddr.getAddress();
		//      cerr << "**************** CREATE SESS FOR " << string( inet_ntoa(a) ) << endl;
	}

	if( dlog->is_info() )
		dlog->info() << peername << "(run): run thread of sessions.." << endl;

	ModbusRTU::mbErrCode res = erTimeOut;
	cancelled = false;

	while( !cancelled && isPending(Socket::pendingInput, timeout) )
	{
		res = receive(addr, timeout);

		if( res == erSessionClosed )
			break;

		/*        if( res == erBadReplyNodeAddress )
		            break;*/

		if( res != erTimeOut )
		{
			uniset_rwmutex_wrlock l(mAsk);
			askCount++;
		}
	}

	if( dlog->is_info() )
		dlog->info() << peername << "(run): stop thread of sessions..disconnect.." << endl;

	disconnect();

	if( dlog->is_info() )
		dlog->info() << peername << "(run): thread stopping..." << endl;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode ModbusTCPSession::receive( ModbusRTU::ModbusAddr addr, timeout_t msec )
{
	ModbusRTU::mbErrCode res = erTimeOut;
	ptTimeout.setTiming(msec);

	{
		memset(&curQueryHeader, 0, sizeof(curQueryHeader));
		res = tcp_processing( static_cast<ost::TCPStream&>(*this), curQueryHeader);

		if( res != erNoError )
			return res;

		if( msec != UniSetTimer::WaitUpTime )
		{
			msec = ptTimeout.getLeft(msec);

			if( msec == 0 ) // времени для ответа не осталось..
				return erTimeOut;
		}

		if( !qrecv.empty() )
		{
			// check addr
			unsigned char _addr = qrecv.front();

			// для режима игнорирования RTU-адреса
			// просто подменяем его на то который пришёл
			// чтобы проверка всегда была успешной...
			if( ignoreAddr )
				addr = _addr;
			else if( _addr != addr )
			{
				// На такие запросы просто не отвечаем...
				return erTimeOut;
			}
		}

		if( cancelled )
			return erSessionClosed;

		memset(&buf, 0, sizeof(buf));
		res = recv( addr, buf, msec );

		if( cancelled )
			return erSessionClosed;

		if( res != erNoError ) // && res!=erBadReplyNodeAddress )
		{
			if( res < erInternalErrorCode )
			{
				ErrorRetMessage em( addr, buf.func, res );
				buf = em.transport_msg();
				send(buf);
				printProcessingTime();
			}
			else if( aftersend_msec >= 0 )
				msleep(aftersend_msec);

			return res;
		}

		if( msec != UniSetTimer::WaitUpTime )
		{
			msec = ptTimeout.getLeft(msec);

			if( msec == 0 )
				return erTimeOut;
		}

		if( res != erNoError )
			return res;

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
	return ModbusTCPCore::sendData(this, buf, len);
}
// -------------------------------------------------------------------------
int ModbusTCPSession::getNextData( unsigned char* buf, int len )
{
	return ModbusTCPCore::getNextData(this, qrecv, buf, len);
}
// --------------------------------------------------------------------------------

mbErrCode ModbusTCPSession::tcp_processing( ost::TCPStream& tcp, ModbusTCP::MBAPHeader& mhead )
{
	if( !isConnected() )
		return erTimeOut;

	// чистим очередь
	while( !qrecv.empty() )
		qrecv.pop();

	unsigned int len = getNextData((unsigned char*)(&mhead), sizeof(mhead));

	if( len < sizeof(mhead) )
	{
		if( len == 0 )
			return erSessionClosed;

		return erInvalidFormat;
	}

	mhead.swapdata();

	if( dlog->is_info() )
	{
		dlog->info() << peername << "(tcp_processing): recv tcp header(" << len << "): ";
		mbPrintMessage( *(dlog.get()), (ModbusByte*)(&mhead), sizeof(mhead));
		(*(dlog.get()))(Debug::INFO) << endl;
	}

	// check header
	if( mhead.pID != 0 )
		return erUnExpectedPacketType; // erTimeOut;

	len = ModbusTCPCore::readNextData(&tcp, qrecv, mhead.len);

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
	*tcp() << endl;
	return erNoError;
}
// -------------------------------------------------------------------------
mbErrCode ModbusTCPSession::pre_send_request( ModbusMessage& request )
{
	if( !isConnected() )
		return erTimeOut;

	curQueryHeader.len = request.len + szModbusHeader;

	if( crcNoCheckit )
		curQueryHeader.len -= szCRC;

	curQueryHeader.swapdata();

	if( dlog->is_info() )
	{
		dlog->info() << peername << "(pre_send_request): send tcp header: ";
		mbPrintMessage( *(dlog.get()), (ModbusByte*)(&curQueryHeader), sizeof(curQueryHeader));
		(*(dlog.get()))(Debug::INFO) << endl;
	}

	*tcp() << curQueryHeader;

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
void ModbusTCPSession::terminate()
{
	ModbusServer::terminate();

	if( dlog->is_info() )
		dlog->info() << peername << "(terminate)..." << endl;

	cancelled = true;

	if( isConnected() )
		disconnect();

	//    if( isRunning() )
	//        ost::Thread::join();
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
	setTimeout(msec);
}
// -------------------------------------------------------------------------
void ModbusTCPSession::connectFinalSession( FinalSlot sl )
{
	slFin = sl;
}
// -------------------------------------------------------------------------
