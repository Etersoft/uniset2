// -------------------------------------------------------------------------
//#include <sys/time.h>
//#include <string.h>
//#include <errno.h>
#include <sstream>
#include <UniSetTypes.h>
#include "MBTCPServer.h"
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
MBTCPServer::MBTCPServer(const std::unordered_set<ModbusAddr>& myaddr, const string& inetaddr, int port, bool verb ):
    sslot(NULL),
    vaddr(myaddr),
    //    prev(ModbusRTU::erNoError),
    //    askCount(0),
    verbose(verb),
    replyVal(-1)
{
    //    int replyTimeout = uni_atoi( conf->getArgParam("--reply-timeout",it.getProp("reply_timeout")).c_str() );
    //    if( replyTimeout <= 0 )
    //        replyTimeout = 2000;

    if( verbose )
        cout << "(init): "
             << " addr: " << inetaddr << ":" << port << endl;

    sslot = new ModbusTCPServerSlot(inetaddr, port);

    //    sslot->initLog(conf,name,logfile);

    sslot->connectReadCoil( sigc::mem_fun(this, &MBTCPServer::readCoilStatus) );
    sslot->connectReadInputStatus( sigc::mem_fun(this, &MBTCPServer::readInputStatus) );
    sslot->connectReadOutput( sigc::mem_fun(this, &MBTCPServer::readOutputRegisters) );
    sslot->connectReadInput( sigc::mem_fun(this, &MBTCPServer::readInputRegisters) );
    sslot->connectForceSingleCoil( sigc::mem_fun(this, &MBTCPServer::forceSingleCoil) );
    sslot->connectForceCoils( sigc::mem_fun(this, &MBTCPServer::forceMultipleCoils) );
    sslot->connectWriteOutput( sigc::mem_fun(this, &MBTCPServer::writeOutputRegisters) );
    sslot->connectWriteSingleOutput( sigc::mem_fun(this, &MBTCPServer::writeOutputSingleRegister) );
    sslot->connectDiagnostics( sigc::mem_fun(this, &MBTCPServer::diagnostics) );
    sslot->connectMEIRDI( sigc::mem_fun(this, &MBTCPServer::read4314) );
    sslot->connectJournalCommand( sigc::mem_fun(this, &MBTCPServer::journalCommand) );
    sslot->connectSetDateTime( sigc::mem_fun(this, &MBTCPServer::setDateTime) );
    sslot->connectRemoteService( sigc::mem_fun(this, &MBTCPServer::remoteService) );
    sslot->connectFileTransfer( sigc::mem_fun(this, &MBTCPServer::fileTransfer) );

    //  sslot->setRecvTimeout(6000);
    //  sslot->setReplyTimeout(10000);

    // init random generator
    gen = std::make_unique<std::mt19937>(rnd());
}

