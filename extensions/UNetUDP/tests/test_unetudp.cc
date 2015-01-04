#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <cc++/socket.h>
#include "UniSetTypes.h"
#include "UInterface.h"
#include "UDPPacket.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
// -----------------------------------------------------------------------------
static int port = 3000;
static ost::IPV4Host host("127.255.255.255");
static UInterface* ui = nullptr;
static ObjectId aid = 2;
static ost::UDPDuplex* udp_r = nullptr;
// -----------------------------------------------------------------------------
void InitTest()
{
    auto conf = uniset_conf();
    CHECK( conf!=nullptr );

    if( ui == nullptr )
    {
        ui = new UInterface();
        // UI понадобиться для проверки записанных в SM значений.
        CHECK( ui->getObjectIndex() != nullptr );
        CHECK( ui->getConf() == conf );
        CHECK( ui->waitReady(aid,10000) );
    }

	if( udp_r == nullptr )
		udp_r = new ost::UDPDuplex(host,port);
}
// -----------------------------------------------------------------------------
static UniSetUDP::UDPMessage receive( unsigned int pnum = 0, timeout_t tout = 2000, int ncycle = 1000 )
{
	UniSetUDP::UDPMessage pack;
	UniSetUDP::UDPPacket buf;

	while( ncycle > 0 )
	{
		if( !udp_r->isInputReady(tout) )
			break;

		size_t ret = udp_r->UDPReceive::receive( &(buf.data), sizeof(buf.data) );
		size_t sz = UniSetUDP::UDPMessage::getMessage(pack,buf);
		if( pnum ==0 || ( pnum > 0 && pack.num >= pnum ) )
			break;

		ncycle--;
	}

	return std::move(pack);
}
// -----------------------------------------------------------------------------
TEST_CASE("[UNetUDP]: respond sensor","[unetudp]")
{
    InitTest();

	// в запускающем файле стоит --unet-recv-timeout 2000
	msleep(2500);
	ObjectId node1_not_respond_s = 1;
	REQUIRE( ui->getValue(node1_not_respond_s) == 1 );
}
// -----------------------------------------------------------------------------
TEST_CASE("[UNetUDP]: check sender","[unetudp][sender]")
{
    InitTest();

    SECTION("Test: read default pack...")
    {
		UniSetUDP::UDPMessage pack = receive();
		REQUIRE( pack.num!=0 );
		REQUIRE( pack.asize()==4 );
		REQUIRE( pack.dsize()==2 );

		for( int i=0; i<pack.asize(); i++ )
		{
			REQUIRE( pack.a_dat[i].val == i+1 );
		}

		REQUIRE( pack.dValue(0) == 1 );
		REQUIRE( pack.dValue(1) == 0 );

		// т.к. данные в SM не менялись, то должен придти пакет с тем же номером что и был..
		UniSetUDP::UDPMessage pack2 = receive();
		REQUIRE( pack2.num == pack.num );
	}

    SECTION("Test: change data...")
    {
		UniSetUDP::UDPMessage pack0 = receive();
		ui->setValue(2,100);
		REQUIRE( ui->getValue(2) == 100 );
		msleep(120);
		UniSetUDP::UDPMessage pack = receive( pack0.num+1 );
		REQUIRE( pack.num!=0 );
		REQUIRE( pack.asize()==4 );
		REQUIRE( pack.dsize()==2 );
		REQUIRE( pack.a_dat[0].val == 100 );

		ui->setValue(2,250);
		REQUIRE( ui->getValue(2) == 250 );
		msleep(120);
		UniSetUDP::UDPMessage pack2 = receive( pack.num+1 );
		REQUIRE( pack2.num!=0 );
		REQUIRE( pack2.num > pack.num );
		REQUIRE( pack2.asize()==4 );
		REQUIRE( pack2.dsize()==2 );
		REQUIRE( pack2.a_dat[0].val == 250 );
	}
}
// -----------------------------------------------------------------------------
#if 0
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
/*! \TODO  Переписать реализацию MBSlave... ввести понятие nbit. */
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
    SECTION("Test: write bad format packet..(incorrect data count)")
    {
        ModbusRTU::WriteOutputMessage msg(slaveaddr,tREG+20000);
        msg.addData(10);
        msg.addData(11);
        msg.addData(12);
        msg.quant-=1;

        try
        {
            mb->write10(msg);
        }
        catch( ModbusRTU::mbException& ex )
        {
            REQUIRE( ex.err == ModbusRTU::erBadDataAddress );
        }
    }
    SECTION("Test: write bad format packet..(incorrect size of bytes)")
    {
        ModbusRTU::WriteOutputMessage msg(slaveaddr,tREG+20000);
        msg.addData(10);
        msg.addData(11);
        msg.addData(12);
        msg.bcnt -= 1;

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
    SECTION("Test: read vtype 'Byte N1'")
    {
        ModbusRTU::ModbusData tREG = 108;
        SECTION("Test: read03")
        {
            ModbusRTU::ReadOutputRetMessage ret = mb->read03(slaveaddr,tREG,Byte::wsize());
            Byte b(ret.data[0]);
            REQUIRE(  (unsigned short)b == 200 );
        }
        SECTION("Test: read04")
        {
            ModbusRTU::ReadInputRetMessage ret = mb->read04(slaveaddr,tREG,Byte::wsize());
            Byte b(ret.data[0]);
            REQUIRE(  (unsigned short)b == 200 );
        }
    }
    SECTION("Test: read vtype 'Byte N2'")
    {
        ModbusRTU::ModbusData tREG = 109;
        SECTION("Test: read03")
        {
            ModbusRTU::ReadOutputRetMessage ret = mb->read03(slaveaddr,tREG,Byte::wsize());
            Byte b(ret.data[0]);
            REQUIRE(  (unsigned short)b == 200 );
        }
        SECTION("Test: read04")
        {
            ModbusRTU::ReadInputRetMessage ret = mb->read04(slaveaddr,tREG,Byte::wsize());
            Byte b(ret.data[0]);
            REQUIRE(  (unsigned short)b == 200 );
        }
    }
}

