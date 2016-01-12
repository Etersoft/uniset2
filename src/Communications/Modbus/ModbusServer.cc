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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <iostream>
#include <sstream>
#include "Exceptions.h"
#include "modbus/ModbusServer.h"
// -------------------------------------------------------------------------
using namespace std;
using namespace ModbusRTU;
using namespace UniSetTypes;
// -------------------------------------------------------------------------
ModbusServer::ModbusServer():
	recvMutex("ModbusServer_recvMutex"),
	recvTimeOut_ms(50),
	replyTimeout_ms(2000),
	aftersend_msec(0),
	sleepPause_usec(100),
	onBroadcast(false),
	crcNoCheckit(false),
	cleanBeforeSend(false)
{
	dlog = make_shared<DebugStream>();
	tmProcessing.setTiming(replyTimeout_ms);
	/*
	    dlog->addLevel(Debug::ANY);
	    dlog->addLevel(Debug::WARN);
	    dlog->addLevel(Debug::CRIT);
	    dlog->addLevel(Debug::INFO);
	*/
}

// -------------------------------------------------------------------------
ModbusServer::~ModbusServer()
{
}
// -------------------------------------------------------------------------
void ModbusServer::setRecvTimeout( timeout_t msec )
{
	if( msec != UniSetTimer::WaitUpTime )
		recvTimeOut_ms = msec;
}
// -------------------------------------------------------------------------
timeout_t ModbusServer::setReplyTimeout( timeout_t msec )
{
	// #warning "Why msec can be 0?"
	assert(msec);

	if( msec == UniSetTimer::WaitUpTime )
		return replyTimeout_ms;

	timeout_t old = replyTimeout_ms; // запоминаем текущее значение, чтобы вернуть в конце
	replyTimeout_ms = msec;
	tmProcessing.setTiming(replyTimeout_ms);
	return old;
}
// -------------------------------------------------------------------------
timeout_t ModbusServer::setAfterSendPause( timeout_t msec )
{
	timeout_t old = aftersend_msec;
	aftersend_msec = msec;
	return old;
}
// --------------------------------------------------------------------------------
bool ModbusServer::checkAddr(const std::unordered_set<ModbusAddr>& vaddr, const ModbusRTU::ModbusAddr addr )
{
	if( addr == ModbusRTU::BroadcastAddr )
		return true;

	auto i = vaddr.find(addr);
	return (i != vaddr.end());
}
// --------------------------------------------------------------------------------
std::string ModbusServer::vaddr2str( const std::unordered_set<ModbusAddr>& vaddr )
{
	ostringstream s;
	s << "[ ";

	for( const auto& a : vaddr )
		s << addr2str(a) << " ";

	s << "]";

	return std::move(s.str());
}
// --------------------------------------------------------------------------------
mbErrCode ModbusServer::processing( ModbusMessage& buf )
{
	if( buf.func == fnReadCoilStatus )
	{
		ReadCoilMessage mRead(buf);
		ReadCoilRetMessage reply(buf.addr); // addr?
		// вызываем обработчик..
		mbErrCode res = readCoilStatus( mRead, reply );

		// в случае ошибок ответа не посылаем
		if( res != erNoError )
		{
			dlog->warn() << "(0x01): err reply: " << mbErr2Str(res) << endl;

			// Если ошибка подразумевает посылку ответа с сообщением об ошибке
			// то посылаем
			if( res < erInternalErrorCode )
			{
				ErrorRetMessage em( mRead.addr, mRead.func, res );
				buf = em.transport_msg();
				return send(buf);
			}

			return res;
		}

		// отвечаем (используя тотже буфер, который будет очищен!!!)...
		buf = reply.transport_msg();
		// -----------------------------------
		// return send(buf);
		res = send(buf);
		printProcessingTime();
		// --------------------------------
		return res;
	}
	else if( buf.func == fnReadInputStatus )
	{
		ReadInputStatusMessage mRead(buf);
		ReadInputStatusRetMessage reply(buf.addr); // addr?
		// вызываем обработчик..
		mbErrCode res = readInputStatus( mRead, reply );

		// в случае ошибок ответа не посылаем
		if( res != erNoError )
		{
			dlog->warn() << "(0x02): err reply: " << mbErr2Str(res) << endl;

			// Если ошибка подразумевает посылку ответа с сообщением об ошибке
			// то посылаем
			if( res < erInternalErrorCode )
			{
				ErrorRetMessage em( mRead.addr, mRead.func, res );
				buf = em.transport_msg();
				return send(buf);
			}

			return res;
		}

		// отвечаем (используя тотже буфер, который будет очищен!!!)...
		buf = reply.transport_msg();
		// -----------------------------------
		// return send(buf);
		res = send(buf);
		printProcessingTime();
		// --------------------------------
		return res;
	}
	else if( buf.func == fnReadOutputRegisters )
	{
		ReadOutputMessage mRead(buf);
		ReadOutputRetMessage reply(buf.addr); // addr?
		// вызываем обработчик..
		mbErrCode res = readOutputRegisters( mRead, reply );

		// в случае ошибок ответа не посылаем
		if( res != erNoError )
		{
			dlog->warn() << "(0x03): err reply: " << mbErr2Str(res) << endl;

			// Если ошибка подразумевает посылку ответа с сообщением об ошибке
			// то посылаем
			if( res < erInternalErrorCode )
			{
				ErrorRetMessage em( mRead.addr, mRead.func, res );
				buf = em.transport_msg();
				return send(buf);
			}

			return res;
		}

		// отвечаем (используя тотже буфер, который будет очищен!!!)...
		buf = reply.transport_msg();
		// -----------------------------------
		// return send(buf);
		res = send(buf);
		printProcessingTime();
		// --------------------------------
		return res;
	}
	else if( buf.func == fnReadInputRegisters )
	{
		ReadInputMessage mRead(buf);
		ReadInputRetMessage reply(buf.addr); // addr?
		// вызываем обработчик..
		mbErrCode res = readInputRegisters( mRead, reply );

		// в случае ошибок ответа не посылаем
		if( res != erNoError )
		{
			dlog->warn() << "(0x04): err reply: " << mbErr2Str(res) << endl;

			// Если ошибка подразумевает посылку ответа с сообщением об ошибке
			// то посылаем
			if( res < erInternalErrorCode )
			{
				ErrorRetMessage em( mRead.addr, mRead.func, res );
				buf = em.transport_msg();
				return send(buf);
			}

			return res;
		}

		// отвечаем (используя тотже буфер, который будет очищен!!!)...
		buf = reply.transport_msg();
		// -----------------------------------
		// return send(buf);
		res = send(buf);
		printProcessingTime();
		// --------------------------------
		return res;
	}
	else if( buf.func == fnForceMultipleCoils )
	{
		ForceCoilsMessage mWrite(buf);
		ForceCoilsRetMessage reply(buf.addr); // addr?

		// вызываем обработчик..
		mbErrCode res = forceMultipleCoils( mWrite, reply );

		// в случае ошибок ответа не посылаем
		if( res != erNoError )
		{
			dlog->warn() << "(0x0F): err reply: " << mbErr2Str(res) << endl;

			// Если ошибка подразумевает посылку ответа с сообщением об ошибке
			// то посылаем
			if( res < erInternalErrorCode )
			{
				ErrorRetMessage em( mWrite.addr, mWrite.func, res );
				buf = em.transport_msg();
				return send(buf);
			}

			return res;
		}

		// отвечаем (используя тотже буфер, который будет очищен!!!)...
		buf = reply.transport_msg();
		// -----------------------------------
		//        return send(buf);
		res = send(buf);
		printProcessingTime();
		// --------------------------------
		return res;
	}
	else if( buf.func == fnWriteOutputRegisters )
	{
		WriteOutputMessage mWrite(buf);
		WriteOutputRetMessage reply(buf.addr); // addr?

		// вызываем обработчик..
		mbErrCode res = writeOutputRegisters( mWrite, reply );

		// в случае ошибок ответа не посылаем
		if( res != erNoError )
		{
			dlog->warn() << "(0x10): err reply: " << mbErr2Str(res) << endl;

			// Если ошибка подразумевает посылку ответа с сообщением об ошибке
			// то посылаем
			if( res < erInternalErrorCode )
			{
				ErrorRetMessage em( mWrite.addr, mWrite.func, res );
				buf = em.transport_msg();
				return send(buf);
			}

			return res;
		}

		// отвечаем (используя тотже буфер, который будет очищен!!!)...
		buf = reply.transport_msg();
		// -----------------------------------
		//        return send(buf);
		res = send(buf);
		printProcessingTime();
		// --------------------------------
		return res;
	}
	else if( buf.func == fnDiagnostics )
	{
		DiagnosticMessage mDiag(buf);
		DiagnosticRetMessage reply(buf.addr, (DiagnosticsSubFunction)mDiag.subf );
		mbErrCode res = diagnostics( mDiag, reply );

		// в случае ошибок ответа не посылаем
		if( res != erNoError )
		{
			dlog->warn() << "(0x08): reply: " << mbErr2Str(res) << endl;

			// Если ошибка подразумевает посылку ответа с сообщением об ошибке
			// то посылаем
			if( res < erInternalErrorCode )
			{
				ErrorRetMessage em( mDiag.addr, mDiag.func, res );
				buf = em.transport_msg();
				return send(buf);
			}

			return res;
		}

		// отвечаем (используя тотже буфер, который будет очищен!!!)...
		buf = reply.transport_msg();
		// -----------------------------------
		res = send(buf);
		printProcessingTime();
		// --------------------------------
		return res;
	}
	else if( buf.func == fnMEI )
	{
		MEIMessageRDI mRDI(buf);
		MEIMessageRetRDI reply( buf.addr, mRDI.devID, 0, 0, mRDI.objID );

		mbErrCode res = read4314( mRDI, reply );

		// в случае ошибок ответа не посылаем
		if( res != erNoError )
		{
			dlog->warn() << "(0x2B/0x0E): reply: " << mbErr2Str(res) << endl;

			// Если ошибка подразумевает посылку ответа с сообщением об ошибке
			// то посылаем
			if( res < erInternalErrorCode )
			{
				ErrorRetMessage em( mRDI.addr, mRDI.func, res );
				buf = em.transport_msg();
				return send(buf);
			}

			return res;
		}

		// отвечаем (используя тотже буфер, который будет очищен!!!)...
		buf = reply.transport_msg();
		// -----------------------------------
		res = send(buf);
		printProcessingTime();
		// --------------------------------
		return res;
	}
	else if( buf.func == fnForceSingleCoil )
	{
		ForceSingleCoilMessage mWrite(buf);
		ForceSingleCoilRetMessage reply(buf.addr); // addr?

		// вызываем обработчик..
		mbErrCode res = forceSingleCoil( mWrite, reply );

		// в случае ошибок ответа не посылаем
		if( res != erNoError )
		{
			dlog->warn() << "(0x05): reply: " << mbErr2Str(res) << endl;

			// Если ошибка подразумевает посылку ответа с сообщением об ошибке
			// то посылаем
			if( res < erInternalErrorCode )
			{
				ErrorRetMessage em( mWrite.addr, mWrite.func, res );
				buf = em.transport_msg();
				return send(buf);
			}

			return res;
		}

		// отвечаем (используя тотже буфер, который будет очищен!!!)...
		buf = reply.transport_msg();
		// -----------------------------------
		//        return send(buf);
		res = send(buf);
		printProcessingTime();
		// --------------------------------
		return res;
	}
	else if( buf.func == fnWriteOutputSingleRegister )
	{
		WriteSingleOutputMessage mWrite(buf);
		WriteSingleOutputRetMessage reply(buf.addr); // addr?

		// вызываем обработчик..
		mbErrCode res = writeOutputSingleRegister( mWrite, reply );

		// в случае ошибок ответа не посылаем
		if( res != erNoError )
		{
			dlog->warn() << "(0x06): reply: " << mbErr2Str(res) << endl;

			// Если ошибка подразумевает посылку ответа с сообщением об ошибке
			// то посылаем
			if( res < erInternalErrorCode )
			{
				ErrorRetMessage em( mWrite.addr, mWrite.func, res );
				buf = em.transport_msg();
				return send(buf);
			}

			return res;
		}

		// отвечаем (используя тотже буфер, который будет очищен!!!)...
		buf = reply.transport_msg();
		// -----------------------------------
		//        return send(buf);
		res = send(buf);
		printProcessingTime();
		// --------------------------------
		return res;
	}
	else if( buf.func == fnJournalCommand )
	{
		JournalCommandMessage mJournal(buf);
		JournalCommandRetMessage reply(buf.addr); // addr?
		// вызываем обработчик..
		mbErrCode res = journalCommand( mJournal, reply );

		// в случае ошибок ответа не посылаем
		if( res != erNoError )
		{
			dlog->warn() << "(0x65): reply: " << mbErr2Str(res) << endl;

			// Если ошибка подразумевает посылку ответа с сообщением об ошибке
			// то посылаем
			if( res < erInternalErrorCode )
			{
				ErrorRetMessage em( mJournal.addr, mJournal.func, res );
				buf = em.transport_msg();
				return send(buf);
			}

			return res;
		}

		// отвечаем (используя тотже буфер, который будет очищен!!!)...
		buf = reply.transport_msg();
		// -----------------------------------
		// return send(buf);
		res = send(buf);
		printProcessingTime();
		// --------------------------------
		return res;
	}
	else if( buf.func == fnSetDateTime )
	{
		SetDateTimeMessage mSet(buf);
		SetDateTimeRetMessage reply(buf.addr); // addr?
		// вызываем обработчик..
		mbErrCode res = setDateTime( mSet, reply );

		// в случае ошибок ответа не посылаем
		if( res != erNoError )
		{
			dlog->warn() << "(0x50): reply: " << mbErr2Str(res) << endl;

			// Если ошибка подразумевает посылку ответа с сообщением об ошибке
			// то посылаем
			if( res < erInternalErrorCode )
			{
				ErrorRetMessage em( mSet.addr, mSet.func, res );
				buf = em.transport_msg();
				return send(buf);
			}

			return res;
		}

		// отвечаем (используя тотже буфер, который будет очищен!!!)...
		buf = reply.transport_msg();
		// -----------------------------------
		// return send(buf);
		res = send(buf);
		printProcessingTime();
		// --------------------------------
		return res;
	}
	else if( buf.func == fnRemoteService )
	{
		RemoteServiceMessage query(buf);
		RemoteServiceRetMessage reply(buf.addr);
		// вызываем обработчик..
		mbErrCode res = remoteService( query, reply );

		// в случае ошибок ответа не посылаем
		if( res != erNoError )
		{
			dlog->warn() << "(0x53): error reply: " << mbErr2Str(res) << endl;

			// Если ошибка подразумевает посылку ответа с сообщением об ошибке
			// то посылаем
			if( res < erInternalErrorCode )
			{
				ErrorRetMessage em( query.addr, query.func, res );
				buf = em.transport_msg();
				return send(buf);
			}

			return res;
		}

		// отвечаем (используя тотже буфер, который будет очищен!!!)...
		buf = reply.transport_msg();
		// -----------------------------------
		// return send(buf);
		res = send(buf);
		printProcessingTime();
		// --------------------------------
		return res;
	}
	else if( buf.func == fnFileTransfer )
	{
		FileTransferMessage query(buf);
		FileTransferRetMessage reply(buf.addr);
		// вызываем обработчик..
		mbErrCode res = fileTransfer( query, reply );

		// в случае ошибок ответа не посылаем
		if( res != erNoError )
		{
			dlog->warn() << "(0x66): error reply: " << mbErr2Str(res) << endl;

			// Если ошибка подразумевает посылку ответа с сообщением об ошибке
			// то посылаем
			if( res < erInternalErrorCode )
			{
				ErrorRetMessage em( query.addr, query.func, res );
				buf = em.transport_msg();
				return send(buf);
			}

			return res;
		}

		// отвечаем (используя тотже буфер, который будет очищен!!!)...
		buf = reply.transport_msg();
		// -----------------------------------
		// return send(buf);
		res = send(buf);
		printProcessingTime();
		// --------------------------------
		return res;
	}

	ErrorRetMessage em( buf.addr, buf.func, erUnExpectedPacketType );
	buf = em.transport_msg();
	send(buf);
	printProcessingTime();
	return erUnExpectedPacketType;
}
// -------------------------------------------------------------------------
mbErrCode ModbusServer::recv( const std::unordered_set<ModbusRTU::ModbusAddr>& vaddr, ModbusMessage& rbuf, timeout_t timeout )
{
	assert(timeout);

	if( timeout == UniSetTimer::WaitUpTime )
		timeout = 15 * 60 * 1000; // используем просто большое время (15 минут), переведя его в наносекунды.

	setChannelTimeout(timeout);
	PassiveTimer tmAbort(timeout);

	// предварительно чистим буфер
	memset(&rbuf, 0, sizeof(rbuf));
	int bcnt = 0;  // receive bytes count

	try
	{
		// wait byte = myaddr || byte = broadcast
		bool begin = false;

		while( !tmAbort.checkTime() )
		{
			bcnt = getNextData((unsigned char*)(&rbuf), sizeof(ModbusAddr));

			if( bcnt > 0 && checkAddr(vaddr, rbuf.addr) )
			{
				begin = true;
				break;
			}

			usleep(sleepPause_usec);
		}

		if( !begin )
			return erTimeOut;

		/*! \todo Подумать Может стоит всё-таки получать весь пакет, а проверять кому он адресован на уровне выше?!
		            // Lav: конечно стоит, нам же надо буфер чистить
		*/
		// Проверка кому адресован пакет... (только если не включён режим отвечать на любые адреса)
		if( !(onBroadcast && rbuf.addr == BroadcastAddr) && !checkAddr(vaddr, rbuf.addr) )
		{
			if( dlog->is_warn() )
			{
				ostringstream err;
				err << "(recv): BadNodeAddress. my= " << vaddr2str(vaddr)
					<< " msg.addr=" << addr2str(rbuf.addr);
				dlog->warn() << err.str() << endl;
			}

			cleanupChannel();
			return erBadReplyNodeAddress;
		}

		return recv_pdu(rbuf, timeout);
	}
	catch( UniSetTypes::TimeOut )
	{
		//        cout << "(recv): catch TimeOut " << endl;
	}
	catch( const Exception& ex ) // SystemError
	{
		dlog->crit() << "(recv): " << ex << endl;
		cleanupChannel();
		return erHardwareError;
	}

	return erTimeOut;
}

