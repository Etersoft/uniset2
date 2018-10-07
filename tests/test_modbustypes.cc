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