// -------------------------------------------------------------
static void test_write10_I2( int val )
{
    using namespace VTypes;
    ModbusRTU::ModbusData tREG = 100;
    ModbusRTU::WriteOutputMessage msg(slaveaddr,tREG);
    I2 tmp(val);
    msg.addData( tmp.raw.v[0] );
    msg.addData( tmp.raw.v[1] );
    ModbusRTU::WriteOutputRetMessage  ret = mb->write10(msg);
    REQUIRE( ret.start == tREG );
    REQUIRE( ret.quant == I2::wsize() );
    REQUIRE( ui->getValue(2001) == val );
}
static void test_write10_I2r( int val )
{
    using namespace VTypes;
    ModbusRTU::ModbusData tREG = 102;
    ModbusRTU::WriteOutputMessage msg(slaveaddr,tREG);
    I2r tmp(val);
    msg.addData( tmp.raw_backorder.v[0] );
    msg.addData( tmp.raw_backorder.v[1] );
    ModbusRTU::WriteOutputRetMessage  ret = mb->write10(msg);
    REQUIRE( ret.start == tREG );
    REQUIRE( ret.quant == I2r::wsize() );
    REQUIRE( ui->getValue(2002) == val );
}
static void test_write10_U2( unsigned int val )
{
    using namespace VTypes;
    ModbusRTU::ModbusData tREG = 104;
    ModbusRTU::WriteOutputMessage msg(slaveaddr,tREG);
    U2 tmp(val);
    msg.addData( tmp.raw.v[0] );
    msg.addData( tmp.raw.v[1] );
    ModbusRTU::WriteOutputRetMessage  ret = mb->write10(msg);
    REQUIRE( ret.start == tREG );
    REQUIRE( ret.quant == U2::wsize() );
    REQUIRE( (unsigned int)ui->getValue(2003) == val );
}
static void test_write10_U2r( unsigned int val )
{
    using namespace VTypes;
    ModbusRTU::ModbusData tREG = 106;
    ModbusRTU::WriteOutputMessage msg(slaveaddr,tREG);
    U2r tmp(val);
    msg.addData( tmp.raw_backorder.v[0] );
    msg.addData( tmp.raw_backorder.v[1] );
    ModbusRTU::WriteOutputRetMessage  ret = mb->write10(msg);
    REQUIRE( ret.start == tREG );
    REQUIRE( ret.quant == U2r::wsize() );
    REQUIRE( (unsigned int)ui->getValue(2004) == val );
}
static void test_write10_F2( const float& val )
{
    using namespace VTypes;
    ModbusRTU::ModbusData tREG = 110;
    ModbusRTU::WriteOutputMessage msg(slaveaddr,tREG);
    F2 tmp(val);
    msg.addData( tmp.raw.v[0] );
    msg.addData( tmp.raw.v[1] );
    ModbusRTU::WriteOutputRetMessage  ret = mb->write10(msg);
    REQUIRE( ret.start == tREG );
    REQUIRE( ret.quant == F2::wsize() );

    auto conf = uniset_conf();
    IOController_i::SensorInfo si;
    si.id = 2007;
    si.node = conf->getLocalNode();
    IOController_i::CalibrateInfo cal = ui->getCalibrateInfo(si);
    float fval = (float)ui->getValue(si.id) / pow10(cal.precision);

    REQUIRE( fval == val );
}
static void test_write10_F2r( const float& val )
{
    using namespace VTypes;
    ModbusRTU::ModbusData tREG = 112;
    ModbusRTU::WriteOutputMessage msg(slaveaddr,tREG);
    F2r tmp(val);
    msg.addData( tmp.raw_backorder.v[0] );
    msg.addData( tmp.raw_backorder.v[1] );
    ModbusRTU::WriteOutputRetMessage  ret = mb->write10(msg);
    REQUIRE( ret.start == tREG );
    REQUIRE( ret.quant == F2r::wsize() );

    auto conf = uniset_conf();
    IOController_i::SensorInfo si;
    si.id = 2008;
    si.node = conf->getLocalNode();
    IOController_i::CalibrateInfo cal = ui->getCalibrateInfo(si);
    float fval = (float)ui->getValue(si.id) / pow10(cal.precision);

    REQUIRE( fval == val );
}

