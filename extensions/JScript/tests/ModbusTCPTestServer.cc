/*
 * Simplified Modbus TCP echo server for JS tests (based on Utilities/MBTester implementation).
 */
#include <sstream>
#include <random>
#include "UniSetTypes.h"
#include "modbus/ModbusTCPServerSlot.h"
#include "uniset-config.h"
#include "ModbusTCPTestServer.h"

using namespace std;
using namespace uniset;
using namespace ModbusRTU;

namespace
{
#ifndef PACKAGE_URL
#define PACKAGE_URL "http://git.etersoft.ru/projects/?p=asu/uniset.git;a=summary"
#endif
}

ModbusTCPServerTest::ModbusTCPServerTest( const std::unordered_set<ModbusRTU::ModbusAddr>& myaddr,
                                          const std::string& inetaddr, int port, bool verb ):
    sslot(nullptr),
    vaddr(myaddr),
    verbose(verb),
    replyVal(-1)
{
    sslot = new ModbusTCPServerSlot(inetaddr, port);

    sslot->connectReadCoil( sigc::mem_fun(this, &ModbusTCPServerTest::readCoilStatus) );
    sslot->connectReadInputStatus( sigc::mem_fun(this, &ModbusTCPServerTest::readInputStatus) );
    sslot->connectReadOutput( sigc::mem_fun(this, &ModbusTCPServerTest::readOutputRegisters) );
    sslot->connectReadInput( sigc::mem_fun(this, &ModbusTCPServerTest::readInputRegisters) );
    sslot->connectForceSingleCoil( sigc::mem_fun(this, &ModbusTCPServerTest::forceSingleCoil) );
    sslot->connectForceCoils( sigc::mem_fun(this, &ModbusTCPServerTest::forceMultipleCoils) );
    sslot->connectWriteOutput( sigc::mem_fun(this, &ModbusTCPServerTest::writeOutputRegisters) );
    sslot->connectWriteSingleOutput( sigc::mem_fun(this, &ModbusTCPServerTest::writeOutputSingleRegister) );
    sslot->connectDiagnostics( sigc::mem_fun(this, &ModbusTCPServerTest::diagnostics) );
    sslot->connectMEIRDI( sigc::mem_fun(this, &ModbusTCPServerTest::read4314) );
    sslot->connectJournalCommand( sigc::mem_fun(this, &ModbusTCPServerTest::journalCommand) );
    sslot->connectSetDateTime( sigc::mem_fun(this, &ModbusTCPServerTest::setDateTime) );
    sslot->connectRemoteService( sigc::mem_fun(this, &ModbusTCPServerTest::remoteService) );
    sslot->connectFileTransfer( sigc::mem_fun(this, &ModbusTCPServerTest::fileTransfer) );

    gen = std::make_unique<std::mt19937>(rnd());
}

ModbusTCPServerTest::~ModbusTCPServerTest()
{
    stop();
    delete sslot;
}

void ModbusTCPServerTest::setVerbose( bool state )
{
    verbose = state;
}

void ModbusTCPServerTest::setReply( long val )
{
    replyVal = val;
}

void ModbusTCPServerTest::setRandomReply( long min, long max )
{
    rndgen = make_unique<std::uniform_int_distribution<>>(min, max);
}

void ModbusTCPServerTest::setFreezeReply( const unordered_map<uint16_t, uint16_t>& rlist )
{
    reglist = rlist;
}

void ModbusTCPServerTest::setMaxSessions( size_t max )
{
    sslot->setMaxSessions(max);
}

bool ModbusTCPServerTest::execute()
{
    return sslot->async_run(vaddr);
}

void ModbusTCPServerTest::stop()
{
    if( sslot )
        sslot->terminate();
}

bool ModbusTCPServerTest::isActive() const
{
    return sslot && sslot->isActive();
}

ModbusRTU::mbErrCode ModbusTCPServerTest::readCoilStatus( const ReadCoilMessage& query,
        ReadCoilRetMessage& reply )
{
    if( verbose )
        cout << "(readCoilStatus): " << query << endl;

    ModbusRTU::DataBits d;
    d.b[0] = 1;
    d.b[2] = 1;
    d.b[4] = 1;
    d.b[6] = 1;

    if ( replyVal != -1 )
        d = ModbusRTU::DataBits(replyVal);

    size_t nbit = 0;
    ModbusData reg = query.start;

    for( ; nbit < query.count; nbit++, reg++ )
    {
        if( auto search = reglist.find(reg); search != reglist.end() )
            reply.setByBitNum(nbit, search->second);
        else if( rndgen )
            reply.setByBitNum(nbit, (*rndgen.get())(*gen.get()));
        else
            reply.setByBitNum(nbit, d.b[reg % ModbusRTU::BitsPerByte]);
    }

    if( verbose )
        cout << "(readCoilStatus): reply: " << reply << endl;

    return ModbusRTU::erNoError;
}

ModbusRTU::mbErrCode ModbusTCPServerTest::readInputStatus( const ReadInputStatusMessage& query,
        ReadInputStatusRetMessage& reply )
{
    if( verbose )
        cout << "(readInputStatus): " << query << endl;

    ModbusRTU::DataBits d;
    d.b[0] = 1;
    d.b[3] = 1;
    d.b[7] = 1;

    if ( replyVal != -1 )
        d = ModbusRTU::DataBits(replyVal);

    size_t nbit = 0;
    ModbusData reg = query.start;

    for( ; nbit < query.count; nbit++, reg++ )
    {
        if( auto search = reglist.find(reg); search != reglist.end() )
            reply.setByBitNum(nbit, search->second);
        else if( rndgen )
            reply.setByBitNum(nbit, (*rndgen.get())(*gen.get()));
        else
            reply.setByBitNum(nbit, d.b[reg % ModbusRTU::BitsPerByte]);
    }

    if( verbose )
        cout << "(readInputStatus): reply: " << reply << endl;

    return ModbusRTU::erNoError;
}