// -------------------------------------------------------------------------
mbErrCode ModbusServer::recv_pdu( ModbusMessage& rbuf, timeout_t timeout )
{
	int bcnt = 1; // 1 - addr

	try
	{
		// -----------------------------------
		tmProcessing.setTiming(replyTimeout_ms);
		// recv func number
		size_t k = getNextData((unsigned char*)(&rbuf.func), sizeof(ModbusByte));

		if( k < sizeof(ModbusByte) )
		{
			dlog->warn() << "(recv): " << (ModbusHeader*)(&rbuf) << endl;
			dlog->warn() << "(recv): заголовок меньше положенного..." << endl;
			cleanupChannel();
			return erInvalidFormat;
		}

		bcnt += k;
		rbuf.len = 0;

		if( dlog->is_info() )
			dlog->info() << "(recv): header: " << rbuf << endl;

		// Определяем тип сообщения
		switch( rbuf.func )
		{
			case fnReadCoilStatus:
				rbuf.len = ReadCoilMessage::szData();

				if( crcNoCheckit )
					rbuf.len -= szCRC;

				break;

			case fnReadInputStatus:
				rbuf.len = ReadInputStatusMessage::szData();

				if( crcNoCheckit )
					rbuf.len -= szCRC;

				break;

			case fnReadOutputRegisters:
				rbuf.len = ReadOutputMessage::szData();

				if( crcNoCheckit )
					rbuf.len -= szCRC;

				break;

			case fnReadInputRegisters:
				rbuf.len = ReadInputMessage::szData();

				if( crcNoCheckit )
					rbuf.len -= szCRC;

				break;

			case fnForceMultipleCoils:
				rbuf.len = ForceCoilsMessage::szHead();
				break;

			case fnWriteOutputRegisters:
				rbuf.len = WriteOutputMessage::szHead();
				break;

			case fnForceSingleCoil:
				rbuf.len = ForceSingleCoilMessage::szHead();
				break;

			case fnWriteOutputSingleRegister:
				rbuf.len = WriteSingleOutputMessage::szHead();
				break;

			case fnDiagnostics:
				rbuf.len = DiagnosticMessage::szHead();
				break;

			case fnMEI:
				rbuf.len = MEIMessageRDI::szHead();
				break;

			case fnJournalCommand:
				rbuf.len = JournalCommandMessage::szData();

				if( crcNoCheckit )
					rbuf.len -= szCRC;

				break;

			case fnSetDateTime:
				rbuf.len = SetDateTimeMessage::szData();

				if( crcNoCheckit )
					rbuf.len -= szCRC;

				break;

			case fnRemoteService:
				rbuf.len = RemoteServiceMessage::szHead();
				break;

			case fnFileTransfer:
				rbuf.len = FileTransferMessage::szData();

				if( crcNoCheckit )
					rbuf.len -= szCRC;

				break;

			default:
				cleanupChannel();
				return erUnExpectedPacketType;
		}

		// ДЛЯ ТОГО ЧТОБЫ НЕ ЖДАТЬ ПРОДОЛЖЕНИЯ БЕЗКОНЕЧНО СБРАСЫВАЕМ TIMEOUT
		setChannelTimeout(10); // 10 msec

		// Получаем остальную часть сообщения
		size_t rlen = getNextData((unsigned char*)(rbuf.data), rbuf.len);

		if( rlen < rbuf.len )
		{
			if( dlog->is_warn() )
			{
				//                rbuf.len = bcnt + rlen - szModbusHeader;
				dlog->warn() << "(recv): buf: " << rbuf << endl;
				dlog->warn() << "(recv)(" << rbuf.func
							 << "): Получили данных меньше чем ждали...(recv="
							 << rlen << " < wait=" << (int)rbuf.len << ")" << endl;
			}

			cleanupChannel();
			return erInvalidFormat;
		}

		bcnt += rlen;

		// получаем остальное...
		if( rbuf.func == fnReadCoilStatus )
		{
			ReadCoilMessage mRead(rbuf);

			if( dlog->is_info() )
				dlog->info() << "(0x01): recv buf: " << rbuf << endl;

			// Проверяем контрольную сумму
			// от начала(включая заголовок) и до конца (исключив последний элемент содержащий CRC)
			if( crcNoCheckit )
				return erNoError;

			ModbusData tcrc = checkCRC((ModbusByte*)(&rbuf), bcnt - szCRC);

			if( tcrc != mRead.crc )
			{
				if( dlog->is_warn() )
				{
					ostringstream err;
					err << "(0x01): bad crc. calc.crc=" << dat2str(tcrc)
						<< " msg.crc=" << dat2str(mRead.crc);
					dlog->warn() << err.str() << endl;
				}

				cleanupChannel();
				return erBadCheckSum;
			}

			return erNoError;
		}
		else if( rbuf.func == fnReadInputStatus )
		{
			ReadInputStatusMessage mRead(rbuf);

			if( dlog->is_info() )
				dlog->info() << "(r0x02): recv buf: " << rbuf << endl;

			// Проверяем контрольную сумму
			// от начала(включая заголовок) и до конца (исключив последний элемент содержащий CRC)
			if( crcNoCheckit )
				return erNoError;

			ModbusData tcrc = checkCRC((ModbusByte*)(&rbuf), bcnt - szCRC);

			if( tcrc != mRead.crc )
			{
				if( dlog->is_warn() )
				{
					ostringstream err;
					err << "(0x02): bad crc. calc.crc=" << dat2str(tcrc)
						<< " msg.crc=" << dat2str(mRead.crc);
					dlog->warn() << err.str() << endl;
				}

				cleanupChannel();
				return erBadCheckSum;
			}

			return erNoError;
		}
		else if( rbuf.func == fnReadOutputRegisters )
		{
			ReadOutputMessage mRead(rbuf);

			if( dlog->is_info() )
				dlog->info() << "(0x03): recv buf: " << rbuf << endl;

			if( crcNoCheckit )
				return erNoError;

			// Проверяем контрольную сумму
			// от начала(включая заголовок) и до конца (исключив последний элемент содержащий CRC)
			ModbusData tcrc = checkCRC((ModbusByte*)(&rbuf), bcnt - szCRC);

			if( tcrc != mRead.crc )
			{
				if( dlog->is_warn() )
				{
					ostringstream err;
					err << "(0x03): bad crc. calc.crc=" << dat2str(tcrc)
						<< " msg.crc=" << dat2str(mRead.crc);
					dlog->warn() << err.str() << endl;
				}

				cleanupChannel();
				return erBadCheckSum;
			}

			return erNoError;
		}
		else if( rbuf.func == fnReadInputRegisters )
		{
			ReadInputMessage mRead(rbuf);

			if( dlog->is_info() )
				dlog->info() << "(0x04): recv buf: " << rbuf << endl;

			if( crcNoCheckit )
				return erNoError;

			// Проверяем контрольную сумму
			// от начала(включая заголовок) и до конца (исключив последний элемент содержащий CRC)
			ModbusData tcrc = checkCRC((ModbusByte*)(&rbuf), bcnt - szCRC);

			if( tcrc != mRead.crc )
			{
				if( dlog->is_warn() )
				{
					ostringstream err;
					err << "(0x04): bad crc. calc.crc=" << dat2str(tcrc)
						<< " msg.crc=" << dat2str(mRead.crc);
					dlog->warn() << err.str() << endl;
				}

				cleanupChannel();
				return erBadCheckSum;
			}

			return erNoError;
		}
		else if( rbuf.func == fnForceMultipleCoils )
		{
			int szDataLen = ForceCoilsMessage::getDataLen(rbuf) + szCRC;

			if( crcNoCheckit )
				szDataLen -= szCRC;

			// Мы получили только предварительный загловок
			// Теперь необходимо дополучить данные
			// (c позиции rlen, т.к. часть уже получили)
			int rlen1 = getNextData((unsigned char*)(&(rbuf.data[rlen])), szDataLen);

			if( rlen1 < szDataLen )
			{
				if( dlog->is_warn() )
				{
					rbuf.len = bcnt + rlen1 - szModbusHeader;
					dlog->warn() << "(0x0F): buf: " << rbuf << endl;
					dlog->warn() << "(0x0F)("
								 << rbuf.func << "):(fnForceMultipleCoils) "
								 << "Получили данных меньше чем ждали...("
								 << rlen1 << " < " << szDataLen << ")" << endl;
				}

				cleanupChannel();
				return erInvalidFormat;
			}

			bcnt += rlen1;
			rbuf.len = bcnt - szModbusHeader;

			ForceCoilsMessage mWrite(rbuf);

			if( dlog->is_info() )
				dlog->info() << "(0x0F): recv buf: " << rbuf << endl;

			if( !crcNoCheckit )
			{
				// Проверяем контрольную сумму
				// от начала(включая заголовок)
				// и до конца (исключив последний элемент содержащий CRC)
				// int mlen = szModbusHeader + mWrite.szHead()+ mWrite.bcnt;
				ModbusData tcrc = checkCRC((ModbusByte*)(&rbuf), bcnt - szCRC);

				if( tcrc != mWrite.crc )
				{
					ostringstream err;
					err << "(0x0F): bad crc. calc.crc=" << dat2str(tcrc)
						<< " msg.crc=" << dat2str(mWrite.crc);
					dlog->warn() << err.str() << endl;
					cleanupChannel();
					return erBadCheckSum;
				}
			}

			if( !mWrite.checkFormat() )
			{
				if( dlog->is_warn() )
				{
					dlog->warn() << "(0x0F): (" << rbuf.func
								 << ")(fnForceMultipleCoils): "
								 << ": некорректный формат сообщения..." << endl;
				}

				cleanupChannel();
				return erInvalidFormat;
			}

			return erNoError;
		}
		else if( rbuf.func == fnWriteOutputRegisters )
		{
			int szDataLen = WriteOutputMessage::getDataLen(rbuf) + szCRC;

			if( crcNoCheckit )
				szDataLen -= szCRC;

			// Мы получили только предварительный загловок
			// Теперь необходимо дополучить данные
			// (c позиции rlen, т.к. часть уже получили)
			int rlen1 = getNextData((unsigned char*)(&(rbuf.data[rlen])), szDataLen);

			if( rlen1 < szDataLen )
			{
				if( dlog->is_warn() )
				{
					rbuf.len = bcnt + rlen1 - szModbusHeader;
					dlog->warn() << "(0x10): buf: " << rbuf << endl;
					dlog->warn() << "(0x10)("
								 << rbuf.func << "):(fnWriteOutputRegisters) "
								 << "Получили данных меньше чем ждали...("
								 << rlen1 << " < " << szDataLen << ")" << endl;
				}

				cleanupChannel();
				return erInvalidFormat;
			}

			bcnt += rlen1;
			rbuf.len = bcnt - szModbusHeader;

			WriteOutputMessage mWrite(rbuf);

			if( dlog->is_info() )
				dlog->info() << "(0x10): recv buf: " << rbuf << endl;

			if( !crcNoCheckit )
			{
				// Проверяем контрольную сумму
				// от начала(включая заголовок)
				// и до конца (исключив последний элемент содержащий CRC)
				// int mlen = szModbusHeader + mWrite.szHead()+ mWrite.bcnt;
				ModbusData tcrc = checkCRC((ModbusByte*)(&rbuf), bcnt - szCRC);

				if( tcrc != mWrite.crc )
				{
					if( dlog->is_warn() )
					{
						ostringstream err;
						err << "(0x10): bad crc. calc.crc=" << dat2str(tcrc)
							<< " msg.crc=" << dat2str(mWrite.crc);
						dlog->warn() << err.str() << endl;
					}

					cleanupChannel();
					return erBadCheckSum;
				}
			}

			if( !mWrite.checkFormat() )
			{
				dlog->warn() << "(0x10): (" << rbuf.func
							 << ")(fnWriteOutputRegisters): "
							 << ": некорректный формат сообщения..." << endl;
				cleanupChannel();
				return erInvalidFormat;
			}

			return erNoError;
		}
		else if( rbuf.func == fnForceSingleCoil )
		{
			int szDataLen = ForceSingleCoilMessage::getDataLen(rbuf) + szCRC;

			if( crcNoCheckit )
				szDataLen -= szCRC;

			// Мы получили только предварительный загловок
			// Теперь необходимо дополучить данные
			// (c позиции rlen, т.к. часть уже получили)
			int rlen1 = getNextData((unsigned char*)(&(rbuf.data[rlen])), szDataLen);

			if( rlen1 < szDataLen )
			{
				if( dlog->is_warn() )
				{
					rbuf.len = bcnt + rlen1 - szModbusHeader;
					dlog->warn() << "(0x05): buf: " << rbuf << endl;
					dlog->warn() << "(0x05)("
								 << rbuf.func << "):(fnForceSingleCoil) "
								 << "Получили данных меньше чем ждали...("
								 << rlen1 << " < " << szDataLen << ")" << endl;
				}

				cleanupChannel();
				return erInvalidFormat;
			}

			bcnt += rlen1;
			rbuf.len = bcnt - szModbusHeader;

			ForceSingleCoilMessage mWrite(rbuf);

			if( dlog->is_info() )
				dlog->info() << "(0x05): recv buf: " << rbuf << endl;

			if( !crcNoCheckit )
			{
				// Проверяем контрольную сумму
				// от начала(включая заголовок)
				// и до конца (исключив последний элемент содержащий CRC)
				// int mlen = szModbusHeader + mWrite.szHead() + mWrite.bcnt;
				ModbusData tcrc = checkCRC((ModbusByte*)(&rbuf), bcnt - szCRC);

				if( tcrc != mWrite.crc )
				{
					if( dlog->is_warn() )
					{
						ostringstream err;
						err << "(0x05): bad crc. calc.crc=" << dat2str(tcrc)
							<< " msg.crc=" << dat2str(mWrite.crc);
						dlog->warn() << err.str() << endl;
					}

					cleanupChannel();
					return erBadCheckSum;
				}
			}

			if( !mWrite.checkFormat() )
			{
				dlog->warn() << "(0x05): (" << rbuf.func
							 << ")(fnForceSingleCoil): "
							 << ": некорректный формат сообщения..." << endl;
				cleanupChannel();
				return erInvalidFormat;
			}

			return erNoError;
		}
		else if( rbuf.func == fnWriteOutputSingleRegister )
		{
			int szDataLen = WriteSingleOutputMessage::getDataLen(rbuf) + szCRC;

			if( crcNoCheckit )
				szDataLen -= szCRC;

			// Мы получили только предварительный загловок
			// Теперь необходимо дополучить данные
			// (c позиции rlen, т.к. часть уже получили)
			int rlen1 = getNextData((unsigned char*)(&(rbuf.data[rlen])), szDataLen);

			if( rlen1 < szDataLen )
			{
				if( dlog->is_warn() )
				{
					rbuf.len = bcnt + rlen1 - szModbusHeader;
					dlog->warn() << "(0x06): buf: " << rbuf << endl;
					dlog->warn() << "(0x06)("
								 << rbuf.func << "):(fnWriteOutputSingleRegisters) "
								 << "Получили данных меньше чем ждали...("
								 << rlen1 << " < " << szDataLen << ")" << endl;
				}

				cleanupChannel();
				return erInvalidFormat;
			}

			bcnt += rlen1;
			rbuf.len = bcnt - szModbusHeader;

			WriteSingleOutputMessage mWrite(rbuf);

			if( dlog->is_info() )
				dlog->info() << "(0x06): recv buf: " << rbuf << endl;

			if( !crcNoCheckit )
			{
				// Проверяем контрольную сумму
				// от начала(включая заголовок)
				// и до конца (исключив последний элемент содержащий CRC)
				// int mlen = szModbusHeader + mWrite.szHead() + mWrite.bcnt;
				ModbusData tcrc = checkCRC((ModbusByte*)(&rbuf), bcnt - szCRC);

				if( tcrc != mWrite.crc )
				{
					if( dlog->is_warn() )
					{
						ostringstream err;
						err << "(0x06): bad crc. calc.crc=" << dat2str(tcrc)
							<< " msg.crc=" << dat2str(mWrite.crc);
						dlog->warn() << err.str() << endl;
					}

					cleanupChannel();
					return erBadCheckSum;
				}
			}

			if( !mWrite.checkFormat() )
			{
				dlog->warn() << "(0x06): (" << rbuf.func
							 << ")(fnWriteOutputSingleRegisters): "
							 << ": некорректный формат сообщения..." << endl;
				cleanupChannel();
				return erInvalidFormat;
			}

			return erNoError;
		}
		else if( rbuf.func == fnDiagnostics )
		{
			int szDataLen = DiagnosticMessage::getDataLen(rbuf) + szCRC;

			if( crcNoCheckit )
				szDataLen -= szCRC;

			// Мы получили только предварительный загловок
			// Теперь необходимо дополучить данные
			// (c позиции rlen, т.к. часть уже получили)
			int rlen1 = getNextData((unsigned char*)(&(rbuf.data[rlen])), szDataLen);

			if( rlen1 < szDataLen )
			{
				if( dlog->is_warn() )
				{
					rbuf.len = bcnt + rlen1 - szModbusHeader;
					dlog->warn() << "(0x08): buf: " << rbuf << endl;
					dlog->warn() << "(0x08)("
								 << rbuf.func << "):(fnDiagnostics) "
								 << "Получили данных меньше чем ждали...("
								 << rlen1 << " < " << szDataLen << ")" << endl;
				}

				cleanupChannel();
				return erInvalidFormat;
			}

			bcnt += rlen1;
			rbuf.len = bcnt - szModbusHeader;

			DiagnosticMessage mDiag(rbuf);

			if( dlog->is_info() )
				dlog->info() << "(0x08): recv buf: " << rbuf << endl;

			if( !crcNoCheckit )
			{
				// Проверяем контрольную сумму
				// от начала(включая заголовок)
				// и до конца (исключив последний элемент содержащий CRC)
				// int mlen = szModbusHeader + mWrite.szHead() + mWrite.bcnt;
				ModbusData tcrc = checkCRC((ModbusByte*)(&rbuf), bcnt - szCRC);

				if( tcrc != mDiag.crc )
				{
					if( dlog->is_warn() )
					{
						ostringstream err;
						err << "(0x08): bad crc. calc.crc=" << dat2str(tcrc)
							<< " msg.crc=" << dat2str(mDiag.crc);
						dlog->warn() << err.str() << endl;
					}

					cleanupChannel();
					return erBadCheckSum;
				}
			}

			/*
			            if( !mDiag.checkFormat() )
			            {
			                dlog->warn() << "(0x08): (" << rbuf.func
			                    << ")(fnDiagnostics): "
			                    << ": некорректный формат сообщения..." << endl;
			                cleanupChannel();
			                return erInvalidFormat;
			            }
			*/
			return erNoError;
		}
		else if( rbuf.func == fnMEI )
		{
			if( !crcNoCheckit )
			{
				int rlen1 = getNextData((unsigned char*)(&(rbuf.data[rlen])), szCRC);

				if( rlen1 < szCRC )
				{
					if( dlog->is_warn() )
					{
						rbuf.len = bcnt + rlen1 - szModbusHeader;
						dlog->warn() << "(0x2B/0x0E): buf: " << rbuf << endl;
						dlog->warn() << "(0x2B/0x0E)("
									 << rbuf.func << "):(fnMEI) "
									 << "Получили данных меньше чем ждали...("
									 << rlen1 << " < " << szCRC << ")" << endl;
					}

					cleanupChannel();
					return erInvalidFormat;
				}

				bcnt += rlen1;
				rbuf.len = bcnt - szModbusHeader;
			}

			MEIMessageRDI mRDI(rbuf);

			if( dlog->is_info() )
				dlog->info() << "(0x2B/0x0E): recv buf: " << rbuf << endl;

			if( crcNoCheckit )
				return erNoError;

			// Проверяем контрольную сумму
			// от начала(включая заголовок)
			// и до конца (исключив последний элемент содержащий CRC)
			// int mlen = szModbusHeader + mWrite.szHead() + mWrite.bcnt;
			ModbusData tcrc = checkCRC((ModbusByte*)(&rbuf), bcnt - szCRC);

			if( tcrc != mRDI.crc )
			{
				if( dlog->is_warn() )
				{
					ostringstream err;
					err << "(0x2B/0x0E): bad crc. calc.crc=" << dat2str(tcrc)
						<< " msg.crc=" << dat2str(mRDI.crc);
					dlog->warn() << err.str() << endl;
				}

				cleanupChannel();
				return erBadCheckSum;
			}

			return erNoError;
		}
		else if( rbuf.func == fnJournalCommand )
		{
			JournalCommandMessage mRead(rbuf);

			if( dlog->is_info() )
				dlog->info() << "(0x65): recv buf: " << rbuf << endl;

			if( crcNoCheckit )
				return erNoError;

			// Проверяем контрольную сумму
			// от начала(включая заголовок) и до конца (исключив последний элемент содержащий CRC)
			// ModbusData tcrc = checkCRC((ModbusByte*)(&rbuf),sizeof(ReadOutputMessage)-szCRC);
			ModbusData tcrc = checkCRC((ModbusByte*)(&rbuf), bcnt - szCRC);

			if( tcrc != mRead.crc )
			{
				if( dlog->is_warn() )
				{
					ostringstream err;
					err << "(0x65): bad crc. calc.crc=" << dat2str(tcrc)
						<< " msg.crc=" << dat2str(mRead.crc);
					dlog->warn() << err.str() << endl;
				}

				cleanupChannel();
				return erBadCheckSum;
			}

			return erNoError;
		}
		else if( rbuf.func == fnSetDateTime )
		{
			SetDateTimeMessage mSet(rbuf);

			if( dlog->is_info() )
				dlog->info() << "(0x50): recv buf: " << rbuf << endl;

			if( !crcNoCheckit )
			{
				// Проверяем контрольную сумму
				// от начала(включая заголовок) и до конца (исключив последний элемент содержащий CRC)
				// ModbusData tcrc = checkCRC((ModbusByte*)(&rbuf),sizeof(ReadOutputMessage)-szCRC);
				ModbusData tcrc = checkCRC((ModbusByte*)(&rbuf), bcnt - szCRC);

				if( tcrc != mSet.crc )
				{
					if( dlog->is_warn() )
					{
						ostringstream err;
						err << "(0x50): bad crc. calc.crc=" << dat2str(tcrc)
							<< " msg.crc=" << dat2str(mSet.crc);
						dlog->warn() << err.str() << endl;
					}

					cleanupChannel();
					return erBadCheckSum;
				}
			}

			if( !mSet.checkFormat() )
			{
				dlog->warn() << "(0x50): некорректные значения..." << endl;
				cleanupChannel();
				return erBadDataValue; // return erInvalidFormat;
			}

			return erNoError;
		}
		else if( rbuf.func == fnRemoteService )
		{
			int szDataLen = RemoteServiceMessage::getDataLen(rbuf) + szCRC;

			if( crcNoCheckit )
				szDataLen -= szCRC;

			// Мы получили только предварительный загловок
			// Теперь необходимо дополучить данные
			// (c позиции rlen, т.к. часть уже получили)
			int rlen1 = getNextData((unsigned char*)(&(rbuf.data[rlen])), szDataLen);

			if( rlen1 < szDataLen )
			{
				if( dlog->is_warn() )
				{
					rbuf.len = bcnt + rlen1 - szModbusHeader;
					dlog->warn() << "(0x53): buf: " << rbuf << endl;
					dlog->warn() << "(0x53)("
								 << rbuf.func << "):(fnWriteOutputRegisters) "
								 << "Получили данных меньше чем ждали...("
								 << rlen1 << " < " << szDataLen << ")" << endl;
				}

				cleanupChannel();
				return erInvalidFormat;
			}

			bcnt += rlen1;
			rbuf.len = bcnt - szModbusHeader;

			RemoteServiceMessage mRServ(rbuf);

			if( dlog->is_info() )
				dlog->info() << "(0x53): recv buf: " << rbuf << endl;

			if( crcNoCheckit )
				return erNoError;

			// Проверяем контрольную сумму
			// от начала(включая заголовок)
			// и до конца (исключив последний элемент содержащий CRC)
			// int mlen = szModbusHeader + mWrite.szHead()+ mWrite.bcnt;
			ModbusData tcrc = checkCRC((ModbusByte*)(&rbuf), bcnt - szCRC);

			if( tcrc != mRServ.crc )
			{
				if( dlog->is_warn() )
				{
					ostringstream err;
					err << "(0x53): bad crc. calc.crc=" << dat2str(tcrc)
						<< " msg.crc=" << dat2str(mRServ.crc);
					dlog->warn() << err.str() << endl;
				}

				cleanupChannel();
				return erBadCheckSum;
			}

			return erNoError;
		}
		else if( rbuf.func == fnFileTransfer )
		{
			FileTransferMessage mFT(rbuf);

			if( dlog->is_info() )
				dlog->info() << "(0x66): recv buf: " << rbuf << endl;

			if( crcNoCheckit )
				return erNoError;

			// Проверяем контрольную сумму
			// от начала(включая заголовок)
			// и до конца (исключив последний элемент содержащий CRC)
			// int mlen = szModbusHeader + mWrite.szHead()+ mWrite.bcnt;
			ModbusData tcrc = checkCRC((ModbusByte*)(&rbuf), bcnt - szCRC);

			if( tcrc != mFT.crc )
			{
				if( dlog->is_warn() )
				{
					ostringstream err;
					err << "(0x66): bad crc. calc.crc=" << dat2str(tcrc)
						<< " msg.crc=" << dat2str(mFT.crc);
					dlog->warn() << err.str() << endl;
				}

				cleanupChannel();
				return erBadCheckSum;
			}

			return erNoError;
		}
		else
		{
			// А как мы сюда добрались?!!!!!!
			return erUnExpectedPacketType;
		}
	}
	catch( ModbusRTU::mbException& ex ) // SystemError
	{
		if( dlog->debugging(Debug::CRIT) )
			dlog->crit() << "(recv): mbException: " << ex << endl;

		cleanupChannel();
		return ex.err;
	}
	catch( UniSetTypes::TimeOut )
	{
		//        cout << "(recv): catch TimeOut " << endl;
	}
	catch( UniSetTypes::Exception& ex ) // SystemError
	{
		if( dlog->debugging(Debug::CRIT) )
			dlog->crit() << "(recv): " << ex << endl;

		cleanupChannel();
		return erHardwareError;
	}

	return erTimeOut;
}
// -------------------------------------------------------------------------
void ModbusServer::setLog( std::shared_ptr<DebugStream> l )
{
	this->dlog = l;
}