static void test_write10_F4raw( const float& val )
{
    using namespace VTypes;
    ModbusRTU::ModbusData tREG = 120;
    ModbusRTU::WriteOutputMessage msg(slaveaddr,tREG);
    F4 tmp(val);
    msg.addData( tmp.raw.v[0] );
    msg.addData( tmp.raw.v[1] );
    msg.addData( tmp.raw.v[2] );
    msg.addData( tmp.raw.v[3] );
    ModbusRTU::WriteOutputRetMessage  ret = mb->write10(msg);
    REQUIRE( ret.start == tREG );
    REQUIRE( ret.quant == F4::wsize() );

    auto conf = uniset_conf();
    IOController_i::SensorInfo si;
    si.id = 2013;
    si.node = conf->getLocalNode();

    long raw = ui->getValue(si.id);
    float fval = 0;
    memcpy( &fval,&raw,std::min(sizeof(fval),sizeof(raw)) );
    REQUIRE( fval == val );
}

static void test_write10_F4prec( const float& val )
{
    using namespace VTypes;
    ModbusRTU::ModbusData tREG = 114;
    ModbusRTU::WriteOutputMessage msg(slaveaddr,tREG);
    F4 tmp(val);
    msg.addData( tmp.raw.v[0] );
    msg.addData( tmp.raw.v[1] );
    msg.addData( tmp.raw.v[2] );
    msg.addData( tmp.raw.v[3] );
    ModbusRTU::WriteOutputRetMessage  ret = mb->write10(msg);
    REQUIRE( ret.start == tREG );
    REQUIRE( ret.quant == F4::wsize() );

    auto conf = uniset_conf();
    IOController_i::SensorInfo si;
    si.id = 2009;
    si.node = conf->getLocalNode();
    IOController_i::CalibrateInfo cal = ui->getCalibrateInfo(si);
    float fval = (float)ui->getValue(si.id) / pow10(cal.precision);

    REQUIRE( fval == val );
}
static void test_write10_byte1( unsigned char val )
{
    using namespace VTypes;
    ModbusRTU::ModbusData tREG = 108;
    ModbusRTU::WriteOutputMessage msg(slaveaddr,tREG);
    Byte tmp(val,0);
    msg.addData( (unsigned short)tmp );

    ModbusRTU::WriteOutputRetMessage  ret = mb->write10(msg);
    REQUIRE( ret.start == tREG );
    REQUIRE( ret.quant == Byte::wsize() );
    REQUIRE( ui->getValue(2005) == (long)val );
}
static void test_write10_byte2( unsigned char val )
{
    using namespace VTypes;
    ModbusRTU::ModbusData tREG = 109;
    ModbusRTU::WriteOutputMessage msg(slaveaddr,tREG);
    Byte tmp(0,val);
    msg.addData( (unsigned short)tmp );

    ModbusRTU::WriteOutputRetMessage  ret = mb->write10(msg);
    REQUIRE( ret.start == tREG );
    REQUIRE( ret.quant == Byte::wsize() );
    REQUIRE( ui->getValue(2006) == (long)val );
}

