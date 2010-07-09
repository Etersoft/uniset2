/*! $Id: ModbusServer.cc,v 1.3 2008/11/23 22:47:29 vpashka Exp $ */
// -------------------------------------------------------------------------
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
	recvTimeOut_ms(50),
	replyTimeout_ms(2000),
	aftersend_msec(0),
	onBroadcast(false),
	crcNoCheckit(false)
{
	tmProcessing.setTiming(replyTimeout_ms);
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
mbErrCode ModbusServer::processing( ModbusMessage& buf )
{
	if( buf.func == fnReadCoilStatus )
	{
		ReadCoilMessage mRead(buf);
		ReadCoilRetMessage reply(buf.addr); // addr?
		// вызываем обработчик..
		mbErrCode res = readCoilStatus( mRead,reply );
		// в случае ошибок ответа не посылаем
		if( res != erNoError )
		{
			dlog[Debug::WARN] << "(0x01): err reply: " << mbErr2Str(res) << endl;

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
		mbErrCode res = readInputStatus( mRead,reply );
		// в случае ошибок ответа не посылаем
		if( res != erNoError )
		{
			dlog[Debug::WARN] << "(0x02): err reply: " << mbErr2Str(res) << endl;

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
		mbErrCode res = readOutputRegisters( mRead,reply );
		// в случае ошибок ответа не посылаем
		if( res != erNoError )
		{
			dlog[Debug::WARN] << "(0x03): err reply: " << mbErr2Str(res) << endl;

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
		mbErrCode res = readInputRegisters( mRead,reply );
		// в случае ошибок ответа не посылаем
		if( res != erNoError )
		{
			dlog[Debug::WARN] << "(0x04): err reply: " << mbErr2Str(res) << endl;

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
		mbErrCode res = forceMultipleCoils( mWrite,reply );
		// в случае ошибок ответа не посылаем
		if( res != erNoError )
		{
			dlog[Debug::WARN] << "(0x0F): err reply: " << mbErr2Str(res) << endl;

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
//		return send(buf);
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
		mbErrCode res = writeOutputRegisters( mWrite,reply );
		// в случае ошибок ответа не посылаем
		if( res != erNoError )
		{
			dlog[Debug::WARN] << "(0x10): err reply: " << mbErr2Str(res) << endl;

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
//		return send(buf);
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
			dlog[Debug::WARN] << "(0x05): reply: " << mbErr2Str(res) << endl;

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
//		return send(buf);
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
			dlog[Debug::WARN] << "(0x06): reply: " << mbErr2Str(res) << endl;

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
//		return send(buf);
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
		mbErrCode res = journalCommand( mJournal,reply );
		// в случае ошибок ответа не посылаем
		if( res != erNoError )
		{
			dlog[Debug::WARN] << "(0x65): reply: " << mbErr2Str(res) << endl;

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
		mbErrCode res = setDateTime( mSet,reply );
		// в случае ошибок ответа не посылаем
		if( res != erNoError )
		{
			dlog[Debug::WARN] << "(0x50): reply: " << mbErr2Str(res) << endl;

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
		mbErrCode res = remoteService( query,reply );
		// в случае ошибок ответа не посылаем
		if( res != erNoError )
		{
			dlog[Debug::WARN] << "(0x53): error reply: " << mbErr2Str(res) << endl;

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
		mbErrCode res = fileTransfer( query,reply );
		// в случае ошибок ответа не посылаем
		if( res != erNoError )
		{
			dlog[Debug::WARN] << "(0x66): error reply: " << mbErr2Str(res) << endl;

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
mbErrCode ModbusServer::recv( ModbusRTU::ModbusAddr addr, ModbusMessage& rbuf, timeout_t timeout )
{
	assert(timeout);
	if( timeout == UniSetTimer::WaitUpTime )
		timeout = 15*60*1000; // используем просто большое время (15 минут), переведя его в наносекунды.

	setChannelTimeout(timeout);
	PassiveTimer tmAbort(timeout);

	// предварительно чистим буфер
	memset(&rbuf,0,sizeof(rbuf));
	int bcnt=0;	// receive bytes count
	
	try
	{
		// wait byte = myaddr || byte = broadcast
		bool begin = false;
		while( !tmAbort.checkTime() )
		{
			bcnt = getNextData((unsigned char*)(&rbuf),sizeof(ModbusAddr));
			if( bcnt>0 && ( rbuf.addr==addr || (onBroadcast && rbuf.addr==BroadcastAddr) ) )
			{
				begin = true;
				break;
			}
		}

		if( !begin )
			return erTimeOut;

#warning Может стоит всё-таки получать весь пакет, а проверять кому он адресован на уровне выше?!	
// Lav: конечно стоит, нам же надо буфер чистить
		// Проверка кому адресован пакет...
		if( rbuf.addr!=addr && rbuf.addr!=BroadcastAddr )
		{
			ostringstream err;
			err << "(recv): BadNodeAddress. my= " << addr2str(addr)
				<< " msg.addr=" << addr2str(rbuf.addr);
			dlog[Debug::WARN] << err.str() << endl;
			return erBadReplyNodeAddress;
		}

		return recv_pdu(rbuf,timeout);
	}	
	catch( UniSetTypes::TimeOut )
	{
//		cout << "(recv): catch TimeOut " << endl;
	}
	catch( Exception& ex ) // SystemError
	{
		dlog[Debug::CRIT] << "(recv): " << ex << endl;
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
		size_t k = getNextData((unsigned char*)(&rbuf.func),sizeof(ModbusByte));
		if( k < sizeof(ModbusByte) )
		{
			dlog[Debug::WARN] << "(recv): " << (ModbusHeader*)(&rbuf) << endl;
			dlog[Debug::WARN] << "(recv): заголовок меньше положенного..." << endl;
			return erInvalidFormat;
		}

		bcnt += k;
		rbuf.len = 0;
		
		if( dlog.debugging(Debug::INFO) )
			dlog[Debug::INFO] << "(recv): header: " << rbuf << endl;

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
				return erUnExpectedPacketType;
		}


		// ДЛЯ ТОГО ЧТОБЫ НЕ ЖДАТЬ ПРОДОЛЖЕНИЯ БЕЗКОНЕЧНО СБРАСЫВАЕМ TIMEOUT
		setChannelTimeout(10); // 10 msec

		// Получаем остальную часть сообщения
		int rlen = getNextData((unsigned char*)(rbuf.data),rbuf.len);
		if( rlen < rbuf.len )
		{
//			rbuf.len = bcnt + rlen - szModbusHeader;
			dlog[Debug::WARN] << "(recv): buf: " << rbuf << endl;
			dlog[Debug::WARN] << "(recv)(" << rbuf.func 
					<< "): Получили данных меньше чем ждали...(recv=" 
					<< rlen << " < wait=" << (int)rbuf.len << ")" << endl;
			return erInvalidFormat;
		}
		
		bcnt+=rlen;

		// получаем остальное...
		if( rbuf.func == fnReadCoilStatus )
		{
			ReadCoilMessage mRead(rbuf);
			if( dlog.debugging(Debug::INFO) )
				dlog[Debug::INFO] << "(0x01): recv buf: " << rbuf << endl;

			// Проверяем контрольную сумму
			// от начала(включая заголовок) и до конца (исключив последний элемент содержащий CRC)
			if( crcNoCheckit )
				return erNoError;

			ModbusData tcrc = checkCRC((ModbusByte*)(&rbuf),bcnt-szCRC);
			if( tcrc != mRead.crc )
			{
				ostringstream err;
				err << "(0x01): bad crc. calc.crc=" << dat2str(tcrc)
					<< " msg.crc=" << dat2str(mRead.crc);
				dlog[Debug::WARN] << err.str() << endl;
				return erBadCheckSum;
			}

			return erNoError;
		}
		else if( rbuf.func == fnReadInputStatus )
		{
			ReadInputStatusMessage mRead(rbuf);
			if( dlog.debugging(Debug::INFO) )
				dlog[Debug::INFO] << "(r0x02): recv buf: " << rbuf << endl;

			// Проверяем контрольную сумму
			// от начала(включая заголовок) и до конца (исключив последний элемент содержащий CRC)
			if( crcNoCheckit )
				return erNoError;

			ModbusData tcrc = checkCRC((ModbusByte*)(&rbuf),bcnt-szCRC);
			if( tcrc != mRead.crc )
			{
				ostringstream err;
				err << "(0x02): bad crc. calc.crc=" << dat2str(tcrc)
					<< " msg.crc=" << dat2str(mRead.crc);
				dlog[Debug::WARN] << err.str() << endl;
				return erBadCheckSum;
			}

			return erNoError;
		}
		else if( rbuf.func == fnReadOutputRegisters )
		{
			ReadOutputMessage mRead(rbuf);
			if( dlog.debugging(Debug::INFO) )
				dlog[Debug::INFO] << "(0x03): recv buf: " << rbuf << endl;

			if( crcNoCheckit )
					return erNoError;

			// Проверяем контрольную сумму
			// от начала(включая заголовок) и до конца (исключив последний элемент содержащий CRC)
			ModbusData tcrc = checkCRC((ModbusByte*)(&rbuf),bcnt-szCRC);
			if( tcrc != mRead.crc )
			{
				ostringstream err;
				err << "(0x03): bad crc. calc.crc=" << dat2str(tcrc)
					<< " msg.crc=" << dat2str(mRead.crc);
				dlog[Debug::WARN] << err.str() << endl;
				return erBadCheckSum;
			}

			return erNoError;
		}
		else if( rbuf.func == fnReadInputRegisters )
		{
			ReadInputMessage mRead(rbuf);
			if( dlog.debugging(Debug::INFO) )
				dlog[Debug::INFO] << "(0x04): recv buf: " << rbuf << endl;

			if( crcNoCheckit )
				return erNoError;

			// Проверяем контрольную сумму
			// от начала(включая заголовок) и до конца (исключив последний элемент содержащий CRC)
			ModbusData tcrc = checkCRC((ModbusByte*)(&rbuf),bcnt-szCRC);
			if( tcrc != mRead.crc )
			{
				ostringstream err;
				err << "(0x04): bad crc. calc.crc=" << dat2str(tcrc)
					<< " msg.crc=" << dat2str(mRead.crc);
				dlog[Debug::WARN] << err.str() << endl;
				return erBadCheckSum;
			}

			return erNoError;			
		}
		else if( rbuf.func == fnForceMultipleCoils )
		{
			int szDataLen = ForceCoilsMessage::getDataLen(rbuf)+szCRC;
			if( crcNoCheckit )
				szDataLen -= szCRC;
			
			// Мы получили только предварительный загловок
			// Теперь необходимо дополучить данные 
			// (c позиции rlen, т.к. часть уже получили)
			int rlen1 = getNextData((unsigned char*)(&(rbuf.data[rlen])),szDataLen);
			if( rlen1 < szDataLen )
			{
				rbuf.len = bcnt + rlen1 - szModbusHeader;
				dlog[Debug::WARN] << "(0x0F): buf: " << rbuf << endl;
				dlog[Debug::WARN] << "(0x0F)(" 
					<< rbuf.func << "):(fnForceMultipleCoils) "
					<< "Получили данных меньше чем ждали...(" 
					<< rlen1 << " < " << szDataLen << ")" << endl;

				return erInvalidFormat;
			}
			
			bcnt += rlen1;
			rbuf.len = bcnt - szModbusHeader;

			ForceCoilsMessage mWrite(rbuf);

			if( dlog.debugging(Debug::INFO) )
				dlog[Debug::INFO] << "(0x0F): recv buf: " << rbuf << endl;

			if( !crcNoCheckit )
			{
				// Проверяем контрольную сумму
				// от начала(включая заголовок) 
				// и до конца (исключив последний элемент содержащий CRC)
				// int mlen = szModbusHeader + mWrite.szHead()+ mWrite.bcnt;
				ModbusData tcrc = checkCRC((ModbusByte*)(&rbuf),bcnt-szCRC);
				if( tcrc != mWrite.crc )
				{
					ostringstream err;
					err << "(0x0F): bad crc. calc.crc=" << dat2str(tcrc)
						<< " msg.crc=" << dat2str(mWrite.crc);
					dlog[Debug::WARN] << err.str() << endl;
					return erBadCheckSum;
				}
			}

#warning Стоит ли здесь проверять
			if( !mWrite.checkFormat() )
			{
				dlog[Debug::WARN] << "(0x0F): (" << rbuf.func 
					<< ")(fnForceMultipleCoils): "
					<< ": некорректный формат сообщения..." << endl; 
				return erInvalidFormat;
			}

			return erNoError;			
		}
		else if( rbuf.func == fnWriteOutputRegisters )
		{
			int szDataLen = WriteOutputMessage::getDataLen(rbuf)+szCRC;

			if( crcNoCheckit )
				szDataLen -= szCRC;
				
			// Мы получили только предварительный загловок
			// Теперь необходимо дополучить данные 
			// (c позиции rlen, т.к. часть уже получили)
			int rlen1 = getNextData((unsigned char*)(&(rbuf.data[rlen])),szDataLen);
			if( rlen1 < szDataLen )
			{
				rbuf.len = bcnt + rlen1 - szModbusHeader;
				dlog[Debug::WARN] << "(0x10): buf: " << rbuf << endl;
				dlog[Debug::WARN] << "(0x10)(" 
					<< rbuf.func << "):(fnWriteOutputRegisters) "
					<< "Получили данных меньше чем ждали...(" 
					<< rlen1 << " < " << szDataLen << ")" << endl;

				return erInvalidFormat;
			}
			
			bcnt += rlen1;
			rbuf.len = bcnt - szModbusHeader;

			WriteOutputMessage mWrite(rbuf);

			if( dlog.debugging(Debug::INFO) )
				dlog[Debug::INFO] << "(0x10): recv buf: " << rbuf << endl;

			if( !crcNoCheckit )
			{
				// Проверяем контрольную сумму
				// от начала(включая заголовок) 
				// и до конца (исключив последний элемент содержащий CRC)
				// int mlen = szModbusHeader + mWrite.szHead()+ mWrite.bcnt;
				ModbusData tcrc = checkCRC((ModbusByte*)(&rbuf),bcnt-szCRC);
				if( tcrc != mWrite.crc )
				{
					ostringstream err;
					err << "(0x10): bad crc. calc.crc=" << dat2str(tcrc)
						<< " msg.crc=" << dat2str(mWrite.crc);
					dlog[Debug::WARN] << err.str() << endl;
					return erBadCheckSum;
				}
			}
			
#warning Стоит ли здесь проверять
			if( !mWrite.checkFormat() )
			{
				dlog[Debug::WARN] << "(0x10): (" << rbuf.func 
					<< ")(fnWriteOutputRegisters): "
					<< ": некорректный формат сообщения..." << endl; 
				return erInvalidFormat;
			}

			return erNoError;			
		}
		else if( rbuf.func == fnForceSingleCoil )
		{
			int szDataLen = ForceSingleCoilMessage::getDataLen(rbuf)+szCRC;

			if( crcNoCheckit )
				szDataLen -= szCRC;
			
			// Мы получили только предварительный загловок
			// Теперь необходимо дополучить данные 
			// (c позиции rlen, т.к. часть уже получили)
			int rlen1 = getNextData((unsigned char*)(&(rbuf.data[rlen])),szDataLen);
			if( rlen1 < szDataLen )
			{
				rbuf.len = bcnt + rlen1 - szModbusHeader;
				dlog[Debug::WARN] << "(0x05): buf: " << rbuf << endl;
				dlog[Debug::WARN] << "(0x05)(" 
					<< rbuf.func << "):(fnForceSingleCoil) "
					<< "Получили данных меньше чем ждали...(" 
					<< rlen1 << " < " << szDataLen << ")" << endl;

				return erInvalidFormat;
			}
			
			bcnt += rlen1;
			rbuf.len = bcnt - szModbusHeader;

			ForceSingleCoilMessage mWrite(rbuf);

			if( dlog.debugging(Debug::INFO) )
				dlog[Debug::INFO] << "(0x05): recv buf: " << rbuf << endl;

			if( !crcNoCheckit )
			{
				// Проверяем контрольную сумму
				// от начала(включая заголовок) 
				// и до конца (исключив последний элемент содержащий CRC)
				// int mlen = szModbusHeader + mWrite.szHead() + mWrite.bcnt;
				ModbusData tcrc = checkCRC((ModbusByte*)(&rbuf),bcnt-szCRC);
				if( tcrc != mWrite.crc )
				{
					ostringstream err;
					err << "(0x05): bad crc. calc.crc=" << dat2str(tcrc)
						<< " msg.crc=" << dat2str(mWrite.crc);
					dlog[Debug::WARN] << err.str() << endl;
					return erBadCheckSum;
				}
			}
#warning Стоит ли здесь проверять
			if( !mWrite.checkFormat() )
			{
				dlog[Debug::WARN] << "(0x05): (" << rbuf.func 
					<< ")(fnForceSingleCoil): "
					<< ": некорректный формат сообщения..." << endl; 
				return erInvalidFormat;
			}

			return erNoError;			
		}
		else if( rbuf.func == fnWriteOutputSingleRegister )
		{
			int szDataLen = WriteSingleOutputMessage::getDataLen(rbuf)+szCRC;

			if( crcNoCheckit )
				szDataLen -= szCRC;
			
			// Мы получили только предварительный загловок
			// Теперь необходимо дополучить данные 
			// (c позиции rlen, т.к. часть уже получили)
			int rlen1 = getNextData((unsigned char*)(&(rbuf.data[rlen])),szDataLen);
			if( rlen1 < szDataLen )
			{
				rbuf.len = bcnt + rlen1 - szModbusHeader;
				dlog[Debug::WARN] << "(0x06): buf: " << rbuf << endl;
				dlog[Debug::WARN] << "(0x06)(" 
					<< rbuf.func << "):(fnWriteOutputSingleRegisters) "
					<< "Получили данных меньше чем ждали...(" 
					<< rlen1 << " < " << szDataLen << ")" << endl;

				return erInvalidFormat;
			}
			
			bcnt += rlen1;
			rbuf.len = bcnt - szModbusHeader;

			WriteSingleOutputMessage mWrite(rbuf);

			if( dlog.debugging(Debug::INFO) )
				dlog[Debug::INFO] << "(0x06): recv buf: " << rbuf << endl;

			if( !crcNoCheckit )
			{
				// Проверяем контрольную сумму
				// от начала(включая заголовок) 
				// и до конца (исключив последний элемент содержащий CRC)
				// int mlen = szModbusHeader + mWrite.szHead() + mWrite.bcnt;
				ModbusData tcrc = checkCRC((ModbusByte*)(&rbuf),bcnt-szCRC);
				if( tcrc != mWrite.crc )
				{
					ostringstream err;
					err << "(0x06): bad crc. calc.crc=" << dat2str(tcrc)
						<< " msg.crc=" << dat2str(mWrite.crc);
					dlog[Debug::WARN] << err.str() << endl;
					return erBadCheckSum;
				}
			}
#warning Стоит ли здесь проверять
			if( !mWrite.checkFormat() )
			{
				dlog[Debug::WARN] << "(0x06): (" << rbuf.func 
					<< ")(fnWriteOutputSingleRegisters): "
					<< ": некорректный формат сообщения..." << endl; 
				return erInvalidFormat;
			}

			return erNoError;			
		}
		else if( rbuf.func == fnJournalCommand )
		{
			JournalCommandMessage mRead(rbuf);
			if( dlog.debugging(Debug::INFO) )
				dlog[Debug::INFO] << "(0x65): recv buf: " << rbuf << endl;

			if( crcNoCheckit )
				return erNoError;

			// Проверяем контрольную сумму
			// от начала(включая заголовок) и до конца (исключив последний элемент содержащий CRC)
			// ModbusData tcrc = checkCRC((ModbusByte*)(&rbuf),sizeof(ReadOutputMessage)-szCRC);
			ModbusData tcrc = checkCRC((ModbusByte*)(&rbuf),bcnt-szCRC);
			if( tcrc != mRead.crc )
			{
				ostringstream err;
				err << "(0x65): bad crc. calc.crc=" << dat2str(tcrc)
					<< " msg.crc=" << dat2str(mRead.crc);
				dlog[Debug::WARN] << err.str() << endl;
				return erBadCheckSum;
			}

			return erNoError;			
		}
		else if( rbuf.func == fnSetDateTime )
		{
			SetDateTimeMessage mSet(rbuf);
			if( dlog.debugging(Debug::INFO) )
				dlog[Debug::INFO] << "(0x50): recv buf: " << rbuf << endl;

			if( !crcNoCheckit )
			{
				// Проверяем контрольную сумму
				// от начала(включая заголовок) и до конца (исключив последний элемент содержащий CRC)
				// ModbusData tcrc = checkCRC((ModbusByte*)(&rbuf),sizeof(ReadOutputMessage)-szCRC);
				ModbusData tcrc = checkCRC((ModbusByte*)(&rbuf),bcnt-szCRC);
				if( tcrc != mSet.crc )
				{
					ostringstream err;
					err << "(0x50): bad crc. calc.crc=" << dat2str(tcrc)
						<< " msg.crc=" << dat2str(mSet.crc);
					dlog[Debug::WARN] << err.str() << endl;
					return erBadCheckSum;
				}
			}
			
			if( !mSet.checkFormat() )
			{
				dlog[Debug::WARN] << "(0x50): некорректные значения..." << endl;
				return erBadDataValue; // return erInvalidFormat;
			}

			return erNoError;			
		}
		else if( rbuf.func == fnRemoteService )
		{
			int szDataLen = RemoteServiceMessage::getDataLen(rbuf)+szCRC;

			if( crcNoCheckit )
				szDataLen -= szCRC;

			// Мы получили только предварительный загловок
			// Теперь необходимо дополучить данные 
			// (c позиции rlen, т.к. часть уже получили)
			int rlen1 = getNextData((unsigned char*)(&(rbuf.data[rlen])),szDataLen);
			if( rlen1 < szDataLen )
			{
				rbuf.len = bcnt + rlen1 - szModbusHeader;
				dlog[Debug::WARN] << "(0x53): buf: " << rbuf << endl;
				dlog[Debug::WARN] << "(0x53)(" 
					<< rbuf.func << "):(fnWriteOutputRegisters) "
					<< "Получили данных меньше чем ждали...(" 
					<< rlen1 << " < " << szDataLen << ")" << endl;

				return erInvalidFormat;
			}
			
			bcnt += rlen1;
			rbuf.len = bcnt - szModbusHeader;

			RemoteServiceMessage mRServ(rbuf);

			if( dlog.debugging(Debug::INFO) )
				dlog[Debug::INFO] << "(0x53): recv buf: " << rbuf << endl;

			if( crcNoCheckit )
				return erNoError;

			// Проверяем контрольную сумму
			// от начала(включая заголовок) 
			// и до конца (исключив последний элемент содержащий CRC)
			// int mlen = szModbusHeader + mWrite.szHead()+ mWrite.bcnt;
			ModbusData tcrc = checkCRC((ModbusByte*)(&rbuf),bcnt-szCRC);
			if( tcrc != mRServ.crc )
			{
				ostringstream err;
				err << "(0x53): bad crc. calc.crc=" << dat2str(tcrc)
					<< " msg.crc=" << dat2str(mRServ.crc);
				dlog[Debug::WARN] << err.str() << endl;
				return erBadCheckSum;
			}

			return erNoError;			
		}
		else if( rbuf.func == fnFileTransfer )
		{
			FileTransferMessage mFT(rbuf);
			if( dlog.debugging(Debug::INFO) )
				dlog[Debug::INFO] << "(0x66): recv buf: " << rbuf << endl;

			if( crcNoCheckit )
				return erNoError;

			// Проверяем контрольную сумму
			// от начала(включая заголовок) 
			// и до конца (исключив последний элемент содержащий CRC)
			// int mlen = szModbusHeader + mWrite.szHead()+ mWrite.bcnt;
			ModbusData tcrc = checkCRC((ModbusByte*)(&rbuf),bcnt-szCRC);
			if( tcrc != mFT.crc )
			{
				ostringstream err;
				err << "(0x66): bad crc. calc.crc=" << dat2str(tcrc)
					<< " msg.crc=" << dat2str(mFT.crc);
				dlog[Debug::WARN] << err.str() << endl;
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
	catch( UniSetTypes::TimeOut )
	{
//		cout << "(recv): catch TimeOut " << endl;
	}
	catch( Exception& ex ) // SystemError
	{
		dlog[Debug::CRIT] << "(recv): " << ex << endl;
		return erHardwareError;
	}

	return erTimeOut;
}
// -------------------------------------------------------------------------
void ModbusServer::setLog( DebugStream& l )
{
	this->dlog = l;
}
// -------------------------------------------------------------------------

void ModbusServer::initLog( UniSetTypes::Configuration* conf,
							const std::string lname, const string logfile )
{						
	conf->initDebug(dlog,lname);

	if( !logfile.empty() )
		dlog.logFile( logfile );
}
// -------------------------------------------------------------------------
void ModbusServer::printProcessingTime()
{
	if( dlog.debugging(Debug::INFO) )
		dlog[Debug::INFO] << "(processingTime): " << tmProcessing.getCurrent() << " [msec]" << endl;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode ModbusServer::replyFileTransfer( const std::string fname, 
														ModbusRTU::FileTransferMessage& query, 
														ModbusRTU::FileTransferRetMessage& reply,
														DebugStream* dlog )
{
	if( dlog && dlog->debugging(Debug::INFO) )
		(*dlog)[Debug::INFO] << "(replyFileTransfer): " << query << endl;
	
	int fd = open(fname.c_str(), O_RDONLY | O_NONBLOCK );
	if( fd <= 0 )
	{
		if( dlog && dlog->debugging(Debug::WARN) )
			(*dlog)[Debug::WARN] << "(replyFileTransfer): open '" << fname << "' with error: " << strerror(errno) << endl;
		return ModbusRTU::erOperationFailed;
	}

	int seek = query.numpacket*ModbusRTU::FileTransferRetMessage::MaxDataLen;
	(void)lseek(fd,seek,SEEK_SET);

	ModbusRTU::ModbusByte buf[ModbusRTU::FileTransferRetMessage::MaxDataLen];

	int ret = ::read(fd,&buf,sizeof(buf));
	if( ret < 0 )
	{
		if( dlog && dlog->debugging(Debug::WARN) )
			(*dlog)[Debug::WARN] << "(replyFileTransfer): read from '" << fname << "' with error: " << strerror(errno) << endl;
		close(fd);
		return ModbusRTU::erOperationFailed;
	}

	struct stat fs;
	if( fstat(fd,&fs) < 0 )
	{
		if( dlog && dlog->debugging(Debug::WARN) )
			(*dlog)[Debug::WARN] << "(replyFileTransfer): fstat for '" << fname << "' with error: " << strerror(errno) << endl;
		close(fd);
		return ModbusRTU::erOperationFailed;
	}
	close(fd);	

	int numpacks = fs.st_size / ModbusRTU::FileTransferRetMessage::MaxDataLen;
	if( fs.st_size % ModbusRTU::FileTransferRetMessage::MaxDataLen )
		numpacks++;
	
	if( !reply.set(query.numfile,numpacks,query.numpacket,buf,ret) )
	{
		if( dlog && dlog->debugging(Debug::WARN) )
			(*dlog)[Debug::WARN] << "(replyFileTransfer): set date failed..." << endl;
		return ModbusRTU::erOperationFailed;
	}

	return ModbusRTU::erNoError;	
}															
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode ModbusServer::replySetDateTime( ModbusRTU::SetDateTimeMessage& query, 
												ModbusRTU::SetDateTimeRetMessage& reply,
												DebugStream* dlog )
{
	if( dlog && dlog->debugging(Debug::INFO) )
		(*dlog)[Debug::INFO] << "(replySetDateTime): " << query << endl;

	struct timeval set;
	struct timezone tz;

	if( gettimeofday(&set,&tz) == 0 )
	{
		struct tm*  t = localtime(&set.tv_sec);
		t->tm_sec 	= query.sec;
		t->tm_min 	= query.min;
		t->tm_hour 	= query.hour;
		t->tm_mday 	= query.day;
		t->tm_mon 	= query.mon-1;
//		t->tm_year 	= (query.century>19) ? query.year + query.century*10 - 1900 : query.year;
		t->tm_year 	= (query.century>19) ? query.year + 2000 - 1900 : query.year;
		set.tv_sec = mktime(t);
		set.tv_usec = 0;

		if( settimeofday(&set,&tz)==0 )
		{
			// подтверждаем сохранение
			// в ответе возвращаем установленное время...
			ModbusRTU::SetDateTimeRetMessage::cpy(reply,query);
			return ModbusRTU::erNoError;
		}
		else if( dlog && dlog->debugging(Debug::WARN) )
			(*dlog)[Debug::WARN] << "(replySetDateTime): settimeofday err: " << strerror(errno) << endl;
	}
	else if( dlog && dlog->debugging(Debug::WARN) )
		(*dlog)[Debug::WARN] << "(replySetDateTime): gettimeofday err: " << strerror(errno) << endl;
	
	return ModbusRTU::erOperationFailed;
}				
// -------------------------------------------------------------------------
mbErrCode ModbusServer::send( ModbusMessage& msg )
{
	mbErrCode ret = pre_send_request(msg);

	if( ret!=erNoError )
		return ret;

	if( msg.len > MAXLENPACKET + szModbusHeader )
	{
		dlog[Debug::WARN] << "(send): длина пакета больше разрешённой..." << endl;
		return erPacketTooLong;
	}

	if( tmProcessing.checkTime() )
	{
		dlog[Debug::WARN] << "(send): reply timeout(" << replyTimeout_ms << ")...!!!" << endl;
		return erTimeOut;
	}

	int len = szModbusHeader+msg.len;
	if( crcNoCheckit )
	{
	    msg.len -= szCRC;
		len -= szCRC;
	}

//	printf("send to %02x type=%d size=%d\n",m.dest,(m.type&TypeMask),slen);
	if( dlog.debugging(Debug::INFO) )
		dlog[Debug::INFO] << "(send): data(" << len << " bytes): " << msg << endl;

	try
	{
		sendData((unsigned char*)(&msg),len);
	}
	catch( Exception& ex ) // SystemError
	{
		dlog[Debug::CRIT] << "(send): " << ex << endl;
		return erHardwareError;
	}

	// Пауза, чтобы не ловить свою посылку
	if( aftersend_msec >= 0 )
		msleep(aftersend_msec);

	return post_send_request(msg);
}

// -------------------------------------------------------------------------