// -------------------------------------------------------------------------
MBTCPServer::~MBTCPServer()
{
    delete sslot;
}
// -------------------------------------------------------------------------
void MBTCPServer::setLog(std::shared_ptr<DebugStream>& dlog )
{
    if( sslot )
        sslot->setLog(dlog);
}
// -------------------------------------------------------------------------
void MBTCPServer::setMaxSessions( size_t max )
{
    sslot->setMaxSessions(max);
}
// -------------------------------------------------------------------------
void MBTCPServer::setRandomReply( long min, long max )
{
    rndgen = make_unique<std::uniform_int_distribution<>>(min, max);
}
// -------------------------------------------------------------------------
void MBTCPServer::setFreezeReply( const unordered_map<uint16_t, uint16_t>& strreglist )
{
    reglist = strreglist;
}
// -------------------------------------------------------------------------
void MBTCPServer::execute()
{
    sslot->run(vaddr);
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
        if(auto search = reglist.find(query.start); search != reglist.end())
            reply.addData(search->second);
        else if( rndgen )
            reply.addData((*rndgen.get())(*gen.get()));
        else if( replyVal != -1 )
            reply.addData(replyVal);
        else
            reply.addData(d);

        return ModbusRTU::erNoError;
    }

    // Фомирование ответа:
    int num = 0; // добавленное количество данных
    ModbusData reg = query.start;

    for( ; num < query.count; num++, reg++ )
    {
        if(auto search = reglist.find(reg); search != reglist.end())
            reply.addData(search->second);
        else if( rndgen )
            reply.addData((*rndgen.get())(*gen.get()));
        else if( replyVal != -1 )
            reply.addData(replyVal);
        else
            reply.addData(d);
    }

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

    if( replyVal == -1 )
    {
        size_t bnum = 0;
        size_t i = 0;

        while( i < query.count )
        {
            reply.addData(0);

            for( size_t nbit = 0; nbit < BitsPerByte && i < query.count; nbit++, i++ )
                reply.setBit(bnum, nbit, d.b[nbit]);

            bnum++;
        }
    }
    else
    {

        size_t bcnt = ModbusRTU::numBytes(query.count);

        for( size_t i = 0; i < bcnt; i++ )
        {
            if(auto search = reglist.find(query.start); search != reglist.end())
                reply.addData(search->second);
            else if( rndgen )
                reply.addData((*rndgen.get())(*gen.get()));
            else
                reply.addData(replyVal);
        }
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
        if( rndgen )
            reply.addData((*rndgen.get())(*gen.get()));
        else if( replyVal != -1 )
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
        if(auto search = reglist.find(reg); search != reglist.end())
            reply.addData(search->second);
        else if( rndgen )
            reply.addData((*rndgen.get())(*gen.get()));
        else if( replyVal != -1 )
            reply.addData(replyVal);
        else
            reply.addData(reg);
    }

    //    cerr << "************ reply: cnt=" << reply.count << endl;
    //    cerr << "reply: " << reply << endl;

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
        if( rndgen )
            reply.addData((*rndgen.get())(*gen.get()));
        else if( replyVal != -1 )
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
        if(auto search = reglist.find(reg); search != reglist.end())
            reply.addData(search->second);
        else  if( rndgen )
            reply.addData((*rndgen.get())(*gen.get()));
        else if( replyVal != -1 )
            reply.addData(replyVal);
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
ModbusRTU::mbErrCode MBTCPServer::forceMultipleCoils( ModbusRTU::ForceCoilsMessage& query,
        ModbusRTU::ForceCoilsRetMessage& reply )
{
    if( verbose )
        cout << "(forceMultipleCoils): " << query << endl;

    ModbusRTU::mbErrCode ret = ModbusRTU::erNoError;
    reply.set(query.start, query.quant);
    return ret;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBTCPServer::writeOutputRegisters( ModbusRTU::WriteOutputMessage& query,
        ModbusRTU::WriteOutputRetMessage& reply )
{
    if( verbose )
        cout << "(writeOutputRegisters): " << query << endl;

    ModbusRTU::mbErrCode ret = ModbusRTU::erNoError;
    reply.set(query.start, query.quant);
    return ret;
}

// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBTCPServer::writeOutputSingleRegister( ModbusRTU::WriteSingleOutputMessage& query,
        ModbusRTU::WriteSingleOutputRetMessage& reply )
{
    if( verbose )
        cout << "(writeOutputSingleRegisters): " << query << endl;

    ModbusRTU::mbErrCode ret = ModbusRTU::erNoError;
    reply.set(query.start, query.data);
    return ret;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBTCPServer::forceSingleCoil( ModbusRTU::ForceSingleCoilMessage& query,
        ModbusRTU::ForceSingleCoilRetMessage& reply )
{
    if( verbose )
        cout << "(forceSingleCoil): " << query << endl;

    ModbusRTU::mbErrCode ret = ModbusRTU::erNoError;
    reply.set(query.start, query.cmd());
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
ModbusRTU::mbErrCode MBTCPServer::setDateTime( ModbusRTU::SetDateTimeMessage& query,
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

    const std::string fname(it->second);

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
        dwarn << "(fileTransfer): open '" << fname << "' with error: " << strerror(errno) << endl;
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
ModbusRTU::mbErrCode MBTCPServer::diagnostics( ModbusRTU::DiagnosticMessage& query,
        ModbusRTU::DiagnosticRetMessage& reply )
{
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
ModbusRTU::mbErrCode MBTCPServer::read4314( ModbusRTU::MEIMessageRDI& query,
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
        reply.addData(query.objID, "MBTCPSlaveEcho");
        return erNoError;
    }
    else if( query.objID == rdiUserApplicationName )
    {
        reply.mf = 0xFF;
        reply.conformity = rdevRegularDevice;
        reply.addData(query.objID, "uniset-mbtcpslave-echo");
        return erNoError;
    }

    return ModbusRTU::erBadDataAddress;
}
// -------------------------------------------------------------------------

