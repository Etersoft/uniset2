/*! $Id: MBTCPServer.cc,v 1.1 2008/11/22 23:22:23 vpashka Exp $ */
// -------------------------------------------------------------------------
//#include <sys/time.h>
//#include <string.h>
//#include <errno.h>
#include <sstream>
#include <UniSetTypes.h>
#include "MBTCPServer.h"
// -------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace ModbusRTU;
// -------------------------------------------------------------------------
MBTCPServer::MBTCPServer( ModbusAddr myaddr, const string inetaddr, int port, bool verb ):
	sslot(NULL),
	addr(myaddr),
//	prev(ModbusRTU::erNoError),
//	askCount(0),
	verbose(verb)
{
	cout << "$Id: MBTCPServer.cc,v 1.1 2008/11/22 23:22:23 vpashka Exp $" << endl;

//	int replyTimeout = uni_atoi( conf->getArgParam("--reply-timeout",it.getProp("reply_timeout")).c_str() );
//	if( replyTimeout <= 0 )
//		replyTimeout = 2000;

	ost::InetAddress ia(inetaddr.c_str());
	

	if( verbose )
		cout << "(init): "
			<< " addr: " << ia << ":" << port << endl;

	sslot 	= new ModbusTCPServerSlot(ia,port);

//	sslot->initLog(conf,name,logfile);
	
	sslot->connectReadCoil( sigc::mem_fun(this, &MBTCPServer::readCoilStatus) );
	sslot->connectReadInputStatus( sigc::mem_fun(this, &MBTCPServer::readInputStatus) );
	sslot->connectReadOutput( sigc::mem_fun(this, &MBTCPServer::readOutputRegisters) );
	sslot->connectReadInput( sigc::mem_fun(this, &MBTCPServer::readInputRegisters) );
	sslot->connectForceSingleCoil( sigc::mem_fun(this, &MBTCPServer::forceSingleCoil) );
	sslot->connectForceCoils( sigc::mem_fun(this, &MBTCPServer::forceMultipleCoils) );
	sslot->connectWriteOutput( sigc::mem_fun(this, &MBTCPServer::writeOutputRegisters) );
	sslot->connectWriteSingleOutput( sigc::mem_fun(this, &MBTCPServer::writeOutputSingleRegister) );
	sslot->connectJournalCommand( sigc::mem_fun(this, &MBTCPServer::journalCommand) );
	sslot->connectSetDateTime( sigc::mem_fun(this, &MBTCPServer::setDateTime) );
	sslot->connectRemoteService( sigc::mem_fun(this, &MBTCPServer::remoteService) );
	sslot->connectFileTransfer( sigc::mem_fun(this, &MBTCPServer::fileTransfer) );	

	sslot->setRecvTimeout(6000);
//	sslot->setAfterSendPause(afterSend);
	sslot->setReplyTimeout(10000);

	// build file list...
}

