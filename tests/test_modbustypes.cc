#include <catch.hpp>

#include <limits>
#include "modbus/ModbusTypes.h"
using namespace std;
using namespace uniset;
// ---------------------------------------------------------------
TEST_CASE("WriteOutputMessage", "[modbus][WriteOutputMessage]" )
{
    SECTION("WriteOutputMessage: limit the amount of data verification")
    {
        ModbusRTU::WriteOutputMessage msg(0x01, 1);

        for( int i = 0; i < (ModbusRTU::MAXDATALEN - 1); i++ )
            msg.addData(10 + i);

        CHECK_FALSE( msg.isFull() );
        msg.addData(1);
        CHECK( msg.isFull() );
    }

    WARN("Tests for 'Modbus types' incomplete..");
}
// ---------------------------------------------------------------
TEST_CASE("Modbus  helpers", "[modbus][helpers]" )
{
    using namespace ModbusRTU;

    SECTION("str2mbAddr")
    {
        REQUIRE( str2mbAddr("0x01") == 0x01 );
        REQUIRE( str2mbAddr("255") == 255 );
        REQUIRE( str2mbAddr("2") == 2 );
        REQUIRE( str2mbAddr("-1") == 255 );
        REQUIRE( str2mbAddr("") == 0 );
        REQUIRE( str2mbAddr("4") == 0x04 );
        REQUIRE( str2mbAddr("0xA") == 10 );
        REQUIRE( str2mbAddr("10") == 0xA );
    }

    SECTION("str2mbAddr")
    {
        REQUIRE( str2mbData("") == 0 );
        REQUIRE( str2mbData("0x01") == 0x01 );
        REQUIRE( str2mbData("10") == 0xA );
        REQUIRE( str2mbData("65535") == 65535 );
        REQUIRE( str2mbData("-1") == 65535 );
    }

    SECTION("dat2str")
    {
        REQUIRE( dat2str(0) == "0" );
        REQUIRE( dat2str(10) == "0xa" );
        REQUIRE( dat2str(2) == "0x2" );
        REQUIRE( dat2str(65535) == "0xffff" );
        REQUIRE( dat2str(-1) == "0xffff" );
    }

    SECTION("addr2str")
    {
        REQUIRE( addr2str(0x1) == "0x01" );
        REQUIRE( addr2str(10) == "0x0a" );    // ожидаемо ли? что dat2str = 0xa.. а addr2str = 0x0a
        REQUIRE( addr2str(255) == "0xff" );
        REQUIRE( addr2str(-1) == "0xff" );
    }

    SECTION("b2str")
    {
        REQUIRE( b2str(0x1) == "01" );
        REQUIRE( b2str(10) == "0a" );
        REQUIRE( b2str(255) == "ff" );
        REQUIRE( b2str(-1) == "ff" );
    }
}
// ---------------------------------------------------------------
#if 0
/*! \TODO Надо ещё подумать как тут протестировать */
TEST_CASE("dat2f", "[modbus]" )
{
    using namespace ModbusRTU;
    REQUIRE( dat2f(0, 0) == 0.0f );
    //    REQUIRE( dat2f(1,0) == 0.0f );
    //    REQUIRE( dat2f(0xff,0xff) == 0.0f );
}
#endif
// ---------------------------------------------------------------
TEST_CASE("isWriteFunction", "[modbus][isWriteFunction]" )
{
    using namespace ModbusRTU;

    CHECK_FALSE( isWriteFunction(fnUnknown) );
    CHECK_FALSE( isWriteFunction(fnReadCoilStatus) );
    CHECK_FALSE( isWriteFunction(fnReadInputStatus) );
    CHECK_FALSE( isWriteFunction(fnReadOutputRegisters) );
    CHECK_FALSE( isWriteFunction(fnReadInputRegisters) );
    CHECK( isWriteFunction(fnForceSingleCoil) );
    CHECK( isWriteFunction(fnWriteOutputSingleRegister) );
    CHECK_FALSE( isWriteFunction(fnDiagnostics) );
    CHECK( isWriteFunction(fnForceMultipleCoils) );
    CHECK( isWriteFunction(fnWriteOutputRegisters) );
    CHECK_FALSE( isWriteFunction(fnReadFileRecord) );
    CHECK_FALSE( isWriteFunction(fnWriteFileRecord) );
    CHECK_FALSE( isWriteFunction(fnMEI) );
    CHECK_FALSE( isWriteFunction(fnSetDateTime) );
    CHECK_FALSE( isWriteFunction(fnRemoteService) );
    CHECK_FALSE( isWriteFunction(fnJournalCommand) );
    CHECK_FALSE( isWriteFunction(fnFileTransfer) );
}
// ---------------------------------------------------------------
TEST_CASE("checkCRC", "[modbus][checkCRC]" )
{
    // ModbusCRC checkCRC( ModbusByte* start, int len );
}
// ---------------------------------------------------------------
TEST_CASE("numBytes function", "[modbus][numbytes]" )
{
    REQUIRE( ModbusRTU::numBytes(0) == 0 );
    REQUIRE( ModbusRTU::numBytes(1) == 1 );
    REQUIRE( ModbusRTU::numBytes(8) == 1 );

    REQUIRE( ModbusRTU::numBytes(10) == 2 );
    REQUIRE( ModbusRTU::numBytes(16) == 2 );

    REQUIRE( ModbusRTU::numBytes(256) == 32 );
    REQUIRE( ModbusRTU::numBytes(257) == 33 );
}
// ---------------------------------------------------------------
TEST_CASE("genRegID", "[modbus][genRegID]" )
{
    size_t max_reg = numeric_limits<ModbusRTU::ModbusData>::max();
    size_t max_fn = numeric_limits<ModbusRTU::ModbusByte>::max();

    ModbusRTU::RegID prevID = ModbusRTU::genRegID(0, 0);

    for( size_t f = 1; f < max_fn; f++ )
    {
        ModbusRTU::RegID minID = ModbusRTU::genRegID(0, f);

        // для каждого нового номера функции должен быть свой диапазон
        // не пересекающийся с другими
        REQUIRE( minID > (prevID + max_reg - 1) );

        prevID = minID;

        for( size_t r = 1; r < max_reg; r++ )
            REQUIRE( ModbusRTU::genRegID(r, f) == minID + r );
    }
}
// ---------------------------------------------------------------
TEST_CASE("BitsBuffer data manipulation", "[modbus][bitsBuffer]" )
{
    ModbusRTU::BitsBuffer bits;
    REQUIRE_FALSE(bits.isFull());

    // at first ADD DATA
    ModbusRTU::DataBits d1;
    d1.set(0, true);
    REQUIRE(bits.addData(d1));

    ModbusRTU::DataBits d2;
    d2.set(0, true);
    REQUIRE(bits.addData(d2));

    uint8_t dnum = 0;
    uint8_t bnum = 0;
    bool state = false;

    REQUIRE( bits.getBit(dnum, bnum, state) );
    REQUIRE( state );

    // reset bit
    REQUIRE( bits.setBit(dnum, dnum, false) );
    state = true;
    REQUIRE( bits.getBit(dnum, bnum, state) );
    REQUIRE_FALSE( state );

    dnum = ModbusRTU::MAXLENPACKET / 2;
    bnum = 6;
    REQUIRE_FALSE( bits.setBit(dnum, bnum, true) );
    REQUIRE_FALSE( bits.getBit(dnum, bnum, state) );
}
// ---------------------------------------------------------------
TEST_CASE("BitsBuffer bits manipulation", "[modbus][bitsBuffer]" )
{
    ModbusRTU::BitsBuffer bits;
    REQUIRE_FALSE(bits.isFull());

    REQUIRE( bits.setByBitNum(0, true) );
    bool state = false;
    REQUIRE( bits.getByBitNum(0, state) );
    REQUIRE( state );
    REQUIRE( bits.bcnt == 1 );

    uint16_t nbit = (ModbusRTU::MAXPDULEN / 2) * 8;
    REQUIRE( bits.setByBitNum(nbit, true) );
    REQUIRE( bits.getByBitNum(nbit, state) );
    REQUIRE( state );
    REQUIRE( bits.setByBitNum(nbit, false) );
    REQUIRE( bits.getByBitNum(nbit, state) );
    REQUIRE_FALSE( state );
    REQUIRE( (size_t)(bits.bcnt) == (ModbusRTU::MAXPDULEN / 2) + 1 );

    nbit = (ModbusRTU::MAXPDULEN * 8) - 1;
    REQUIRE( bits.setByBitNum(nbit, true) );
    REQUIRE( bits.getByBitNum(nbit, state) );
    REQUIRE(state);

    nbit = (ModbusRTU::MAXPDULEN * 8);
    REQUIRE_FALSE( bits.setByBitNum(nbit, true) );
    state = true;
    REQUIRE_FALSE( bits.getByBitNum(nbit, state) );
    REQUIRE(state); // no change state
}
// ---------------------------------------------------------------
TEST_CASE("ReadCoilRetMessage", "[modbus][coil message]" )
{
    ModbusRTU::ModbusMessage buf;
    buf.pduhead.addr = 1;
    buf.pduhead.func = ModbusRTU::fnReadCoilStatus;
    buf.data[0] = 2; // bcnt
    buf.data[1] = 1; // byte 1
    buf.data[2] = 255; // byte 2
    buf.data[3] = 2; // hi crc
    buf.data[4] = 2; // low crc

    ModbusRTU::ReadCoilRetMessage msg(buf);
    REQUIRE( msg.addr == 1 );
    REQUIRE( msg.func == ModbusRTU::fnReadCoilStatus );
    REQUIRE( msg.bcnt == 2 );
    bool state = false;
    REQUIRE( msg.getByBitNum(0, state) );
    REQUIRE( state );
    REQUIRE( msg.getByBitNum(1, state) );
    REQUIRE_FALSE( state );
    REQUIRE( msg.getByBitNum(10, state) );
    REQUIRE( state );

    // too long
    buf.data[0] = ModbusRTU::MAXPDULEN;
    REQUIRE_NOTHROW(ModbusRTU::ReadCoilRetMessage(buf) );
    buf.data[0] = ModbusRTU::MAXPDULEN + 1;
    REQUIRE_THROWS_AS(ModbusRTU::ReadCoilRetMessage(buf), ModbusRTU::mbException); // erPacketTooLong
}
// ---------------------------------------------------------------
TEST_CASE("ReadCoilMessage", "[modbus][coil message]" )
{
    ModbusRTU::ReadCoilMessage msg(1, 1, 100);
    REQUIRE( msg.addr == 1 );
    REQUIRE( msg.func == ModbusRTU::fnReadCoilStatus );
    REQUIRE( msg.start == 1 );
    REQUIRE( msg.count == 100 );

    ModbusRTU::ModbusMessage buf;
    buf.pduhead.addr = 2;
    buf.pduhead.func = ModbusRTU::fnReadCoilStatus;
    buf.data[0] = 0;
    buf.data[1] = 2;
    buf.data[2] = 0;
    buf.data[3] = 200;

    ModbusRTU::ReadCoilMessage msg2 = buf;
    REQUIRE( msg2.start == 2 );
    REQUIRE( msg2.count == 200 );

    ModbusRTU::ReadCoilMessage msg3(buf);
    REQUIRE( msg3.start == 2 );
    REQUIRE( msg3.count == 200 );

    // too long
    buf.data[2] = 255;
    buf.data[3] = 255;
    REQUIRE_THROWS_AS(ModbusRTU::ReadCoilMessage(buf), ModbusRTU::mbException); // erPacketTooLong
}
// ---------------------------------------------------------------
TEST_CASE("ReadInputStatusMessage", "[modbus][input status message]" )
{
    ModbusRTU::ReadInputStatusMessage msg(1, 1, 100);
    REQUIRE( msg.addr == 1 );
    REQUIRE( msg.func == ModbusRTU::fnReadInputStatus );
    REQUIRE( msg.start == 1 );
    REQUIRE( msg.count == 100 );

    ModbusRTU::ModbusMessage buf;
    buf.pduhead.addr = 2;
    buf.pduhead.func = ModbusRTU::fnReadInputStatus;
    buf.data[0] = 0; // start hi
    buf.data[1] = 1; // start low
    buf.data[2] = 7; // count hi
    buf.data[3] = 208; // count low

    ModbusRTU::ReadInputStatusMessage msg2 = buf;
    REQUIRE( msg2.start == 1 );
    REQUIRE( msg2.count == 2000 );

    ModbusRTU::ReadInputStatusMessage msg3(buf);
    REQUIRE( msg3.start == 1 );
    REQUIRE( msg3.count == 2000 );

    // too long
    buf.data[2] = 255;
    buf.data[3] = 255;
    REQUIRE_THROWS_AS(ModbusRTU::ReadInputStatusMessage(buf), ModbusRTU::mbException); // erPacketTooLong
}
// ---------------------------------------------------------------
TEST_CASE("ReadInputStatusRetMessage", "[modbus][input status message]" )
{
    ModbusRTU::ModbusMessage buf;
    buf.pduhead.addr = 1;
    buf.pduhead.func = ModbusRTU::fnReadInputStatus;
    buf.data[0] = 2; // bcnt
    buf.data[1] = 1; // byte 1
    buf.data[2] = 255; // byte 2
    buf.data[3] = 2; // hi crc
    buf.data[4] = 2; // low crc

    ModbusRTU::ReadInputStatusRetMessage msg(buf);
    REQUIRE( msg.addr == 1 );
    REQUIRE( msg.func == ModbusRTU::fnReadInputStatus );
    REQUIRE( msg.bcnt == 2 );
    bool state = false;
    REQUIRE( msg.getByBitNum(0, state) );
    REQUIRE( state );
    REQUIRE( msg.getByBitNum(1, state) );
    REQUIRE_FALSE( state );
    REQUIRE( msg.getByBitNum(10, state) );
    REQUIRE( state );

    // too long
    buf.data[0] = ModbusRTU::MAXPDULEN;
    REQUIRE_NOTHROW(ModbusRTU::ReadInputStatusRetMessage(buf) );
    buf.data[0] = ModbusRTU::MAXPDULEN + 1;
    REQUIRE_THROWS_AS(ModbusRTU::ReadInputStatusRetMessage(buf), ModbusRTU::mbException); // erPacketTooLong
}
// ---------------------------------------------------------------