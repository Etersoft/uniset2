// -------------------------------------------------------------------------
#include <sstream>
#include <limits>
#include <Poco/Net/NetException.h>
#include "UniSetTypes.h"
#include "MBTCPTestServer.h"
#include "VTypes.h"
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
std::ostream& operator<<(std::ostream& os, const MBTCPTestServer* m )
{
    return os << m->myname;
}
// -------------------------------------------------------------------------
MBTCPTestServer::MBTCPTestServer( const std::unordered_set<ModbusAddr>& _vaddr, const string& inetaddr, int port, bool verb ):
    sslot(NULL),
    vaddr(_vaddr),
    verbose(verb),
    replyVal(std::numeric_limits<uint32_t>::max()),
    forceSingleCoilCmd(false),
    lastWriteOutputSingleRegister(0),
    lastForceCoilsQ(0, 0),
    lastWriteOutputQ(0, 0),
    disabled(false)
{
    if( verbose )
        cout << "(MBTCPTestServer::init): "
             << " addr: " << inetaddr << ":" << port << endl;

    {
        ostringstream s;
        s << inetaddr << ":" << port;
        myname = s.str();
    }

    try
    {
        sslot = new ModbusTCPServerSlot(inetaddr, port);
    }
    catch( const Poco::Net::NetException& e )
    {
        ostringstream err;
        err << "(MBTCPTestServer::init): Can`t create socket " << inetaddr << ":" << port << " err: " << e.message() << endl;
        cerr << err.str() << endl;
        throw SystemError(err.str());
    }
    catch( const std::exception& ex )
    {
        cerr << "(MBTCPTestServer::init): Can`t create socket " << inetaddr << ":" << port << " err: " << ex.what() << endl;
        throw;
    }

    //    sslot->initLog(conf,name,logfile);

    sslot->connectReadCoil( sigc::mem_fun(this, &MBTCPTestServer::readCoilStatus) );
    sslot->connectReadInputStatus( sigc::mem_fun(this, &MBTCPTestServer::readInputStatus) );
    sslot->connectReadOutput( sigc::mem_fun(this, &MBTCPTestServer::readOutputRegisters) );
    sslot->connectReadInput( sigc::mem_fun(this, &MBTCPTestServer::readInputRegisters) );
    sslot->connectForceSingleCoil( sigc::mem_fun(this, &MBTCPTestServer::forceSingleCoil) );
    sslot->connectForceCoils( sigc::mem_fun(this, &MBTCPTestServer::forceMultipleCoils) );
    sslot->connectWriteOutput( sigc::mem_fun(this, &MBTCPTestServer::writeOutputRegisters) );
    sslot->connectWriteSingleOutput( sigc::mem_fun(this, &MBTCPTestServer::writeOutputSingleRegister) );
    sslot->connectDiagnostics( sigc::mem_fun(this, &MBTCPTestServer::diagnostics) );
    sslot->connectMEIRDI( sigc::mem_fun(this, &MBTCPTestServer::read4314) );
    sslot->connectJournalCommand( sigc::mem_fun(this, &MBTCPTestServer::journalCommand) );
    sslot->connectSetDateTime( sigc::mem_fun(this, &MBTCPTestServer::setDateTime) );
    sslot->connectRemoteService( sigc::mem_fun(this, &MBTCPTestServer::remoteService) );
    sslot->connectFileTransfer( sigc::mem_fun(this, &MBTCPTestServer::fileTransfer) );

    sslot->setRecvTimeout(6000);
    sslot->setReplyTimeout(10000);

    // build file list...
}