TEST_CASE("Write(0x10): vtypes..","[modbus][mbslave][mbtcpslave]")
{
    using namespace VTypes;
    InitTest();

    SECTION("Test: write vtype 'I2'")
    {
        test_write10_I2(numeric_limits<int>::max());
        test_write10_I2(0);
        test_write10_I2(numeric_limits<int>::min());
    }
    SECTION("Test: write vtype 'I2r'")
    {
        test_write10_I2r(numeric_limits<int>::max());
        test_write10_I2r(0);
        test_write10_I2r(numeric_limits<int>::min());
    }
    SECTION("Test: write vtype 'U2'")
    {
        test_write10_U2(numeric_limits<unsigned int>::max());
        test_write10_U2(0);
        test_write10_U2(numeric_limits<unsigned int>::min());
    }
    SECTION("Test: write vtype 'U2r'")
    {
        test_write10_U2r(numeric_limits<unsigned int>::max());
        test_write10_U2r(0);
        test_write10_U2r(numeric_limits<unsigned int>::min());
    }
    SECTION("Test: write vtype 'F2'")
    {
        test_write10_F2(-0.05);
        test_write10_F2(0);
        test_write10_F2(100000.23);
    }
    SECTION("Test: write vtype 'F2r'")
    {
        test_write10_F2r(-0.05);
        test_write10_F2r(0);
        test_write10_F2r(100000.23);
    }
    SECTION("Test: write vtype 'F4'(raw)")
    {
        test_write10_F4raw(numeric_limits<float>::max());
        test_write10_F4raw(0);
        test_write10_F4raw(numeric_limits<float>::min());
    }
    SECTION("Test: write vtype 'F4'(precision)")
    {
        test_write10_F4prec(15.55555);
        test_write10_F4prec(0);
        test_write10_F4prec(-15.00001);
    }
    SECTION("Test: write vtype 'Byte N1'")
    {
        test_write10_byte1(numeric_limits<unsigned char>::max());
        test_write10_byte1(0);
        test_write10_byte1(numeric_limits<unsigned char>::min());
        test_write10_byte1(numeric_limits<char>::max());
        test_write10_byte1(numeric_limits<char>::min());
    }
    SECTION("Test: write vtype 'Byte N2'")
    {
        test_write10_byte2(numeric_limits<unsigned char>::max());
        test_write10_byte2(0);
        test_write10_byte2(numeric_limits<unsigned char>::min());
        test_write10_byte2(numeric_limits<char>::max());
        test_write10_byte2(numeric_limits<char>::min());
    }
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

TEST_CASE("access mode","[modbus][mbslvae][mbtcpslave]")
{
    SECTION("test 'RO' register")
    {
        ModbusRTU::ModbusData tREG=124;

        // read
        ModbusRTU::ReadInputRetMessage ret = mb->read04(slaveaddr,tREG,1);
        REQUIRE( ret.data[0] == 1002 );

        // write
        try
        {
            ModbusRTU::WriteOutputMessage msg(slaveaddr,tREG);
            msg.addData(33);
            mb->write10(msg);
        }
        catch( ModbusRTU::mbException& ex )
        {
            REQUIRE( ex.err == ModbusRTU::erBadDataAddress );
        }
    }
    SECTION("test 'WO' register")
    {
        ModbusRTU::ModbusData tREG=125;

        // read
        try
        {
            mb->read04(slaveaddr,tREG,1);
        }
        catch( ModbusRTU::mbException& ex )
        {
            REQUIRE( ex.err == ModbusRTU::erBadDataAddress );
        }

        // write
        ModbusRTU::WriteOutputMessage msg(slaveaddr,tREG);
        msg.addData(555);
        ModbusRTU::WriteOutputRetMessage ret = mb->write10(msg);
        REQUIRE( ret.start == tREG );
        REQUIRE( ret.quant == 1 );
        REQUIRE( ui->getValue(2015) == 555 );
    }
    SECTION("test 'RW' register")
    {
        ModbusRTU::ModbusData tREG=126;

        // write
        ModbusRTU::WriteOutputMessage msg(slaveaddr,tREG);
        msg.addData(555);
        ModbusRTU::WriteOutputRetMessage ret = mb->write10(msg);
        REQUIRE( ret.start == tREG );
        REQUIRE( ret.quant == 1 );
        REQUIRE( ui->getValue(2016) == 555 );

        // read
        ModbusRTU::ReadInputRetMessage rret = mb->read04(slaveaddr,tREG,1);
        REQUIRE( rret.data[0] == 555 );
    }
}
#endif