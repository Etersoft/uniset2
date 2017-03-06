// -------------------------------------------------------------------------
//#include <sys/time.h>
//#include <string.h>
//#include <errno.h>
#include <sstream>
#include "UniSetTypes.h"
#include "MBSlave.h"
#include "uniset-config.h"
// -------------------------------------------------------------------------
#ifndef PACKAGE_URL
#define PACKAGE_URL "http://git.etersoft.ru/projects/?p=asu/uniset.git;a=summary"
#endif
// -------------------------------------------------------------------------
using namespace std;
using namespace uniset;
using namespace ModbusRTU;
// -------------------------------------------------------------------------
MBSlave::MBSlave(const std::unordered_set<ModbusAddr>& _vaddr, const std::string& dev, const std::string& speed, bool use485 ):
	rscomm(NULL),
	vaddr(_vaddr),
	//    prev(ModbusRTU::erNoError),
	//    askCount(0),
	verbose(false),
	replyVal(-1),
	replyVal2(-1),
	replyVal3(-1)
{
	//    int replyTimeout = uni_atoi( conf->getArgParam("--reply-timeout",it.getProp("reply_timeout")).c_str() );
	//    if( replyTimeout <= 0 )
	//        replyTimeout = 2000;

	if( verbose )
		cout << "(init): "
			 << " addr=" << ModbusServer::vaddr2str(vaddr)
			 << " dev=" << dev
			 << " speed=" << speed;

	rscomm     = new ModbusRTUSlaveSlot(dev, use485);

	rscomm->setSpeed(speed);
	//    rscomm->initLog(conf,name,logfile);

	rscomm->connectReadCoil( sigc::mem_fun(this, &MBSlave::readCoilStatus) );
	rscomm->connectReadInputStatus( sigc::mem_fun(this, &MBSlave::readInputStatus) );
	rscomm->connectReadOutput( sigc::mem_fun(this, &MBSlave::readOutputRegisters) );
	rscomm->connectReadInput( sigc::mem_fun(this, &MBSlave::readInputRegisters) );
	rscomm->connectForceSingleCoil( sigc::mem_fun(this, &MBSlave::forceSingleCoil) );
	rscomm->connectForceCoils( sigc::mem_fun(this, &MBSlave::forceMultipleCoils) );
	rscomm->connectWriteOutput( sigc::mem_fun(this, &MBSlave::writeOutputRegisters) );
	rscomm->connectWriteSingleOutput( sigc::mem_fun(this, &MBSlave::writeOutputSingleRegister) );
	rscomm->connectJournalCommand( sigc::mem_fun(this, &MBSlave::journalCommand) );
	rscomm->connectSetDateTime( sigc::mem_fun(this, &MBSlave::setDateTime) );
	rscomm->connectRemoteService( sigc::mem_fun(this, &MBSlave::remoteService) );
	rscomm->connectFileTransfer( sigc::mem_fun(this, &MBSlave::fileTransfer) );
	rscomm->connectDiagnostics( sigc::mem_fun(this, &MBSlave::diagnostics) );
	rscomm->connectMEIRDI( sigc::mem_fun(this, &MBSlave::read4314) );


	rscomm->setRecvTimeout(2000);
	//    rscomm->setAfterSendPause(afterSend);
	//    rscomm->setReplyTimeout(replyTimeout);

	// build file list...
}