// -------------------------------------------------------------------------
MBTCPTestServer::~MBTCPTestServer()
{
    if( sslot )
        sslot->terminate();
}
// -------------------------------------------------------------------------
void MBTCPTestServer::setLog( std::shared_ptr<DebugStream> dlog )
{
    if( sslot )
        sslot->setLog(dlog);
}
// -------------------------------------------------------------------------
void MBTCPTestServer::execute()
{
    if( sslot )
        sslot->async_run( vaddr );
}
// -------------------------------------------------------------------------
void MBTCPTestServer::sigterm( int signo )
{
    if( sslot )
        sslot->terminate();
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBTCPTestServer::readCoilStatus( ReadCoilMessage& query,
        ReadCoilRetMessage& reply )
{
    if( disabled )
        return ModbusRTU::erTimeOut;

    if( verbose )
        cout << "(readCoilStatus): " << query << endl;

    ModbusRTU::DataBits d;
    d.b[0] = 1;
    d.b[2] = 1;
    d.b[4] = 1;
    d.b[6] = 1;

    if( query.count == 0 )
    {
        if( verbose )
            cout << "(readCoilStatus): ERROR qount=0" << endl;

        return ModbusRTU::erBadDataValue;
    }

    if( query.count <= 1 )
    {
        if( replyVal != std::numeric_limits<uint32_t>::max() )
            reply.addData(replyVal);
        else
            reply.addData(d);

        return ModbusRTU::erNoError;
    }

    // Фомирование ответа:
    size_t num = 0; // добавленное количество данных
    size_t bcnt = numBytes(query.count);

    for( ; num < bcnt; num++ )
    {
        if( replyVal != std::numeric_limits<uint32_t>::max() )
            reply.addData(replyVal);
        else
            reply.addData(d);
    }

    return ModbusRTU::erNoError;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBTCPTestServer::readInputStatus( ReadInputStatusMessage& query,
        ReadInputStatusRetMessage& reply )
{
    if( disabled )
        return ModbusRTU::erTimeOut;

    if( verbose )
        cout << "(readInputStatus): " << query << endl;

    ModbusRTU::DataBits d;
    d.b[0] = 1;
    d.b[3] = 1;
    d.b[7] = 1;

    if( query.count == 0 )
    {
        if( verbose )
            cout << "(readInputStatus): ERROR qount=0" << endl;

        return ModbusRTU::erBadDataValue;
    }

    if( replyVal == std::numeric_limits<uint32_t>::max() )
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
            reply.addData(replyVal);
    }

    return ModbusRTU::erNoError;
}
// -------------------------------------------------------------------------
mbErrCode MBTCPTestServer::readInputRegisters( ReadInputMessage& query,
        ReadInputRetMessage& reply )
{
    if( disabled )
        return ModbusRTU::erTimeOut;

    if( verbose )
        cout << "(readInputRegisters): " << query << endl;

    if( query.count <= 1 )
    {
        if( replyVal != std::numeric_limits<uint32_t>::max() )
            reply.addData(replyVal);
        else
            reply.addData(query.start);

        return ModbusRTU::erNoError;
    }

    // Фомирование ответа:
    size_t num = 0; // добавленное количество данных
    ModbusData reg = query.start;

    for( ; num < query.count; num++, reg++ )
    {
        if( replyVal != std::numeric_limits<uint32_t>::max() )
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
ModbusRTU::mbErrCode MBTCPTestServer::readOutputRegisters(
    ModbusRTU::ReadOutputMessage& query, ModbusRTU::ReadOutputRetMessage& reply )
{
    if( disabled )
        return ModbusRTU::erTimeOut;

    if( verbose )
        cout << "(readOutputRegisters): " << query << endl;

    if( query.count <= 1 )
    {
        if( replyVal != std::numeric_limits<uint32_t>::max() )
            reply.addData(replyVal);
        else
            reply.addData(query.start);

        return ModbusRTU::erNoError;
    }

    // Фомирование ответа:
    size_t num = 0; // добавленное количество данных
    ModbusData reg = query.start;

    for( ; num < query.count; num++, reg++ )
    {
        if( replyVal != std::numeric_limits<uint32_t>::max() )
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
ModbusRTU::mbErrCode MBTCPTestServer::forceMultipleCoils( ModbusRTU::ForceCoilsMessage& query,
        ModbusRTU::ForceCoilsRetMessage& reply )
{
    if( disabled )
        return ModbusRTU::erTimeOut;

    if( verbose )
        cout << "(forceMultipleCoils): " << query << endl;

    ModbusRTU::mbErrCode ret = ModbusRTU::erNoError;
    reply.set(query.start, query.quant);
    lastForceCoilsQ = query;
    return ret;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBTCPTestServer::writeOutputRegisters( ModbusRTU::WriteOutputMessage& query,
        ModbusRTU::WriteOutputRetMessage& reply )
{
    if( disabled )
        return ModbusRTU::erTimeOut;

    if( verbose )
        cout << "(writeOutputRegisters): " << query << endl;

    ModbusRTU::mbErrCode ret = ModbusRTU::erNoError;
    reply.set(query.start, query.quant);
    lastWriteOutputQ = query;

    if( query.start == 41 )
    {
        VTypes::F2 f2(query.data, VTypes::F2::wsize());
        f2_test_value = (float)f2;
    }

    return ret;
}

// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBTCPTestServer::writeOutputSingleRegister( ModbusRTU::WriteSingleOutputMessage& query,
        ModbusRTU::WriteSingleOutputRetMessage& reply )
{
    if( disabled )
        return ModbusRTU::erTimeOut;

    if( verbose )
        cout << "(writeOutputSingleRegisters): " << query << endl;

    ModbusRTU::mbErrCode ret = ModbusRTU::erNoError;
    lastWriteOutputSingleRegister = (signed short)query.data;
    reply.set(query.start, query.data);
    return ret;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBTCPTestServer::forceSingleCoil( ModbusRTU::ForceSingleCoilMessage& query,
        ModbusRTU::ForceSingleCoilRetMessage& reply )
{
    if( disabled )
        return ModbusRTU::erTimeOut;

    if( verbose )
        cout << "(forceSingleCoil): " << query << endl;

    ModbusRTU::mbErrCode ret = ModbusRTU::erNoError;
    forceSingleCoilCmd = query.cmd();
    reply.set(query.start, query.cmd());
    return ret;
}

// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBTCPTestServer::journalCommand( ModbusRTU::JournalCommandMessage& query,
        ModbusRTU::JournalCommandRetMessage& reply )
{
    if( disabled )
        return ModbusRTU::erTimeOut;

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
ModbusRTU::mbErrCode MBTCPTestServer::setDateTime( ModbusRTU::SetDateTimeMessage& query,
        ModbusRTU::SetDateTimeRetMessage& reply )
{
    if( disabled )
        return ModbusRTU::erTimeOut;

    if( verbose )
        cout << "(setDateTime): " << query << endl;

    // подтверждаем сохранение
    // в ответе возвращаем установленное время...
    ModbusRTU::SetDateTimeRetMessage::cpy(reply, query);
    return ModbusRTU::erNoError;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBTCPTestServer::remoteService( ModbusRTU::RemoteServiceMessage& query,
        ModbusRTU::RemoteServiceRetMessage& reply )
{
    cerr << "(remoteService): " << query << endl;
    return ModbusRTU::erOperationFailed;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode MBTCPTestServer::fileTransfer( ModbusRTU::FileTransferMessage& query,
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
ModbusRTU::mbErrCode MBTCPTestServer::diagnostics( ModbusRTU::DiagnosticMessage& query,
        ModbusRTU::DiagnosticRetMessage& reply )
{
    if( disabled )
        return ModbusRTU::erTimeOut;

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
ModbusRTU::mbErrCode MBTCPTestServer::read4314( ModbusRTU::MEIMessageRDI& query,
        ModbusRTU::MEIMessageRetRDI& reply )
{
    if( disabled )
        return ModbusRTU::erTimeOut;

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
