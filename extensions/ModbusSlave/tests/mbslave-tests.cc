#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <time.h>
#include "MBSlave.h"
#include "UniSetTypes.h"
#include "modbus/ModbusTCPMaster.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
// -----------------------------------------------------------------------------
static ModbusRTU::ModbusAddr slaveaddr = 0x01; // conf->getArgInt("--mbs-my-addr");
static int port = 20048; // conf->getArgInt("--mbs-inet-port");
static string addr("127.0.0.1"); // conf->getArgParam("--mbs-inet-addr");
static ObjectId slaveID = 6004; // conf->getObjectID( conf->getArgParam("--mbs-name"));
static ModbusTCPMaster* mb = nullptr;
static UInterface* ui = nullptr;
// -----------------------------------------------------------------------------
void InitTest()
{
	CHECK( conf!=0 );

	if( ui == nullptr )
	{
		ui = new UInterface();
		// UI понадобиться для проверки записанных в SM значений.
		CHECK( ui->getObjectIndex() != 0 );
		CHECK( ui->getConf() == UniSetTypes::conf );
		CHECK( ui->waitReady(slaveID,5000) );
	}

	if( mb == nullptr )
	{
		mb = new ModbusTCPMaster();
    	ost::InetAddress ia(addr.c_str()); 
    	mb->setTimeout(2000);
    	REQUIRE_NOTHROW( mb->connect(ia,port) );
	}
}
// -----------------------------------------------------------------------------
#if 0
TEST_CASE("(0x01): read coil status","[modbus][mbslave][mbtcpslave]")
{
	InitTest();
	// read 1 bit
	{
		ModbusRTU::ReadCoilRetMessage ret(slaveaddr);
		REQUIRE_NOTHROW( ret = mb->read01(slaveaddr,1000,1) );
		ModbusRTU::DataBits b(ret.data[0]);
		REQUIRE( b[0] == 1 );
	}
	// read 3 bit
	{
		ModbusRTU::ReadCoilRetMessage ret(slaveaddr);
		REQUIRE_NOTHROW( ret = mb->read01(slaveaddr,1000,3) );
		ModbusRTU::DataBits b(ret.data[0]);
		REQUIRE( b[0] == 1 );
		REQUIRE( b[1] == 1 );
		REQUIRE( b[2] == 0 );
	}
}

TEST_CASE("(0x02): read input status","[modbus][mbslave][mbtcpslave]")
{
	InitTest();
	SECTION("read 1 bit")
	{
		ModbusRTU::ReadInputStatusRetMessage ret(slaveaddr);
		REQUIRE_NOTHROW( ret = mb->read02(slaveaddr,1000,1) );
		ModbusRTU::DataBits b(ret.data[0]);
		REQUIRE( b[0] == 1 );
	}
	SECTION("read 3 bit")
	{
		ModbusRTU::ReadInputStatusRetMessage ret(slaveaddr);
		REQUIRE_NOTHROW( ret = mb->read02(slaveaddr,1000,3) );
		ModbusRTU::DataBits b(ret.data[0]);
		REQUIRE( b[0] == 1 );
		REQUIRE( b[1] == 1 );
		REQUIRE( b[2] == 0 );
	}
}
#endif

TEST_CASE("Function (0x03): 'read register outputs or memories or read word outputs or memories'","[modbus][mbslave][mbtcpslave]")
{
	InitTest();
	ModbusRTU::ModbusData tREG=10;
	SECTION("Test: read one reg..")
	{
		ModbusRTU::ReadOutputRetMessage ret = mb->read03(slaveaddr,tREG,1);
		REQUIRE( ret.data[0] == 10 );
	}
	SECTION("Test: read many registers..")
	{
		ModbusRTU::ReadOutputRetMessage ret = mb->read03(slaveaddr,tREG,3);
		REQUIRE( ret.data[0] == 10 );
		REQUIRE( ret.data[1] == 11 );
		REQUIRE( (signed short)(ret.data[2]) == -10 );
	}
	SECTION("Test: read MAXDATA count..")
	{
		ModbusRTU::ReadOutputRetMessage ret = mb->read03(slaveaddr,tREG,ModbusRTU::MAXDATALEN);
		REQUIRE( ret.count == ModbusRTU::MAXDATALEN );
	}
	SECTION("Test: read TOO many registers")
	{
		try
		{
			mb->read03(slaveaddr,-23,1200);
		}
		catch( ModbusRTU::mbException& ex )
		{
			REQUIRE( ex.err == ModbusRTU::erTimeOut );
		}
	}
	SECTION("Test: read unknown registers")
	{
		try
		{
			mb->read03(slaveaddr,-23,1);
		}
		catch( ModbusRTU::mbException& ex )
		{
			REQUIRE( ex.err == ModbusRTU::erBadDataAddress );
		}
	}
	SECTION("Test: incorrect number")
	{
		try
		{
			mb->read03(slaveaddr,tREG,-3);
		}
		catch( ModbusRTU::mbException& ex )
		{
			REQUIRE( ex.err == ModbusRTU::erTimeOut );
		}
	}
	SECTION("Test: zero number")
	{
		try
		{
			mb->read03(slaveaddr,tREG,0);
		}
		catch( ModbusRTU::mbException& ex )
		{
			REQUIRE( ex.err == ModbusRTU::erTimeOut );
		}
	}
}