std::unordered_set<ModbusAddr> ModbusServer::addr2vaddr(ModbusAddr& mbaddr)
{
	std::unordered_set<ModbusRTU::ModbusAddr> v;
	v.emplace(mbaddr);
	return std::move(v);
}
// -------------------------------------------------------------------------
mbErrCode ModbusServer::receive(const std::unordered_set<ModbusAddr>& vaddr, timeout_t msecTimeout)
{
	// приходиться делать специальное исключение для кода erSessionClosed
	// т.к. это означает, что клиент ничего не посылал (read=0) и отвалился (закрыл канал).
	// т.е. собственно события receive и не было..
	// Это актуально для TCPServe-а.. (но вообще это получается какой-то архитектурный изъян)

	mbErrCode ret = erNoError;

	if( !m_pre_signal.empty() )
	{
		ret = m_pre_signal.emit(vaddr,msecTimeout);
		if( ret != erNoError && ret != erSessionClosed )
		{
			errmap[ret] +=1;
			return ret;
		}
	}

	ret = realReceive(vaddr,msecTimeout);

	// собираем статистику..
	if( ret != erTimeOut && ret != erSessionClosed )
		askCount++;

	if( ret != erNoError && ret != erSessionClosed )
		errmap[ret] +=1;

	if( ret != erSessionClosed )
		m_post_signal.emit(ret);

	return ret;
}
// -------------------------------------------------------------------------
mbErrCode ModbusServer::receive_one( ModbusAddr a, timeout_t msec )
{
	auto v = addr2vaddr(a);
	return receive(v, msec);
}
// -------------------------------------------------------------------------
ModbusServer::PreReceiveSignal ModbusServer::signal_pre_receive()
{
	return m_pre_signal;
}
// -------------------------------------------------------------------------
ModbusServer::PostReceiveSignal ModbusServer::signal_post_receive()
{
	return m_post_signal;
}
// -------------------------------------------------------------------------
void ModbusServer::initLog( UniSetTypes::Configuration* conf,
							const std::string& lname, const string& logfile )
{
	conf->initLogStream(dlog, lname);

	if( !logfile.empty() )
		dlog->logFile( logfile );
}
// -------------------------------------------------------------------------
void ModbusServer::printProcessingTime()
{
	if( dlog->is_info() )
		dlog->info() << "(processingTime): " << tmProcessing.getCurrent() << " [msec] (lim: " << tmProcessing.getInterval() << ")" << endl;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode ModbusServer::replyFileTransfer( const std::string& fname,
		ModbusRTU::FileTransferMessage& query,
		ModbusRTU::FileTransferRetMessage& reply,
		std::shared_ptr<DebugStream> dlog )
{
	if( dlog && dlog->is_info() )
		dlog->info() << "(replyFileTransfer): " << query << endl;

	int fd = open(fname.c_str(), O_RDONLY | O_NONBLOCK );

	if( fd < 0 )
	{
		if( dlog && dlog->is_warn() )
			(*dlog)[Debug::WARN] << "(replyFileTransfer): open '" << fname << "' with error: " << strerror(errno) << endl;

		return ModbusRTU::erOperationFailed;
	}

	int seek = query.numpacket * ModbusRTU::FileTransferRetMessage::MaxDataLen;
	int ret = lseek(fd, seek, SEEK_SET);

	if( ret == -1 )
	{
		close(fd);

		if( dlog && dlog->is_warn() )
			(*dlog)[Debug::WARN] << "(replyFileTransfer): open '" << fname << "' with error: " << strerror(errno) << endl;

		return ModbusRTU::erOperationFailed;
	}

	ModbusRTU::ModbusByte buf[ModbusRTU::FileTransferRetMessage::MaxDataLen];

	ret = ::read(fd, &buf, sizeof(buf));

	if( ret < 0 )
	{
		close(fd);

		if( dlog && dlog->is_warn() )
			(*dlog)[Debug::WARN] << "(replyFileTransfer): read from '" << fname << "' with error: " << strerror(errno) << endl;

		return ModbusRTU::erOperationFailed;
	}

	struct stat fs;

	if( fstat(fd, &fs) < 0 )
	{
		close(fd);

		if( dlog && dlog->is_warn() )
			(*dlog)[Debug::WARN] << "(replyFileTransfer): fstat for '" << fname << "' with error: " << strerror(errno) << endl;

		return ModbusRTU::erOperationFailed;
	}

	close(fd);

	int numpacks = fs.st_size / ModbusRTU::FileTransferRetMessage::MaxDataLen;

	if( fs.st_size % ModbusRTU::FileTransferRetMessage::MaxDataLen )
		numpacks++;

	if( !reply.set(query.numfile, numpacks, query.numpacket, buf, ret) )
	{
		if( dlog && dlog->is_warn() )
			(*dlog)[Debug::WARN] << "(replyFileTransfer): set date failed..." << endl;

		return ModbusRTU::erOperationFailed;
	}

	return ModbusRTU::erNoError;
}
// -------------------------------------------------------------------------
ModbusServer::ExchangeErrorMap ModbusServer::getErrorMap()
{
	ExchangeErrorMap m(errmap);
	return std::move(m);
}
// -------------------------------------------------------------------------
size_t ModbusServer::getErrCount( mbErrCode e )
{
	auto i = errmap.find(e);
	if( i == errmap.end() )
		return 0;

	return i->second;
}
// -------------------------------------------------------------------------
size_t ModbusServer::resetErrCount( mbErrCode e, size_t set )
{
	auto i = errmap.find(e);
	if( i == errmap.end() )
		return 0;

	size_t ret = i->second;
	i->second = set;
	return ret;
}
// -------------------------------------------------------------------------
void ModbusServer::resetAskCounter()
{
	askCount = 0;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode ModbusServer::replySetDateTime( ModbusRTU::SetDateTimeMessage& query,
		ModbusRTU::SetDateTimeRetMessage& reply,
		std::shared_ptr<DebugStream> dlog )
{
	if( dlog && dlog->is_info() )
		dlog->info() << "(replySetDateTime): " << query << endl;

	struct timeval set;
	struct timezone tz;

	if( gettimeofday(&set, &tz) == 0 )
	{
		struct tm  t;
		localtime_r(&set.tv_sec, &t);
		t.tm_sec     = query.sec;
		t.tm_min     = query.min;
		t.tm_hour     = query.hour;
		t.tm_mday     = query.day;
		t.tm_mon     = query.mon - 1;
		//        t.tm_year     = (query.century>19) ? query.year + query.century*10 - 1900 : query.year;
		t.tm_year     = (query.century > 19) ? query.year + 2000 - 1900 : query.year;
		set.tv_sec = mktime(&t); // может вернуть -1 (!)
		set.tv_usec = 0;

		if( set.tv_sec >= 0 && settimeofday(&set, &tz) == 0 )
		{
			// подтверждаем сохранение
			// в ответе возвращаем установленное время...
			ModbusRTU::SetDateTimeRetMessage::cpy(reply, query);
			return ModbusRTU::erNoError;
		}
		else if( dlog && dlog->is_warn() )
			(*dlog)[Debug::WARN] << "(replySetDateTime): settimeofday err: " << strerror(errno) << endl;
	}
	else if( dlog && dlog->is_warn() )
		(*dlog).warn() << "(replySetDateTime): gettimeofday err: " << strerror(errno) << endl;

	return ModbusRTU::erOperationFailed;
}
// -------------------------------------------------------------------------
mbErrCode ModbusServer::send( ModbusMessage& msg )
{
	if( cleanBeforeSend )
		cleanupChannel();

	mbErrCode ret = pre_send_request(msg);

	if( ret != erNoError )
		return ret;

	if( msg.len > MAXLENPACKET + szModbusHeader )
	{
		if( dlog->is_warn() )
			dlog->warn() << "(send): длина пакета больше разрешённой..." << endl;

		return erPacketTooLong;
	}

	if( tmProcessing.checkTime() )
	{
		if( dlog->is_warn() )
			dlog->warn() << "(send): reply timeout(" << tmProcessing.getInterval() << ")...!!!" << endl;

		return erTimeOut;
	}

	int len = szModbusHeader + msg.len;

	if( crcNoCheckit )
	{
		msg.len -= szCRC;
		len -= szCRC;
	}


	if( dlog->is_info() )
		dlog->info() << "(send): data(" << len << " bytes): " << msg << endl;

	try
	{
		sendData((unsigned char*)(&msg), len);
	}
	catch( const Exception& ex ) // SystemError
	{
		if( dlog->is_crit() )
			dlog->crit() << "(send): " << ex << endl;

		return erHardwareError;
	}

	if( aftersend_msec > 0 )
		msleep(aftersend_msec);

	return post_send_request(msg);
}

// -------------------------------------------------------------------------
