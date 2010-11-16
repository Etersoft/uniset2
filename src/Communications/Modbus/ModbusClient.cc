#include <string.h>
#include <errno.h>
#include <iostream>
#include <sstream>
#include "Exceptions.h"
#include "modbus/ModbusClient.h"
// -------------------------------------------------------------------------
using namespace std;
using namespace ModbusRTU;
using namespace UniSetTypes;

// -------------------------------------------------------------------------
ModbusClient::ModbusClient():
	replyTimeOut_ms(2000),
	aftersend_msec(0),
	crcNoCheckit(false)
{
	tmProcessing.setTiming(replyTimeOut_ms);
}

// -------------------------------------------------------------------------
ModbusClient::~ModbusClient()
{
}
// -------------------------------------------------------------------------
void ModbusClient::setTimeout( timeout_t msec )
{
	if( msec != UniSetTimer::WaitUpTime )
		replyTimeOut_ms = msec;
}
// -------------------------------------------------------------------------
int ModbusClient::setAfterSendPause( timeout_t msec )
{
	timeout_t old = aftersend_msec;
	aftersend_msec = msec;
	return old;
}
// --------------------------------------------------------------------------------
ReadCoilRetMessage ModbusClient::read01( ModbusAddr addr,
										ModbusData start, ModbusData count )
										throw(ModbusRTU::mbException)
{
	ReadCoilMessage msg(addr,start,count);
	qbuf = msg.transport_msg();
	mbErrCode res = query(addr,qbuf,reply,replyTimeOut_ms);

	if( res==erNoError )
		return ReadCoilRetMessage(reply);

	throw mbException(res);
}
// --------------------------------------------------------------------------------
ReadInputStatusRetMessage ModbusClient::read02( ModbusAddr addr,
										ModbusData start, ModbusData count )
										throw(ModbusRTU::mbException)
{
	ReadInputStatusMessage msg(addr,start,count);
	qbuf = msg.transport_msg();
	mbErrCode res = query(addr,qbuf,reply,replyTimeOut_ms);

	if( res==erNoError )
		return ReadInputStatusRetMessage(reply);

	throw mbException(res);
}

// --------------------------------------------------------------------------------
ReadOutputRetMessage ModbusClient::read03( ModbusAddr addr, 
											ModbusData start, ModbusData count )
												throw(ModbusRTU::mbException)
{
	ReadOutputMessage msg(addr,start,count);
	qbuf = msg.transport_msg();
	mbErrCode res = query(addr,qbuf,reply,replyTimeOut_ms);

	if( res==erNoError )
		return ReadOutputRetMessage(reply);

	throw mbException(res);
}
// --------------------------------------------------------------------------------
ReadInputRetMessage ModbusClient::read04( ModbusAddr addr, 
											ModbusData start, ModbusData count )
												throw(ModbusRTU::mbException)
{
	ReadInputMessage msg(addr,start,count);
	qbuf = msg.transport_msg();
	mbErrCode res = query(addr,qbuf,reply,replyTimeOut_ms);

	if( res==erNoError )
		return ReadInputRetMessage(reply);

	throw mbException(res);
}
// --------------------------------------------------------------------------------
ForceSingleCoilRetMessage ModbusClient::write05( ModbusAddr addr, 
														ModbusData start, bool cmd )
															throw(ModbusRTU::mbException)
{
	ForceSingleCoilMessage msg(addr,start,cmd);
	qbuf = msg.transport_msg();

	mbErrCode res = query(addr,qbuf,reply,replyTimeOut_ms);
	if( res==erNoError )
		return ForceSingleCoilRetMessage(reply);

	throw mbException(res);
}
// --------------------------------------------------------------------------------

WriteSingleOutputRetMessage ModbusClient::write06( ModbusAddr addr, 
														ModbusData start, ModbusData data )
															throw(ModbusRTU::mbException)
{
	WriteSingleOutputMessage msg(addr,start,data);
	qbuf = msg.transport_msg();

	mbErrCode res = query(addr,qbuf,reply,replyTimeOut_ms);
	if( res==erNoError )
		return WriteSingleOutputRetMessage(reply);

	throw mbException(res);
}
// --------------------------------------------------------------------------------
ForceCoilsRetMessage ModbusClient::write0F( ForceCoilsMessage& msg )
												throw(ModbusRTU::mbException)
{
	qbuf = msg.transport_msg();
	mbErrCode res = query(msg.addr,qbuf,reply,replyTimeOut_ms);

	if( res==erNoError )
		return ForceCoilsRetMessage(reply);

	throw mbException(res);
}
// --------------------------------------------------------------------------------