TEST_CASE("Function (0x04): 'read input registers or memories or read word outputs or memories'","[modbus][mbslave][mbtcpslave]")
{
	InitTest();
	ModbusRTU::ModbusData tREG=10;
	SECTION("Test: read one reg..")
	{
		ModbusRTU::ReadInputRetMessage ret = mb->read04(slaveaddr,tREG,1);
		REQUIRE( ret.data[0] == 10 );
	}
	SECTION("Test: read one reg..")
	{
		ModbusRTU::ReadInputRetMessage ret = mb->read04(slaveaddr,tREG,1);
		REQUIRE( ret.data[0] == 10 );
	}
	SECTION("Test: read many registers..")
	{
		ModbusRTU::ReadInputRetMessage ret = mb->read04(slaveaddr,tREG,4);
		REQUIRE( ret.data[0] == 10 );
		REQUIRE( ret.data[1] == 11 );
		REQUIRE( (signed short)(ret.data[2]) == -10 );
		REQUIRE( (signed short)(ret.data[3]) == -10000 );
	}
	SECTION("Test: read MAXDATA count..")
	{
		ModbusRTU::ReadInputRetMessage ret = mb->read04(slaveaddr,tREG,ModbusRTU::MAXDATALEN);
		REQUIRE( ret.count == ModbusRTU::MAXDATALEN );
	}
	SECTION("Test: read TOO many registers")
	{
		try
		{
			mb->read04(slaveaddr,-23,1200);
		}
		catch( ModbusRTU::mbException& ex )
		{
			REQUIRE( ex.err == ModbusRTU::erTimeOut );
		}
	}
	SECTION("Test: read unknown registers")
	{
		try
		{
			mb->read04(slaveaddr,-23,1);
		}
		catch( ModbusRTU::mbException& ex )
		{
			REQUIRE( ex.err == ModbusRTU::erBadDataAddress );
		}
	}
	SECTION("Test: incorrect number")
	{
		try
		{
			mb->read04(slaveaddr,tREG,-3);
		}
		catch( ModbusRTU::mbException& ex )
		{
			REQUIRE( ex.err == ModbusRTU::erTimeOut );
		}
	}
	SECTION("Test: zero number")
	{
		try
		{
			mb->read04(slaveaddr,tREG,0);
		}
		catch( ModbusRTU::mbException& ex )
		{
			REQUIRE( ex.err == ModbusRTU::erTimeOut );
		}
	}
}

TEST_CASE("(0x05): forces a single coil to either ON or OFF","[modbus][mbslave][mbtcpslave]")
{
	InitTest();
	ObjectId tID = 1007;
	ModbusRTU::ModbusData tREG=14;
	SECTION("Test: ON")
	{
		ModbusRTU::ForceSingleCoilRetMessage  ret = mb->write05(slaveaddr,tREG,true);
		CHECK( ret.start == tREG );
		CHECK( ret.cmd() == true );
		CHECK( ui->getValue(tID) == 1 );
	}
	SECTION("Test: OFF")
	{
		ModbusRTU::ForceSingleCoilRetMessage  ret = mb->write05(slaveaddr,tREG,false);
		CHECK( ret.start == tREG );
		CHECK( ret.cmd() == false );
		CHECK( ui->getValue(tID) == 0 );
	}
}