mbErrCode ModbusTCPServerTest::readInputRegisters( const ReadInputMessage& query,
        ReadInputRetMessage& reply )
{
    if( verbose )
        cout << "(readInputRegisters): " << query << endl;

    int num = 0;
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

    if( verbose )
        cout << "(readInputRegisters): reply: " << reply << endl;

    return ModbusRTU::erNoError;
}

ModbusRTU::mbErrCode ModbusTCPServerTest::readOutputRegisters(
    const ModbusRTU::ReadOutputMessage& query, ModbusRTU::ReadOutputRetMessage& reply )
{
    if( verbose )
        cout << "(readOutputRegisters): " << query << endl;

    int num = 0;
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

    if( verbose )
        cout << "(readOutputRegisters): reply: " << reply << endl;

    return ModbusRTU::erNoError;
}

ModbusRTU::mbErrCode ModbusTCPServerTest::forceMultipleCoils( const ModbusRTU::ForceCoilsMessage& query,
        ModbusRTU::ForceCoilsRetMessage& reply )
{
    reply.set(query.start, query.quant);
    return ModbusRTU::erNoError;
}

ModbusRTU::mbErrCode ModbusTCPServerTest::writeOutputRegisters( const ModbusRTU::WriteOutputMessage& query,
        ModbusRTU::WriteOutputRetMessage& reply )
{
    reply.set(query.start, query.quant);
    return ModbusRTU::erNoError;
}

ModbusRTU::mbErrCode ModbusTCPServerTest::writeOutputSingleRegister( const ModbusRTU::WriteSingleOutputMessage& query,
        ModbusRTU::WriteSingleOutputRetMessage& reply )
{
    reply.set(query.start, query.data);
    return ModbusRTU::erNoError;
}

ModbusRTU::mbErrCode ModbusTCPServerTest::forceSingleCoil( const ModbusRTU::ForceSingleCoilMessage& query,
        ModbusRTU::ForceSingleCoilRetMessage& reply )
{
    reply.set(query.start, query.cmd());
    return ModbusRTU::erNoError;
}

ModbusRTU::mbErrCode ModbusTCPServerTest::journalCommand( const ModbusRTU::JournalCommandMessage& query,
        ModbusRTU::JournalCommandRetMessage& reply )
{
    switch( query.cmd )
    {
        case 0:
        {
            if( reply.setData( (ModbusRTU::ModbusByte*)(&query.num), sizeof(query.num) ) )
                return ModbusRTU::erNoError;

            return ModbusRTU::erPacketTooLong;
        }

        case 1:
        {
            ModbusRTU::JournalCommandRetOK::set(reply, 1, 0);
            return ModbusRTU::erNoError;
        }

        default:
        {
            ModbusRTU::JournalCommandRetOK::set(reply, 2, 1);
            return ModbusRTU::erNoError;
        }
    }
}

ModbusRTU::mbErrCode ModbusTCPServerTest::setDateTime( const ModbusRTU::SetDateTimeMessage& query,
        ModbusRTU::SetDateTimeRetMessage& reply )
{
    ModbusRTU::SetDateTimeRetMessage::cpy(reply, query);
    return ModbusRTU::erNoError;
}

ModbusRTU::mbErrCode ModbusTCPServerTest::remoteService( const ModbusRTU::RemoteServiceMessage& query,
        ModbusRTU::RemoteServiceRetMessage& reply )
{
    (void)query;
    (void)reply;
    return ModbusRTU::erOperationFailed;
}

ModbusRTU::mbErrCode ModbusTCPServerTest::fileTransfer( const ModbusRTU::FileTransferMessage& query,
        ModbusRTU::FileTransferRetMessage& reply )
{
    (void)query;
    (void)reply;
    return ModbusRTU::erOperationFailed;
}

ModbusRTU::mbErrCode ModbusTCPServerTest::diagnostics( const ModbusRTU::DiagnosticMessage& query,
        ModbusRTU::DiagnosticRetMessage& reply )
{
    if( query.subf == ModbusRTU::subEcho )
    {
        reply = query;
        return ModbusRTU::erNoError;
    }

    reply = query;
    reply.data[0] = 10;
    return ModbusRTU::erNoError;
}

ModbusRTU::mbErrCode ModbusTCPServerTest::read4314( const ModbusRTU::MEIMessageRDI& query,
        ModbusRTU::MEIMessageRetRDI& reply )
{
    if( query.devID <= rdevMinNum || query.devID >= rdevMaxNum )
        return erOperationFailed;

    reply.mf = 0xFF;
    reply.conformity = rdevRegularDevice;

    if( query.objID == rdiVendorName )
        reply.addData(query.objID, "etersoft");
    else if( query.objID == rdiProductCode )
        reply.addData(query.objID, PACKAGE_NAME);
    else if( query.objID == rdiMajorMinorRevision )
        reply.addData(query.objID, PACKAGE_VERSION);
    else if( query.objID == rdiVendorURL )
        reply.addData(query.objID, PACKAGE_URL);
    else if( query.objID == rdiProductName )
        reply.addData(query.objID, PACKAGE_NAME);
    else if( query.objID == rdiModelName )
        reply.addData(query.objID, "MBTCPSlaveEcho");
    else if( query.objID == rdiUserApplicationName )
        reply.addData(query.objID, "uniset-mbtcpslave-echo");
    else
        return ModbusRTU::erBadDataAddress;

    return ModbusRTU::erNoError;
}