// -------------------------------------------------------------------------
MBTCPServer::~MBTCPServer()
{
	delete sslot;
}
// -------------------------------------------------------------------------
void MBTCPServer::setLog( DebugStream& dlog )
{
	if( sslot )
		sslot->setLog(dlog);
}
// -------------------------------------------------------------------------
void MBTCPServer::execute()
{
	// Работа...
	while(1)
	{
		ModbusRTU::mbErrCode res = sslot->receive( addr, UniSetTimer::WaitUpTime );
#if 0
		// собираем статистику обмена
		if( prev!=ModbusRTU::erTimeOut )
		{
			//  с проверкой на переполнение
			askCount = askCount>=numeric_limits<long>::max() ? 0 : askCount+1;
			if( res!=ModbusRTU::erNoError )			
				errmap[res]++;
		
			prev = res;
		}
#endif
		
		if( verbose && res!=ModbusRTU::erNoError && res!=ModbusRTU::erTimeOut )
			cerr << "(wait): " << ModbusRTU::mbErr2Str(res) << endl;
	}
}
// -------------------------------------------------------------------------
void MBTCPServer::sigterm( int signo )
{
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBTCPServer::readCoilStatus( ReadCoilMessage& query, 
												ReadCoilRetMessage& reply )
{
	if( verbose )
		cout << "(readCoilStatus): " << query << endl;

	ModbusRTU::DataBits d;
	d.b[0] = 1;
	d.b[2] = 1;
	d.b[4] = 1;
	d.b[6] = 1;

	if( query.count <= 1 )
	{
		reply.addData(d);
		return ModbusRTU::erNoError;
	}

	// Фомирование ответа:
	int num=0; // добавленное количество данных
	ModbusData reg = query.start;
	for( ; num<query.count; num++, reg++ )
		reply.addData(d);

	// Если мы в начале проверили, что запрос входит в разрешёный диапазон
	// то теоретически этой ситуации возникнуть не может...
	if( reply.bcnt < query.count )
	{
		cerr << "(readCoilStatus): Получили меньше чем ожидали. "
			<< " Запросили " << query.count << " получили " << reply.bcnt << endl;
	}

	return ModbusRTU::erNoError;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBTCPServer::readInputStatus( ReadInputStatusMessage& query, 
												ReadInputStatusRetMessage& reply )
{
	if( verbose )
		cout << "(readInputStatus): " << query << endl;

	ModbusRTU::DataBits d;
	d.b[0] = 1;
	d.b[3] = 1;
	d.b[7] = 1;


	if( query.count <= 1 )
	{
		reply.addData(d);
		return ModbusRTU::erNoError;
	}

	// Фомирование ответа:
	int num=0; // добавленное количество данных
	ModbusData reg = query.start;
	for( ; num<query.count; num++, reg++ )
		reply.addData(d);

	// Если мы в начале проверили, что запрос входит в разрешёный диапазон
	// то теоретически этой ситуации возникнуть не может...
	if( reply.bcnt < query.count )
	{
		cerr << "(readInputStatus): Получили меньше чем ожидали. "
			<< " Запросили " << query.count << " получили " << reply.bcnt << endl;
	}

	return ModbusRTU::erNoError;
}
// -------------------------------------------------------------------------
mbErrCode MBTCPServer::readInputRegisters( ReadInputMessage& query, 
										ReadInputRetMessage& reply )
{
	if( verbose )
		cout << "(readInputRegisters): " << query << endl;

	if( query.count <= 1 )
	{
		reply.addData(query.start);
		return ModbusRTU::erNoError;
	}

	// Фомирование ответа:
	int num=0; // добавленное количество данных
	ModbusData reg = query.start;
	for( ; num<query.count; num++, reg++ )
		reply.addData(reg);

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
ModbusRTU::mbErrCode MBTCPServer::readOutputRegisters( 
				ModbusRTU::ReadOutputMessage& query, ModbusRTU::ReadOutputRetMessage& reply )
{
	if( verbose )
		cout << "(readOutputRegisters): " << query << endl;

	if( query.count <= 1 )
	{
		reply.addData(query.start);
		return ModbusRTU::erNoError;
	}

	// Фомирование ответа:
	int num=0; // добавленное количество данных
	ModbusData reg = query.start;
	for( ; num<query.count; num++, reg++ )
		reply.addData(reg);

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
ModbusRTU::mbErrCode MBTCPServer::forceMultipleCoils( ModbusRTU::ForceCoilsMessage& query, 
													ModbusRTU::ForceCoilsRetMessage& reply )
{
	if( verbose )
		cout << "(forceMultipleCoils): " << query << endl;

	ModbusRTU::mbErrCode ret = ModbusRTU::erNoError;
	reply.set(query.start,query.quant);
	return ret;
}													
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBTCPServer::writeOutputRegisters( ModbusRTU::WriteOutputMessage& query,
											ModbusRTU::WriteOutputRetMessage& reply )
{
	if( verbose )
		cout << "(writeOutputRegisters): " << query << endl;

	ModbusRTU::mbErrCode ret = ModbusRTU::erNoError;
	reply.set(query.start,query.quant);
	return ret;
}

// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBTCPServer::writeOutputSingleRegister( ModbusRTU::WriteSingleOutputMessage& query,
											ModbusRTU::WriteSingleOutputRetMessage& reply )
{
	if( verbose )
		cout << "(writeOutputSingleRegisters): " << query << endl;

	ModbusRTU::mbErrCode ret = ModbusRTU::erNoError;
	reply.set(query.start,query.data);
	return ret;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBTCPServer::forceSingleCoil( ModbusRTU::ForceSingleCoilMessage& query,
											ModbusRTU::ForceSingleCoilRetMessage& reply )
{
	if( verbose )
		cout << "(forceSingleCoil): " << query << endl;

	ModbusRTU::mbErrCode ret = ModbusRTU::erNoError;
	reply.set(query.start,query.cmd());
	return ret;
}

// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBTCPServer::journalCommand( ModbusRTU::JournalCommandMessage& query, 
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
			ModbusRTU::JournalCommandRetOK::set(reply,1,0);
			return ModbusRTU::erNoError;	
		}
		break;

		case 2: // write по modbus пока не поддерживается
		default:
		{
			// формируем ответ		
			ModbusRTU::JournalCommandRetOK::set(reply,2,1);
			return ModbusRTU::erNoError;	
		}
		break;
	}

	return ModbusRTU::erTimeOut;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBTCPServer::setDateTime( ModbusRTU::SetDateTimeMessage& query, 
									ModbusRTU::SetDateTimeRetMessage& reply )
{
	if( verbose )
		cout << "(setDateTime): " << query << endl;

	// подтверждаем сохранение
	// в ответе возвращаем установленное время...
	ModbusRTU::SetDateTimeRetMessage::cpy(reply,query);
	return ModbusRTU::erNoError;
}				
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBTCPServer::remoteService( ModbusRTU::RemoteServiceMessage& query, 
									ModbusRTU::RemoteServiceRetMessage& reply )
{
	cerr << "(remoteService): " << query << endl;
	return ModbusRTU::erOperationFailed;
}									
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBTCPServer::fileTransfer( ModbusRTU::FileTransferMessage& query, 
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
	if( fd <= 0 )
	{
		dlog[Debug::WARN] << "(fileTransfer): open '" << fname << "' with error: " << strerror(errno) << endl;
		return ModbusRTU::erOperationFailed;
	}

	int seek = query.numpacket*ModbusRTU::FileTransferRetMessage::MaxDataLen;
	(void)lseek(fd,seek,SEEK_SET);

	ModbusRTU::ModbusByte buf[ModbusRTU::FileTransferRetMessage::MaxDataLen];

	int ret = ::read(fd,&buf,sizeof(buf));
	if( ret < 0 )
	{
		dlog[Debug::WARN] << "(fileTransfer): read from '" << fname << "' with error: " << strerror(errno) << endl;
		close(fd);
		return ModbusRTU::erOperationFailed;
	}

	// вычисляем общий размер файла в "пакетах"
//	(void)lseek(fd,0,SEEK_END);
//	int numpacks = lseek(fd,0,SEEK_CUR) / ModbusRTU::FileTransferRetMessage::MaxDataLen;
//	if( lseek(fd,0,SEEK_CUR) % ModbusRTU::FileTransferRetMessage::MaxDataLen )
//		numpacks++;

	struct stat fs;
	if( fstat(fd,&fs) < 0 )
	{
		dlog[Debug::WARN] << "(fileTransfer): fstat for '" << fname << "' with error: " << strerror(errno) << endl;
		close(fd);
		return ModbusRTU::erOperationFailed;
	}
	close(fd);	

//	cerr << "******************* ret = " << ret << " fsize = " << fs.st_size 
//		<< " maxsize = " << ModbusRTU::FileTransferRetMessage::MaxDataLen << endl;

	int numpacks = fs.st_size / ModbusRTU::FileTransferRetMessage::MaxDataLen;
	if( fs.st_size % ModbusRTU::FileTransferRetMessage::MaxDataLen )
		numpacks++;
	
	if( !reply.set(query.numfile,numpacks,query.numpacket,buf,ret) )
	{
		dlog[Debug::WARN] << "(fileTransfer): set date failed..." << endl;
		return ModbusRTU::erOperationFailed;
	}

	return ModbusRTU::erNoError;	
#endif

}									
// -------------------------------------------------------------------------