TEST_CASE("(0x06): write register outputs or memories","[modbus][mbslave][mbtcpslave]")
{
	InitTest();
	ObjectId tID = 1008;
	ModbusRTU::ModbusData tREG=15;
	SECTION("Test: write register")
	{
		ModbusRTU::WriteSingleOutputRetMessage  ret = mb->write06(slaveaddr,tREG,10);
		REQUIRE( ret.start == tREG );
		REQUIRE( ret.data == 10 );
		REQUIRE( ui->getValue(tID) == 10 );
	}
	SECTION("Test: write negative value")
	{
		ModbusRTU::WriteSingleOutputRetMessage  ret = mb->write06(slaveaddr,tREG,-10);
		REQUIRE( ret.start == tREG );
		REQUIRE( (signed short)ret.data == -10 );
		REQUIRE( (signed short)ui->getValue(tID) == -10 );
	}
	SECTION("Test: write zero value")
	{
		ModbusRTU::WriteSingleOutputRetMessage  ret = mb->write06(slaveaddr,tREG,0);
		REQUIRE( ret.start == tREG );
		REQUIRE( ret.data == 0 );
		REQUIRE( ui->getValue(tID) == 0 );
	}
	SECTION("Test: write OVERFLOW VALUE")
	{
		WARN("FIXME: what to do in this situation?!");
#if 0
		ModbusRTU::WriteSingleOutputRetMessage  ret = mb->write06(slaveaddr,15,100000);
		REQUIRE( ret.start == 15 );
		REQUIRE( ret.data == 34464 );
		REQUIRE( ui->getValue(1008) == 34464 );
#endif
	}
	SECTION("Test: write unknown register")
	{
		try
		{
			mb->write06(slaveaddr,-23,10);
		}
		catch( ModbusRTU::mbException& ex )
		{
			REQUIRE( ex.err == ModbusRTU::erBadDataAddress );
		}
	}
}
#if 0
	SECTION("(0x08): Diagnostics (Serial Line only)")
	{

	}
#endif

#if 0
	\TODO  Переписать реализацию MBSlave... ввести понятие nbit.