// -------------------------------------------------------------------------
MBSlave::~MBSlave()
{
	delete rscomm;
}
// -------------------------------------------------------------------------
void MBSlave::setLog( std::shared_ptr<DebugStream> dlog )
{
	if( rscomm )
		rscomm->setLog(dlog);
}
// -------------------------------------------------------------------------
void MBSlave::execute()
{
	// Работа...
	while(1)
	{
		ModbusRTU::mbErrCode res = rscomm->receive( vaddr, UniSetTimer::WaitUpTime );
#if 0

		// собираем статистику обмена
		if( prev != ModbusRTU::erTimeOut )
		{
			//  с проверкой на переполнение
			askCount = askCount >= numeric_limits<long>::max() ? 0 : askCount + 1;

			if( res != ModbusRTU::erNoError )
				++errmap[res];

			prev = res;
		}

#endif

		if( verbose && res != ModbusRTU::erNoError && res != ModbusRTU::erTimeOut )
			cerr << "(wait): " << ModbusRTU::mbErr2Str(res) << endl;
	}
}
// -------------------------------------------------------------------------
void MBSlave::sigterm( int signo )
{
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::readCoilStatus( ReadCoilMessage& query,
		ReadCoilRetMessage& reply )
{
	if( verbose )
		cout << "(readCoilStatus): " << query << endl;

	ModbusRTU::DataBits d;
	d.b[0] = 1;
	d.b[2] = 1;
	d.b[4] = 1;
	d.b[6] = 1;

	// Фомирование ответа:
	unsigned int bcnt = query.count / ModbusRTU::BitsPerByte;

	if( (query.count % ModbusRTU::BitsPerByte) > 0 )
		bcnt++;

	for( unsigned int i = 0; i < bcnt; i++ )
	{
		if( replyVal != -1 )
			reply.addData(replyVal);
		else
			reply.addData(d);
	}

	return ModbusRTU::erNoError;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::readInputStatus( ReadInputStatusMessage& query,
		ReadInputStatusRetMessage& reply )
{
	if( verbose )
		cout << "(readInputStatus): " << query << endl;

	ModbusRTU::DataBits d;
	d.b[0] = 1;
	d.b[3] = 1;
	d.b[7] = 1;

	if( replyVal == -1 )
	{
		int bnum = 0;
		int i = 0;

		while( i < query.count )
		{
			reply.addData(0);

			for( unsigned int nbit = 0; nbit < BitsPerByte && i < query.count; nbit++, i++ )
				reply.setBit(bnum, nbit, d.b[nbit]);

			bnum++;
		}
	}
	else
	{
		unsigned int bcnt = query.count / ModbusRTU::BitsPerByte;

		if( (query.count % ModbusRTU::BitsPerByte) > 0 )
			bcnt++;

		for( unsigned int i = 0; i < bcnt; i++ )
		{
			if( i == 1 )
				reply.addData(replyVal2);
			else if( i == 2 )
				reply.addData(replyVal3);
			else
				reply.addData(replyVal);
		}
	}

	return ModbusRTU::erNoError;
}
// -------------------------------------------------------------------------
mbErrCode MBSlave::readInputRegisters( ReadInputMessage& query,
									   ReadInputRetMessage& reply )
{
	if( verbose )
		cout << "(readInputRegisters): " << query << endl;

	if( query.count <= 1 )
	{
		if( replyVal != -1 )
			reply.addData(replyVal);
		else
			reply.addData(query.start);

		return ModbusRTU::erNoError;
	}

	// Фомирование ответа:
	int num = 0; // добавленное количество данных
	ModbusData reg = query.start;

	for( ; num < query.count; num++, reg++ )
	{
		if( replyVal != -1 )
		{
			if( num == 1 && replyVal2 != -1  )
				reply.addData(replyVal2);
			else if( num == 2 && replyVal3 != -1 )
				reply.addData(replyVal3);
			else
				reply.addData(replyVal);
		}
		else
			reply.addData(reg);
	}

	// Если мы в начале проверили, что запрос входит в разрешёный диапазон
	// то теоретически этой ситуации возникнуть не может...
	if( reply.count < query.count )
	{
		cerr << "(readInputRegisters): Получили меньше чем ожидали. "
			 << " Запросили " << query.count << " получили " << reply.count << endl;
	}

	return ModbusRTU::erNoError;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::readOutputRegisters(
	ModbusRTU::ReadOutputMessage& query, ModbusRTU::ReadOutputRetMessage& reply )
{
	if( verbose )
		cout << "(readOutputRegisters): " << query << endl;

	if( query.count <= 1 )
	{
		if( replyVal != -1 )
			reply.addData(replyVal);
		else
			reply.addData(query.start);

		return ModbusRTU::erNoError;
	}

	// Фомирование ответа:
	int num = 0; // добавленное количество данных
	ModbusData reg = query.start;

	for( ; num < query.count; num++, reg++ )
	{
		if( replyVal != -1 )
		{
			if( num == 1 && replyVal2 != -1  )
				reply.addData(replyVal2);
			else if( num == 2 && replyVal3 != -1 )
				reply.addData(replyVal3);
			else
				reply.addData(replyVal);
		}
		else
			reply.addData(reg);
	}

	// Если мы в начале проверили, что запрос входит в разрешёный диапазон
	// то теоретически этой ситуации возникнуть не может...
	if( reply.count < query.count )
	{
		cerr << "(readOutputRegisters): Получили меньше чем ожидали. "
			 << " Запросили " << query.count << " получили " << reply.count << endl;
	}

	return ModbusRTU::erNoError;
}

// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::forceMultipleCoils( ModbusRTU::ForceCoilsMessage& query,
		ModbusRTU::ForceCoilsRetMessage& reply )
{
	if( verbose )
		cout << "(forceMultipleCoils): " << query << endl;

	ModbusRTU::mbErrCode ret = ModbusRTU::erNoError;
	reply.set(query.start, query.quant);
	return ret;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::writeOutputRegisters( ModbusRTU::WriteOutputMessage& query,
		ModbusRTU::WriteOutputRetMessage& reply )
{
	if( verbose )
		cout << "(writeOutputRegisters): " << query << endl;

	ModbusRTU::mbErrCode ret = ModbusRTU::erNoError;
	reply.set(query.start, query.quant);
	return ret;
}

// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::writeOutputSingleRegister( ModbusRTU::WriteSingleOutputMessage& query,
		ModbusRTU::WriteSingleOutputRetMessage& reply )
{
	if( verbose )
		cout << "(writeOutputSingleRegisters): " << query << endl;

	ModbusRTU::mbErrCode ret = ModbusRTU::erNoError;
	reply.set(query.start, query.data);
	return ret;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::forceSingleCoil( ModbusRTU::ForceSingleCoilMessage& query,
		ModbusRTU::ForceSingleCoilRetMessage& reply )
{
	if( verbose )
		cout << "(forceSingleCoil): " << query << endl;

	ModbusRTU::mbErrCode ret = ModbusRTU::erNoError;
	reply.set(query.start, query.cmd());
	return ret;
}

// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::journalCommand( ModbusRTU::JournalCommandMessage& query,
		ModbusRTU::JournalCommandRetMessage& reply )
{
	if( verbose )
		cout << "(journalCommand): " << query << endl;

	switch( query.cmd )
	{
		case 0:
		{
			if( reply.setData( (ModbusRTU::ModbusByte*)(&query.num), sizeof(query.num) ) )
				return ModbusRTU::erNoError;

			return ModbusRTU::erPacketTooLong;
		}
		break;

		case 1:
		{
			ModbusRTU::JournalCommandRetOK::set(reply, 1, 0);
			return ModbusRTU::erNoError;
		}
		break;

		case 2: // write по modbus пока не поддерживается
		default:
		{
			// формируем ответ
			ModbusRTU::JournalCommandRetOK::set(reply, 2, 1);
			return ModbusRTU::erNoError;
		}
		break;
	}

	return ModbusRTU::erTimeOut;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::setDateTime( ModbusRTU::SetDateTimeMessage& query,
		ModbusRTU::SetDateTimeRetMessage& reply )
{
	if( verbose )
		cout << "(setDateTime): " << query << endl;

	// подтверждаем сохранение
	// в ответе возвращаем установленное время...
	ModbusRTU::SetDateTimeRetMessage::cpy(reply, query);
	return ModbusRTU::erNoError;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::remoteService( ModbusRTU::RemoteServiceMessage& query,
		ModbusRTU::RemoteServiceRetMessage& reply )
{
	cerr << "(remoteService): " << query << endl;
	return ModbusRTU::erOperationFailed;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::fileTransfer( ModbusRTU::FileTransferMessage& query,
		ModbusRTU::FileTransferRetMessage& reply )
{
	if( verbose )
		cout << "(fileTransfer): " << query << endl;

	return ModbusRTU::erOperationFailed;

#if 0
	FileList::iterator it = flist.find(query.numfile);

	if( it == flist.end() )
		return ModbusRTU::erBadDataValue;

	std::string fname(it->second);

	int fd = open(fname.c_str(), O_RDONLY | O_NONBLOCK );

	if( fd < 0 )
	{
		dwarn << "(fileTransfer): open '" << fname << "' with error: " << strerror(errno) << endl;
		return ModbusRTU::erOperationFailed;
	}

	int seek = query.numpacket * ModbusRTU::FileTransferRetMessage::MaxDataLen;
	int ret = lseek(fd, seek, SEEK_SET);

	if( ret < 0 )
	{
		close(fd);
		dwarn << "(fileTransfer): lseek '" << fname << "' with error: " << strerror(errno) << endl;
		return ModbusRTU::erOperationFailed;
	}

	ModbusRTU::ModbusByte buf[ModbusRTU::FileTransferRetMessage::MaxDataLen];

	ret = ::read(fd, &buf, sizeof(buf));

	if( ret < 0 )
	{
		close(fd);
		dwarn << "(fileTransfer): read from '" << fname << "' with error: " << strerror(errno) << endl;
		return ModbusRTU::erOperationFailed;
	}

	// вычисляем общий размер файла в "пакетах"
	//    (void)lseek(fd,0,SEEK_END);
	//    int numpacks = lseek(fd,0,SEEK_CUR) / ModbusRTU::FileTransferRetMessage::MaxDataLen;
	//    if( lseek(fd,0,SEEK_CUR) % ModbusRTU::FileTransferRetMessage::MaxDataLen )
	//        numpacks++;

	struct stat fs;

	if( fstat(fd, &fs) < 0 )
	{
		close(fd);
		dwarn << "(fileTransfer): fstat for '" << fname << "' with error: " << strerror(errno) << endl;
		return ModbusRTU::erOperationFailed;
	}

	close(fd);

	//    cerr << "******************* ret = " << ret << " fsize = " << fs.st_size
	//        << " maxsize = " << ModbusRTU::FileTransferRetMessage::MaxDataLen << endl;

	int numpacks = fs.st_size / ModbusRTU::FileTransferRetMessage::MaxDataLen;

	if( fs.st_size % ModbusRTU::FileTransferRetMessage::MaxDataLen )
		numpacks++;

	if( !reply.set(query.numfile, numpacks, query.numpacket, buf, ret) )
	{
		dwarn << "(fileTransfer): set date failed..." << endl;
		return ModbusRTU::erOperationFailed;
	}

	return ModbusRTU::erNoError;
#endif

}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::diagnostics( ModbusRTU::DiagnosticMessage& query,
		ModbusRTU::DiagnosticRetMessage& reply )
{
	if( verbose )
		cout << "(diagnostics): " << query << endl;

	if( query.subf == ModbusRTU::subEcho )
	{
		reply = query;
		return ModbusRTU::erNoError;
	}

	if( query.subf == ModbusRTU::dgBusErrCount )
	{
		reply = query;
		reply.data[0] = 10;
		return ModbusRTU::erNoError;
	}

	if( query.subf == ModbusRTU::dgMsgSlaveCount || query.subf == ModbusRTU::dgBusMsgCount )
	{
		reply = query;
		reply.data[0] = 10;
		return ModbusRTU::erNoError;
	}

	if( query.subf == ModbusRTU::dgSlaveNAKCount )
	{
		reply = query;
		reply.data[0] = 10;
		return ModbusRTU::erNoError;
	}

	if( query.subf == ModbusRTU::dgClearCounters )
	{
		reply = query;
		return ModbusRTU::erNoError;
	}

	return ModbusRTU::erOperationFailed;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBSlave::read4314( ModbusRTU::MEIMessageRDI& query,
										ModbusRTU::MEIMessageRetRDI& reply )
{
	if( verbose )
		cout << "(read4314): " << query << endl;

	if( query.devID <= rdevMinNum || query.devID >= rdevMaxNum )
		return erOperationFailed;

	if( query.objID == rdiVendorName )
	{
		reply.mf = 0xFF;
		reply.conformity = rdevBasicDevice;
		reply.addData(query.objID, "etersoft");
		//        reply.addData(rdiProductCode, PACKAGE_NAME);
		//        reply.addData(rdiMajorMinorRevision,PACKAGE_VERSION);
		return erNoError;
	}
	else if( query.objID == rdiProductCode )
	{
		reply.mf = 0xFF;
		reply.conformity = rdevBasicDevice;
		reply.addData(query.objID, PACKAGE_NAME);
		return erNoError;
	}
	else if( query.objID == rdiMajorMinorRevision )
	{
		reply.mf = 0xFF;
		reply.conformity = rdevBasicDevice;
		reply.addData(query.objID, PACKAGE_VERSION);
		return erNoError;
	}
	else if( query.objID == rdiVendorURL )
	{
		reply.mf = 0xFF;
		reply.conformity = rdevRegularDevice;
		reply.addData(query.objID, PACKAGE_URL);
		return erNoError;
	}
	else if( query.objID == rdiProductName )
	{
		reply.mf = 0xFF;
		reply.conformity = rdevRegularDevice;
		reply.addData(query.objID, PACKAGE_NAME);
		return erNoError;
	}
	else if( query.objID == rdiModelName )
	{
		reply.mf = 0xFF;
		reply.conformity = rdevRegularDevice;
		reply.addData(query.objID, "MBSlaveEcho");
		return erNoError;
	}
	else if( query.objID == rdiUserApplicationName )
	{
		reply.mf = 0xFF;
		reply.conformity = rdevRegularDevice;
		reply.addData(query.objID, "uniset-mbrtuslave-echo");
		return erNoError;
	}

	return ModbusRTU::erBadDataAddress;
}
// -------------------------------------------------------------------------