WriteOutputRetMessage ModbusClient::write10( WriteOutputMessage& msg )
												throw(ModbusRTU::mbException)
{
	qbuf = msg.transport_msg();
	mbErrCode res = query(msg.addr,qbuf,reply,replyTimeOut_ms);

	if( res==erNoError )
		return WriteOutputRetMessage(reply);

	throw mbException(res);
}
// --------------------------------------------------------------------------------
SetDateTimeRetMessage ModbusClient::setDateTime( ModbusAddr addr, ModbusByte hour, ModbusByte min, ModbusByte sec,
								ModbusByte day, ModbusByte mon, ModbusByte year,
								ModbusByte century )
										throw(ModbusRTU::mbException)
{
	SetDateTimeMessage msg(addr);
	msg.hour 	= hour;
	msg.min 	= min;
	msg.sec 	= sec;
	msg.day 	= day;
	msg.mon 	= mon;
	msg.year 	= year;
	msg.century = century;
	qbuf = msg.transport_msg();

	mbErrCode res = query(addr,qbuf,reply,replyTimeOut_ms);
	if( res==erNoError )
		return SetDateTimeRetMessage(reply);

	throw mbException(res);
}						
// --------------------------------------------------------------------------------
void ModbusClient::fileTransfer( ModbusAddr addr, ModbusData numfile, 
									const char* save2filename, timeout_t part_timeout_msec )
												throw(ModbusRTU::mbException)
{
//#warning Необходимо реализовать
//	throw mbException(erUnExpectedPacketType);
	mbErrCode res = erNoError;

	FILE* fdsave = fopen(save2filename,"w");
	if( fdsave <= 0 )
	{
		if( dlog.debugging(Debug::WARN) )
			dlog[Debug::WARN] << "(fileTransfer): fopen '" 
						<< save2filename << "' with error: " 
						<< strerror(errno) << endl;
		throw mbException(erHardwareError);
	}
		
	unsigned short maxpackets	= 65535;
	unsigned short curpack		= 0;

	PassiveTimer ptTimeout(part_timeout_msec);

	while( curpack < maxpackets && !ptTimeout.checkTime() )
	{
		try
		{	
			FileTransferRetMessage ret = partOfFileTransfer( addr, numfile, curpack, part_timeout_msec );
			if( ret.numfile != numfile )
			{
				if( dlog.debugging(Debug::WARN) )
					dlog[Debug::WARN] << "(fileTransfer): recv nfile=" << ret.numfile 
										<< " !=numfile(" << numfile << ")" << endl;
				continue;
			}

			if( ret.packet != curpack )
			{
				if( dlog.debugging(Debug::WARN) )
					dlog[Debug::WARN] << "(fileTransfer): recv npack=" << ret.packet 
										<< " !=curpack(" << curpack << ")" << endl;
				continue;
			}
				
			maxpackets = ret.numpacks;

			if( dlog.debugging(Debug::INFO) )
				dlog[Debug::INFO] << "(fileTransfer): maxpackets=" 
								<< ret.numpacks << " curpack=" << curpack+1 << endl;
					
			// save data...
			if( fwrite(&ret.data,ret.dlen,1,fdsave) <= 0 )
			{
				dlog[Debug::WARN] << "(fileTransfer): fwrite '" 
						<< save2filename << "' with error: " 
						<< strerror(errno) << endl;
	
				res = erHardwareError;
				break;
			}

			ptTimeout.reset();
			curpack = ret.packet+1; // curpack++;
		}
		catch( mbException& ex )
		{
			if( ex.err == erBadCheckSum )
				continue;

			res = ex.err;
			break;			
		}
	}
	
	fclose(fdsave);
	
	if( curpack == maxpackets )
		return;

	if( res==erNoError )
		res = erTimeOut;

	throw mbException(res);
}						
// --------------------------------------------------------------------------------
FileTransferRetMessage ModbusClient::partOfFileTransfer( ModbusAddr addr,
										ModbusData idFile, ModbusData numpack,
										timeout_t part_timeout_msec )
											throw(ModbusRTU::mbException)
{
	FileTransferMessage msg(addr,idFile,numpack);
	qbuf = msg.transport_msg();

	mbErrCode res = query(addr,qbuf,reply,part_timeout_msec);
	if( res==erNoError )
		return FileTransferRetMessage(reply);

	throw mbException(res);
}									
// --------------------------------------------------------------------------------
mbErrCode ModbusClient::recv( ModbusAddr addr, ModbusByte qfunc, 
								ModbusMessage& rbuf, timeout_t timeout )
{
	if( timeout == UniSetTimer::WaitUpTime )
		timeout = 15*60*1000*1000; // используем просто большое время (15 минут). Переведя его в наносекунды.

	setChannelTimeout(timeout);
	PassiveTimer tmAbort(timeout);

	// предварительно чистим буфер
	memset(&rbuf,0,sizeof(rbuf));
	int bcnt=0;	// receive bytes count
	
	try
	{
		bool begin = false;
		while( !tmAbort.checkTime() )
		{
			bcnt = getNextData((unsigned char*)(&rbuf),sizeof(ModbusAddr));
			if( bcnt!=0 && ( rbuf.addr==addr ) ) // || (onBroadcast && rbuf.addr==BroadcastAddr) ) )
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

		return recv_pdu(qfunc,rbuf,timeout);
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

// --------------------------------------------------------------------------------
mbErrCode ModbusClient::recv_pdu( ModbusByte qfunc, ModbusMessage& rbuf, timeout_t timeout )
{
	int bcnt=1; // receive bytes count (1 - addr)
	try
	{
		// -----------------------------------
		tmProcessing.setTiming(replyTimeOut_ms);
		// recv func number
		size_t k = getNextData((unsigned char*)(&rbuf.func),sizeof(ModbusByte));
		if( k < sizeof(ModbusByte) )
		{
			dlog[Debug::WARN] << "(recv): " << (ModbusHeader*)(&rbuf) << endl;
			dlog[Debug::WARN] << "(recv): заголовок меньше положенного..." << endl;
			cleanupChannel();
			return erInvalidFormat;
		}

		bcnt += k;
		rbuf.len = 0;
		
		if( dlog.debugging(Debug::INFO) )
			dlog[Debug::INFO] << "(recv): header: " << rbuf << endl;

		// обработка сообщения об ошибке...
		if( rbuf.func == (qfunc|MBErrMask) )
		{
			rbuf.len = ErrorRetMessage::szData();
			if( crcNoCheckit )
				rbuf.len -= szCRC;
			
			int rlen = getNextData((unsigned char*)(&(rbuf.data)),rbuf.len);
			if( rlen < rbuf.len )
			{
				if( dlog.debugging(Debug::WARN) )
				{
					dlog[Debug::WARN] << "(recv:Error): buf: " << rbuf << endl;
					dlog[Debug::WARN] << "(recv:Error)(" << rbuf.func 
							<< "): Получили данных меньше чем ждали...(recv=" 
							<< rlen << " < wait=" << rbuf.len << ")" << endl;
				}
				
				cleanupChannel();
				return erInvalidFormat;
			}
			bcnt+=rlen;
			
			ErrorRetMessage em(rbuf);
			if( !crcNoCheckit )
			{
				ModbusData tcrc = checkCRC((ModbusByte*)(&rbuf),bcnt-szCRC);
				if( tcrc != em.crc )
				{
					ostringstream err;
					err << "(recv:error): bad crc. calc.crc=" << dat2str(tcrc)
						<< " msg.crc=" << dat2str(em.crc);
					dlog[Debug::WARN] << err.str() << endl;
					return erBadCheckSum;
				}
			}

			return (mbErrCode)em.ecode;
		}

		if( qfunc!=rbuf.func )
		{
			cleanupChannel();
			return erUnExpectedPacketType;
		}
	
		// Определяем тип сообщения
		switch( rbuf.func )
		{
			case fnReadCoilStatus:
				rbuf.len = ReadCoilRetMessage::szHead();
			break;

			case fnReadInputStatus:
				rbuf.len = ReadInputStatusRetMessage::szHead();
			break;

			case fnReadOutputRegisters:
				rbuf.len = ReadOutputRetMessage::szHead();
			break;

			case fnReadInputRegisters:
				rbuf.len = ReadInputRetMessage::szHead();
			break;

			case fnForceMultipleCoils:
				rbuf.len = ForceCoilsRetMessage::szData();
				if( crcNoCheckit )
					rbuf.len -= szCRC;
			break;
	
			case fnWriteOutputRegisters:
				rbuf.len = WriteOutputRetMessage::szData();
				if( crcNoCheckit )
					rbuf.len -= szCRC;
			break;

			case fnWriteOutputSingleRegister:
				rbuf.len = WriteSingleOutputRetMessage::szData();
				if( crcNoCheckit )
					rbuf.len -= szCRC;
			break;

			case fnForceSingleCoil:
				rbuf.len = ForceSingleCoilRetMessage::szData();
				if( crcNoCheckit )
					rbuf.len -= szCRC;
			break;

			case fnSetDateTime:
				rbuf.len = SetDateTimeRetMessage::szData();
				if( crcNoCheckit )
					rbuf.len -= szCRC;
			break;

			case fnFileTransfer:
				rbuf.len = FileTransferRetMessage::szHead();
			break;

/*
			case fnJournalCommand:
				rbuf.len = JournalCommandMessage::szData();
			break;

			case fnRemoteService:
				rbuf.len = RemoteServiceMessage::szHead();
			break;
*/			
			default:
				cleanupChannel();
				return erUnExpectedPacketType;
		}


		// ДЛЯ ТОГО ЧТОБЫ НЕ ЖДАТЬ ПРОДОЛЖЕНИЯ БЕЗКОНЕЧНО СБРАСЫВАЕМ TIMEOUT
		setChannelTimeout(10); // 10 msec

		// Получаем остальную часть сообщения
		int rlen = getNextData((unsigned char*)(rbuf.data),rbuf.len);
		if( rlen < rbuf.len )
		{
//			rbuf.len = bcnt + rlen - szModbusHeader;
			if( dlog.debugging(Debug::WARN) )
			{
				dlog[Debug::WARN] << "(recv): buf: " << rbuf << endl;
				dlog[Debug::WARN] << "(recv)(" << rbuf.func 
						<< "): Получили данных меньше чем ждали...(recv=" 
						<< rlen << " < wait=" << rbuf.len << ")" << endl;
			}

			cleanupChannel();
			return erInvalidFormat;
		}
		
		bcnt+=rlen;

		// получаем остальное...
		if( rbuf.func == fnReadCoilStatus )
		{
			int szDataLen = ReadCoilRetMessage::getDataLen(rbuf)+szCRC;

			if( crcNoCheckit )
				szDataLen -= szCRC;
			
			// Мы получили только предварительный загловок
			// Теперь необходимо дополучить данные 
			// (c позиции rlen, т.к. часть уже получили)
			int rlen1 = getNextData((unsigned char*)(&(rbuf.data[rlen])),szDataLen);
			if( rlen1 < szDataLen )
			{
				rbuf.len = bcnt + rlen1 - szModbusHeader;
				if( dlog.debugging(Debug::WARN) )
				{
					dlog[Debug::WARN] << "(0x01): buf: " << rbuf << endl;
					dlog[Debug::WARN] << "(0x01)(" 
						<< (int)rbuf.func << "):(fnReadCoilStatus) "
						<< "Получили данных меньше чем ждали...(" 
						<< rlen1 << " < " << szDataLen << ")" << endl;
				}

				cleanupChannel();
				return erInvalidFormat;
			}
			
			bcnt += rlen1;
			rbuf.len = bcnt - szModbusHeader;

			ReadCoilRetMessage mRead(rbuf);

			if( dlog.debugging(Debug::INFO) )
				dlog[Debug::INFO] << "(0x01)(fnReadCoilStatus): recv buf: " << rbuf << endl;

			if( crcNoCheckit )
				return erNoError;

			// Проверяем контрольную сумму
			// от начала(включая заголовок) 
			// и до конца (исключив последний элемент содержащий CRC)
			// int mlen = szModbusHeader + mWrite.szHead()+ mWrite.bcnt;
			ModbusData tcrc = checkCRC((ModbusByte*)(&rbuf),bcnt-szCRC);
			if( tcrc != mRead.crc )
			{
				ostringstream err;
				err << "(0x01)(fnReadCoilStatus): bad crc. calc.crc=" << dat2str(tcrc)
					<< " msg.crc=" << dat2str(mRead.crc);
				if( dlog.debugging(Debug::WARN) )
					dlog[Debug::WARN] << err.str() << endl;
				return erBadCheckSum;
			}

			return erNoError;
		}
		else if( rbuf.func == fnReadInputStatus )
		{
			int szDataLen = ReadInputStatusRetMessage::getDataLen(rbuf)+szCRC;

			if( crcNoCheckit )
				szDataLen -= szCRC;
			
			// Мы получили только предварительный загловок
			// Теперь необходимо дополучить данные 
			// (c позиции rlen, т.к. часть уже получили)
			int rlen1 = getNextData((unsigned char*)(&(rbuf.data[rlen])),szDataLen);
			if( rlen1 < szDataLen )
			{
				rbuf.len = bcnt + rlen1 - szModbusHeader;
				if( dlog.debugging(Debug::WARN) )
				{
					dlog[Debug::WARN] << "(0x02): buf: " << rbuf << endl;
					dlog[Debug::WARN] << "(0x02)(" 
						<< (int)rbuf.func << "):(fnReadInputStatus) "
						<< "Получили данных меньше чем ждали...(" 
						<< rlen1 << " < " << szDataLen << ")" << endl;
				}
					
				cleanupChannel();
				return erInvalidFormat;
			}
			
			bcnt += rlen1;
			rbuf.len = bcnt - szModbusHeader;

			ReadInputStatusRetMessage mRead(rbuf);

			if( dlog.debugging(Debug::INFO) )
				dlog[Debug::INFO] << "(0x02)(fnReadInputStatus): recv buf: " << rbuf << endl;
			
			if( crcNoCheckit )
				return erNoError;

			// Проверяем контрольную сумму
			// от начала(включая заголовок) 
			// и до конца (исключив последний элемент содержащий CRC)
			// int mlen = szModbusHeader + mWrite.szHead()+ mWrite.bcnt;
			ModbusData tcrc = checkCRC((ModbusByte*)(&rbuf),bcnt-szCRC);
			if( tcrc != mRead.crc )
			{
				ostringstream err;
				err << "(recv:fnReadInputStatus): bad crc. calc.crc=" << dat2str(tcrc)
					<< " msg.crc=" << dat2str(mRead.crc);
				if( dlog.debugging(Debug::WARN) )
					dlog[Debug::WARN] << err.str() << endl;
				return erBadCheckSum;
			}

			return erNoError;			
		}
		else if( rbuf.func == fnReadInputRegisters )
		{
			int szDataLen = ReadInputRetMessage::getDataLen(rbuf)+szCRC;

			if( crcNoCheckit )
				szDataLen -= szCRC;
			
			// Мы получили только предварительный загловок
			// Теперь необходимо дополучить данные 
			// (c позиции rlen, т.к. часть уже получили)
			int rlen1 = getNextData((unsigned char*)(&(rbuf.data[rlen])),szDataLen);
			if( rlen1 < szDataLen )
			{
				rbuf.len = bcnt + rlen1 - szModbusHeader;
				if( dlog.debugging(Debug::WARN) )
				{
					dlog[Debug::WARN] << "(0x04): buf: " << rbuf << endl;
					dlog[Debug::WARN] << "(0x04)(" 
						<< (int)rbuf.func << "):(fnReadInputRegisters) "
						<< "Получили данных меньше чем ждали...(" 
						<< rlen1 << " < " << szDataLen << ")" << endl;
				}

				cleanupChannel();
				return erInvalidFormat;
			}
			
			bcnt += rlen1;
			rbuf.len = bcnt - szModbusHeader;

			ReadInputRetMessage mRead(rbuf);

			if( dlog.debugging(Debug::INFO) )
				dlog[Debug::INFO] << "(recv)(fnReadInputRegisters): recv buf: " << rbuf << endl;

			if( crcNoCheckit )
				return erNoError;

			// Проверяем контрольную сумму
			// от начала(включая заголовок) 
			// и до конца (исключив последний элемент содержащий CRC)
			// int mlen = szModbusHeader + mWrite.szHead()+ mWrite.bcnt;
			ModbusData tcrc = checkCRC((ModbusByte*)(&rbuf),bcnt-szCRC);
			if( tcrc != mRead.crc )
			{
				ostringstream err;
				err << "(recv:fnReadInputRegisters): bad crc. calc.crc=" << dat2str(tcrc)
					<< " msg.crc=" << dat2str(mRead.crc);
				if( dlog.debugging(Debug::WARN) )
					dlog[Debug::WARN] << err.str() << endl;
				return erBadCheckSum;
			}

			return erNoError;
		}
		else if( rbuf.func == fnReadOutputRegisters )
		{
			int szDataLen = ReadOutputRetMessage::getDataLen(rbuf)+szCRC;
			if( crcNoCheckit )
				szDataLen -= szCRC;
			
			// Мы получили только предварительный загловок
			// Теперь необходимо дополучить данные 
			// (c позиции rlen, т.к. часть уже получили)
			int rlen1 = getNextData((unsigned char*)(&(rbuf.data[rlen])),szDataLen);
			if( rlen1 < szDataLen )
			{
				rbuf.len = bcnt + rlen1 - szModbusHeader;
				if( dlog.debugging(Debug::WARN) )
				{
					dlog[Debug::WARN] << "(0x03): buf: " << rbuf << endl;
					dlog[Debug::WARN] << "(0x03)(" 
						<< (int)rbuf.func << "):(fnReadInputRegisters) "
						<< "Получили данных меньше чем ждали...(" 
						<< rlen1 << " < " << szDataLen << ")" << endl;
				}

				cleanupChannel();
				return erInvalidFormat;
			}
			
			bcnt += rlen1;
			rbuf.len = bcnt - szModbusHeader;

			ReadOutputRetMessage mRead(rbuf);

			if( dlog.debugging(Debug::INFO) )
				dlog[Debug::INFO] << "(recv)(fnReadOutputRegisters): recv buf: " << rbuf << endl;

			if( crcNoCheckit )
				return erNoError;

			// Проверяем контрольную сумму
			// от начала(включая заголовок) 
			// и до конца (исключив последний элемент содержащий CRC)
			// int mlen = szModbusHeader + mWrite.szHead()+ mWrite.bcnt;
			ModbusData tcrc = checkCRC((ModbusByte*)(&rbuf),bcnt-szCRC);
			if( tcrc != mRead.crc )
			{
				ostringstream err;
				err << "(recv:fnReadOutputRegisters): bad crc. calc.crc=" << dat2str(tcrc)
					<< " msg.crc=" << dat2str(mRead.crc);
				if( dlog.debugging(Debug::WARN) )
					dlog[Debug::WARN] << err.str() << endl;
				return erBadCheckSum;
			}

			return erNoError;
		}
		else if( rbuf.func == fnForceMultipleCoils )
		{
			rbuf.len = bcnt - szModbusHeader;
			ForceCoilsRetMessage mWrite(rbuf);

			if( dlog.debugging(Debug::INFO) )
				dlog[Debug::INFO] << "(0x0F): recv buf: " << rbuf << endl;

			if( crcNoCheckit )
				return erNoError;

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
			return erNoError;
		}
		else if( rbuf.func == fnWriteOutputRegisters )
		{
			rbuf.len = bcnt - szModbusHeader;
			WriteOutputRetMessage mWrite(rbuf);

			if( dlog.debugging(Debug::INFO) )
				dlog[Debug::INFO] << "(0x10): recv buf: " << rbuf << endl;

			if( crcNoCheckit )
				return erNoError;

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

			return erNoError;
		}
		else if( rbuf.func == fnWriteOutputSingleRegister )
		{
			WriteSingleOutputRetMessage mWrite(rbuf);

			if( dlog.debugging(Debug::INFO) )
				dlog[Debug::INFO] << "(0x06): recv buf: " << rbuf << endl;

			if( crcNoCheckit )
				return erNoError;

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

			return erNoError;
		}
		else if( rbuf.func == fnForceSingleCoil )
		{
			ForceSingleCoilRetMessage mWrite(rbuf);

			if( dlog.debugging(Debug::INFO) )
				dlog[Debug::INFO] << "(0x05): recv buf: " << rbuf << endl;

			if( crcNoCheckit )
				return erNoError;

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
			return erNoError;
		}
		else if( rbuf.func == fnSetDateTime )
		{
			SetDateTimeRetMessage mSet(rbuf);
			if( dlog.debugging(Debug::INFO) )
				dlog[Debug::INFO] << "(0x50): recv buf: " << rbuf << endl;

			if( !crcNoCheckit )
			{
				// Проверяем контрольную сумму
				// от начала(включая заголовок) и до конца (исключив последний элемент содержащий CRC)
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
		else if( rbuf.func == fnFileTransfer )
		{
			int szDataLen = FileTransferRetMessage::getDataLen(rbuf)+szCRC;
			if( crcNoCheckit )
				szDataLen -= szCRC;

			// Мы получили только предварительный загловок
			// Теперь необходимо дополучить данные 
			// (c позиции rlen, т.к. часть уже получили)
			int rlen1 = getNextData((unsigned char*)(&(rbuf.data[rlen])),szDataLen);
			if( rlen1 < szDataLen )
			{
				rbuf.len = bcnt + rlen1 - szModbusHeader;
				dlog[Debug::WARN] << "(0x66): buf: " << rbuf << endl;
				dlog[Debug::WARN] << "(0x66)(" 
					<< rbuf.func << "):(fnFileTransfer) "
					<< "Получили данных меньше чем ждали...(" 
					<< rlen1 << " < " << szDataLen << ")" << endl;
				
				cleanupChannel();
				return erInvalidFormat;
			}
			
			bcnt += rlen1;
			rbuf.len = bcnt - szModbusHeader;

			FileTransferRetMessage mFT(rbuf);
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
#if 0
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

				cleanupChannel();
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
#endif // not standart commands

		else
		{
			// А как мы сюда добрались?!!!!!!
			cleanupChannel();
			return erUnExpectedPacketType;
		}		
	}
	catch( mbException& ex )
	{
		dlog[Debug::CRIT] << "(recv): " << ex << endl;
		return ex.err;
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
mbErrCode ModbusClient::send( ModbusMessage& msg )
{
	if( msg.len > MAXLENPACKET + szModbusHeader )
	{
		dlog[Debug::WARN] << "(send): длина пакета больше разрешённой..." << endl;
		return erPacketTooLong;
	}

	int len = szModbusHeader+msg.len;
	if( crcNoCheckit )
		len -= szCRC;

	if( dlog.debugging(Debug::INFO) )
		dlog[Debug::INFO] << "(send)(" << len << " bytes): " << msg << endl;

	try
	{
		sendData((unsigned char*)(&msg),len);
	}
	catch( mbException& ex )
	{
		dlog[Debug::CRIT] << "(send): " << ex << endl;
		return ex.err;
	}
	catch( Exception& ex ) // SystemError
	{
		dlog[Debug::CRIT] << "(send): " << ex << endl;
		return erHardwareError;
	}

	// Пауза, чтобы не ловить свою посылку
	if( aftersend_msec >= 0 )
		msleep(aftersend_msec);
//#warning Разобраться с паузой после посылки...
//	msleep(10);

	return erNoError;
}

// -------------------------------------------------------------------------
void ModbusClient::initLog( UniSetTypes::Configuration* conf,
							const std::string lname, const string logfile )
{						
	conf->initDebug(dlog,lname);

	if( !logfile.empty() )
		dlog.logFile( logfile );
}
// -------------------------------------------------------------------------
void ModbusClient::setLog( DebugStream& l )
{
	this->dlog = l;
}
// -------------------------------------------------------------------------
void ModbusClient::printProcessingTime()
{
	if( dlog.debugging(Debug::INFO) )
	{
		dlog[Debug::INFO] << "(processingTime): " 
			<< tmProcessing.getCurrent() << " [мсек]" << endl;
	}
}
// -------------------------------------------------------------------------