TEST_CASE("(0x0F): force multiple coils","[modbus][mbslave][mbtcpslave]")
{
	WARN("FIXME: 'force coil status'. Use 'nbit'?"):
	InitTest();
	ObjectId tID = 1009;
	ModbusRTU::ModbusData tREG=16;
	SECTION("Test: write 2 bit to 1")
	{
		ModbusRTU::ForceCoilsMessage msg(slaveaddr,tREG);
        ModbusRTU::DataBits b(3);
        msg.addData(b);
		ModbusRTU::ForceCoilsRetMessage ret = mb->write0F(msg);
		REQUIRE( ret.start == tREG );
		REQUIRE( ret.quant == 8 );
		REQUIRE( ui->getValue(tID) == 1 );
		REQUIRE( ui->getValue(tID+1) == 1 );
	}
	SECTION("Test: write 2 bit to 0")
	{
		ModbusRTU::ForceCoilsMessage msg(slaveaddr,tREG);
        ModbusRTU::DataBits b(0);
        msg.addData(b);
		ModbusRTU::ForceCoilsRetMessage ret = mb->write0F(msg);
		REQUIRE( ret.start == tREG );
		REQUIRE( ret.quant == 8 );
		REQUIRE( ui->getValue(tID) == 0 );
		REQUIRE( ui->getValue(tID+1) == 0 );
	}
}
#endif
TEST_CASE("(0x10): write register outputs or memories","[modbus][mbslave][mbtcpslave]")
{
	InitTest();
	InitTest();
	ObjectId tID = 1025;
	ModbusRTU::ModbusData tREG=18;
	SECTION("Test: write one register")
	{
		ModbusRTU::WriteOutputMessage msg(slaveaddr,tREG);
		msg.addData(10);
		ModbusRTU::WriteOutputRetMessage ret = mb->write10(msg);
		REQUIRE( ret.start == tREG );
		REQUIRE( ret.quant == 1 );
		REQUIRE( ui->getValue(tID) == 10 );
	}
	SECTION("Test: write 3 register")
	{
		ModbusRTU::WriteOutputMessage msg(slaveaddr,tREG);
		msg.addData(10);
		msg.addData(11);
		msg.addData(12);
		ModbusRTU::WriteOutputRetMessage ret = mb->write10(msg);
		REQUIRE( ret.start == tREG );
		REQUIRE( ret.quant == 3 );
		REQUIRE( ui->getValue(tID) == 10 );
		REQUIRE( ui->getValue(tID+1) == 11 );
		REQUIRE( ui->getValue(tID+2) == 1 );  // 1 - т.к. это "DI"
	}
	SECTION("Test: write negative value")
	{
		ModbusRTU::WriteOutputMessage msg(slaveaddr,tREG);
		msg.addData(-10);
		msg.addData(-100);
		ModbusRTU::WriteOutputRetMessage ret = mb->write10(msg);
		REQUIRE( ret.start == tREG );
		REQUIRE( ret.quant == 2 );
		REQUIRE( (signed short)ui->getValue(tID) == -10 );
		REQUIRE( (signed short)ui->getValue(tID+1) == -100 );
	}
	SECTION("Test: write zero registers")
	{
		ModbusRTU::WriteOutputMessage msg(slaveaddr,tREG);
		msg.addData(0);
		msg.addData(0);
		msg.addData(0);
		ModbusRTU::WriteOutputRetMessage ret = mb->write10(msg);
		REQUIRE( ret.start == tREG );
		REQUIRE( ret.quant == 3 );
		REQUIRE( ui->getValue(tID) == 0 );
		REQUIRE( ui->getValue(tID+1) == 0 );
		REQUIRE( ui->getValue(tID+2) == 0 );
	}
	SECTION("Test: write OVERFLOW VALUE")
	{
		WARN("FIXME: what to do in this situation?!");
#if 0
		ModbusRTU::WriteSingleOutputRetMessage  ret = mb->write06(slaveaddr,15,100000);
		REQUIRE( ret.start == 15 );
		REQUIRE( ret.data == 34464 );
		REQUIRE( ui->getValue(1008) == 34464 );
#endif
	}
	SECTION("Test: write 2 good registers and unknown register")
	{
		ModbusRTU::WriteOutputMessage msg(slaveaddr,tREG+1);
		msg.addData(10);
		msg.addData(11);
		msg.addData(12); // BAD REG..
		ModbusRTU::WriteOutputRetMessage ret = mb->write10(msg);
		REQUIRE( ret.start == tREG+1 );
		WARN("FIXME: 'ret.quant' must be '3' or '2'?!");
		REQUIRE( ret.quant == 3 ); // "2" ?!! \TODO узнать как нужно поступать по стандарту!
		REQUIRE( ui->getValue(tID+1) == 10 );
		REQUIRE( ui->getValue(tID+2) == 1 );   // 1 - т.к. это "DI"
		REQUIRE( ui->getValue(tID+3) == 0 );
	}
	SECTION("Test: write ALL unknown registers")
	{
		ModbusRTU::WriteOutputMessage msg(slaveaddr,tREG+20000);
		msg.addData(10);
		msg.addData(11);
		msg.addData(12);
		try
		{
			mb->write10(msg);
		}
		catch( ModbusRTU::mbException& ex )
		{
			REQUIRE( ex.err == ModbusRTU::erBadDataAddress );
		}
	}
}

TEST_CASE("Read(0x03,0x04): vtypes..","[modbus][mbslave][mbtcpslave]")
{
	using namespace VTypes;
	InitTest();

	SECTION("Test: read vtype 'I2'")
	{
		ModbusRTU::ModbusData tREG = 100;
		SECTION("Test: read03")
		{
			ModbusRTU::ReadOutputRetMessage ret = mb->read03(slaveaddr,tREG,I2::wsize());
			I2 i2(ret.data,ret.count);
			REQUIRE( (int)i2 == -100000 );
		}
		SECTION("Test: read04")
		{
			ModbusRTU::ReadInputRetMessage ret = mb->read04(slaveaddr,tREG,I2::wsize());
			I2 i2(ret.data,ret.count);
			REQUIRE( (int)i2 == -100000 );
		}
	}
	SECTION("Test: read vtype 'I2r'")
	{
		ModbusRTU::ModbusData tREG = 102;
		SECTION("Test: read03")
		{
			ModbusRTU::ReadOutputRetMessage ret = mb->read03(slaveaddr,tREG,I2r::wsize());
			I2r i2r(ret.data,ret.count);
			REQUIRE( (int)i2r == -100000 );
		}
		SECTION("Test: read04")
		{
			ModbusRTU::ReadInputRetMessage ret = mb->read04(slaveaddr,tREG,I2r::wsize());
			I2r i2r(ret.data,ret.count);
			REQUIRE( (int)i2r == -100000 );
		}
	}
	SECTION("Test: read vtype 'U2'")
	{
		ModbusRTU::ModbusData tREG = 104;
		SECTION("Test: read03")
		{
			ModbusRTU::ReadOutputRetMessage ret = mb->read03(slaveaddr,tREG,U2::wsize());
			U2 u2(ret.data,ret.count);
			REQUIRE( (unsigned int)u2 == 4294967295 );
		}
		SECTION("Test: read04")
		{
			ModbusRTU::ReadInputRetMessage ret = mb->read04(slaveaddr,tREG,U2::wsize());
			U2 u2(ret.data,ret.count);
			REQUIRE( (unsigned int)u2 == 4294967295 );
		}
	}
	SECTION("Test: read vtype 'U2r'")
	{
		ModbusRTU::ModbusData tREG = 106;
		SECTION("Test: read03")
		{
			ModbusRTU::ReadOutputRetMessage ret = mb->read03(slaveaddr,tREG,U2r::wsize());
			U2r u2r(ret.data,ret.count);
			REQUIRE( (unsigned int)u2r == 4294967295 );
		}
		SECTION("Test: read04")
		{
			ModbusRTU::ReadInputRetMessage ret = mb->read04(slaveaddr,tREG,U2r::wsize());
			U2r u2r(ret.data,ret.count);
			REQUIRE( (unsigned int)u2r == 4294967295 );
		}
	}
	SECTION("Test: read vtype 'F2'")
	{
		ModbusRTU::ModbusData tREG = 110;
		SECTION("Test: read03")
		{
			ModbusRTU::ReadOutputRetMessage ret = mb->read03(slaveaddr,tREG,F2::wsize());
			F2 f2(ret.data,ret.count);
			REQUIRE( (float)f2 == 2.5 );
		}
		SECTION("Test: read04")
		{
			ModbusRTU::ReadInputRetMessage ret = mb->read04(slaveaddr,tREG,F2::wsize());
			F2 f2(ret.data,ret.count);
			REQUIRE( (float)f2 == 2.5 );
		}
	}
	SECTION("Test: read vtype 'F2r'")
	{
		ModbusRTU::ModbusData tREG = 112;
		SECTION("Test: read03")
		{
			ModbusRTU::ReadOutputRetMessage ret = mb->read03(slaveaddr,tREG,F2r::wsize());
			F2r f2r(ret.data,ret.count);
			REQUIRE( (float)f2r == 2.5 );
		}
		SECTION("Test: read04")
		{
			ModbusRTU::ReadInputRetMessage ret = mb->read04(slaveaddr,tREG,F2r::wsize());
			F2r f2r(ret.data,ret.count);
			REQUIRE( (float)f2r == 2.5 );
		}
	}
	SECTION("Test: read vtype 'F4'")
	{
		ModbusRTU::ModbusData tREG = 114;
		SECTION("Test: read03")
		{
			ModbusRTU::ReadOutputRetMessage ret = mb->read03(slaveaddr,tREG,F4::wsize());
			F4 f4(ret.data,ret.count);
			REQUIRE( (float)f4 == 2.5 );
		}
		SECTION("Test: read04")
		{
			ModbusRTU::ReadInputRetMessage ret = mb->read04(slaveaddr,tREG,F4::wsize());
			F4 f4(ret.data,ret.count);
			REQUIRE( (float)f4 == 2.5 );
		}
	}
}

TEST_CASE("Write(0x10): vtypes..","[modbus][mbslave][mbtcpslave]")
{
	InitTest();
	FAIL("Tests for '0x10 and vtypes' not yet..");
}

#if 0
TEST_CASE("(0x14): read file record","[modbus][mbslave][mbtcpslave]")
{
	
}
TEST_CASE("(0x15): write file record","[modbus][mbslave][mbtcpslave]")
{
	
}
TEST_CASE("(0x2B): Modbus Encapsulated Interface","[modbus][mbslave][mbtcpslave]")
{
	
}
TEST_CASE("(0x50): set date and time")
{
	
}
TEST_CASE("(0x53): call remote service")
{
	
}
TEST_CASE("(0x65): read,write,delete alarm journal")
{
	
}
TEST_CASE("(0x66): file transfer")
{
	
}
#endif
