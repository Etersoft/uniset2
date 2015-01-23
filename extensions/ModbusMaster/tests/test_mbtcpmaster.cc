#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <time.h>
#include <memory>
#include <limits>
#include "UniSetTypes.h"
#include "MBTCPTestServer.h"
#include "MBTCPMultiMaster.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
// -----------------------------------------------------------------------------
static ModbusRTU::ModbusAddr slaveaddr = 0x01; // conf->getArgInt("--mbs-my-addr");
static int port = 20048; // conf->getArgInt("--mbs-inet-port");
static string addr("127.0.0.1"); // conf->getArgParam("--mbs-inet-addr");
static ModbusRTU::ModbusAddr slaveADDR = 0x01;
static shared_ptr<MBTCPTestServer> mbs;
static shared_ptr<UInterface> ui;
static ObjectId mbID = 6004; // MBTCPMaster1
static int polltime=100; // conf->getArgInt("--mbtcp-polltime");
static ObjectId slaveNotRespond = 10; // Slave_Not_Respond_S
static const ObjectId exchangeMode = 11; // MBTCPMaster_Mode_AS
// -----------------------------------------------------------------------------
static void InitTest()
{
    auto conf = uniset_conf();
    CHECK( conf!=nullptr );

    if( !ui )
    {
        ui = make_shared<UInterface>();
        // UI понадобиться для проверки записанных в SM значений.
        CHECK( ui->getObjectIndex() != nullptr );
        CHECK( ui->getConf() == conf );
        CHECK( ui->waitReady(slaveNotRespond,8000) );
    }

    if( !mbs )
    {
        mbs = make_shared<MBTCPTestServer>(slaveADDR,addr,port,false);
        CHECK( mbs!= nullptr );
        mbs->runThread();
        for( int i=0; !mbs->isRunning() && i<10; i++ )
            msleep(200);
        CHECK( mbs->isRunning() );
        msleep(2000);
        CHECK( ui->getValue(slaveNotRespond) == 0 );
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: 0x01 (read coil status)","[modbus][0x01][mbmaster][mbtcpmaster]")
{
    InitTest();

    CHECK( ui->isExist(mbID) );
    mbs->setReply(65535);
    msleep(polltime+100);
    REQUIRE( ui->getValue(1000) == 1 );
    REQUIRE( ui->getValue(1001) == 1 );
    REQUIRE( ui->getValue(1002) == 1 );

    mbs->setReply(0);
    msleep(polltime+100);
    REQUIRE( ui->getValue(1000) == 0 );
    REQUIRE( ui->getValue(1001) == 0 );
    REQUIRE( ui->getValue(1002) == 0 );
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: 0x02 (read input status)","[modbus][0x02][mbmaster][mbtcpmaster]")
{
    InitTest();

    CHECK( ui->isExist(mbID) );
    mbs->setReply(65535);
    msleep(polltime+100);
    REQUIRE( ui->getValue(1040) == 1 );
    REQUIRE( ui->getValue(1041) == 1 );
    REQUIRE( ui->getValue(1042) == 1 );

    mbs->setReply(0);
    msleep(polltime+100);
    REQUIRE( ui->getValue(1040) == 0 );
    REQUIRE( ui->getValue(1041) == 0 );
    REQUIRE( ui->getValue(1042) == 0 );
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: 0x03 (read register outputs or memories or read word outputs or memories)","[modbus][0x03][mbmaster][mbtcpmaster]")
{
    InitTest();

    CHECK( ui->isExist(mbID) );
    mbs->setReply(10);
    msleep(polltime+100);
    REQUIRE( ui->getValue(1003) == 10 );
    REQUIRE( ui->getValue(1004) == 10 );
    REQUIRE( ui->getValue(1005) == 10 );
    REQUIRE( ui->getValue(1006) == 10 );
    mbs->setReply(-10);
    msleep(polltime+100);
    REQUIRE( ui->getValue(1003) == -10 );
    REQUIRE( ui->getValue(1004) == -10 );
    REQUIRE( ui->getValue(1005) == -10 );
    REQUIRE( ui->getValue(1006) == -10 );
    mbs->setReply(0);
    msleep(polltime+100);
    REQUIRE( ui->getValue(1003) == 0 );
    REQUIRE( ui->getValue(1004) == 0 );
    REQUIRE( ui->getValue(1005) == 0 );
    REQUIRE( ui->getValue(1006) == 0 );
    mbs->setReply(65535);
    msleep(polltime+100);
    REQUIRE( ui->getValue(1003) == -1 );
    REQUIRE( ui->getValue(1004) == -1 );
    REQUIRE( ui->getValue(1005) == -1 );
    REQUIRE( ui->getValue(1006) == -1 );
    REQUIRE( ui->getValue(1007) == 65535 ); // unsigned

    mbs->setReply(0xffff);
    msleep(polltime+100);
    REQUIRE( ui->getValue(1008) == 0xffffffff ); // I2
    REQUIRE( ui->getValue(1009) == 0xffffffff ); // U2
    mbs->setReply(0xff);
    msleep(polltime+100);
    REQUIRE( ui->getValue(1008) == 0x00ff00ff ); // I2
    REQUIRE( ui->getValue(1009) == 0x00ff00ff ); // U2

    // т.к. для VTypes есть отдельный тест, то смысла проверять отдельно типы нету.
    // потому-что по протоколу приходит просто поток байт (регистров), а уже дальше проиходит преобразование
    // при помощи vtype.. т.е. вроде как ModbusMaster уже не причём..
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: 0x04 (read input registers or memories or read word outputs or memories)","[modbus][0x04][mbmaster][mbtcpmaster]")
{
    InitTest();

    CHECK( ui->isExist(mbID) );
    mbs->setReply(10);
    msleep(polltime+100);
    REQUIRE( ui->getValue(1010) == 10 );
    REQUIRE( ui->getValue(1011) == 10 );
    REQUIRE( ui->getValue(1012) == 10 );
    REQUIRE( ui->getValue(1013) == 10 );
    mbs->setReply(-10);
    msleep(polltime+100);
    REQUIRE( ui->getValue(1010) == -10 );
    REQUIRE( ui->getValue(1011) == -10 );
    REQUIRE( ui->getValue(1012) == -10 );
    REQUIRE( ui->getValue(1013) == -10 );
    mbs->setReply(0);
    msleep(polltime+100);
    REQUIRE( ui->getValue(1010) == 0 );
    REQUIRE( ui->getValue(1011) == 0 );
    REQUIRE( ui->getValue(1012) == 0 );
    REQUIRE( ui->getValue(1013) == 0 );
    mbs->setReply(65535);
    msleep(polltime+100);
    REQUIRE( ui->getValue(1010) == -1 );
    REQUIRE( ui->getValue(1011) == -1 );
    REQUIRE( ui->getValue(1012) == -1 );
    REQUIRE( ui->getValue(1013) == -1 );
    REQUIRE( ui->getValue(1014) == 65535 ); // unsigned

    mbs->setReply(0xffff);
    msleep(polltime+100);
    REQUIRE( ui->getValue(1015) == 0xffffffff ); // I2
    REQUIRE( ui->getValue(1016) == 0xffffffff ); // U2
    mbs->setReply(0xff);
    msleep(polltime+100);
    REQUIRE( ui->getValue(1015) == 0x00ff00ff ); // I2
    REQUIRE( ui->getValue(1016) == 0x00ff00ff ); // U2

    // т.к. для VTypes есть отдельный тест, то смысла проверять отдельно типы нету.
    // потому-что по протоколу приходит просто поток байт (регистров), а уже дальше проиходит преобразование
    // при помощи vtype.. т.е. вроде как ModbusMaster уже не причём..
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: 0x05 (forces a single coil to either ON or OFF)","[modbus][0x05][mbmaster][mbtcpmaster]")
{
    InitTest();

    CHECK( ui->isExist(mbID) );
    ui->setValue(1017,0);
    REQUIRE( ui->getValue(1017) == 0 );
    msleep(polltime+100);
    CHECK_FALSE( mbs->getForceSingleCoilCmd() );

    ui->setValue(1017,1);
    REQUIRE( ui->getValue(1017) == 1 );
    msleep(polltime+100);
    CHECK( mbs->getForceSingleCoilCmd() );

    ui->setValue(1017,0);
    REQUIRE( ui->getValue(1017) == 0 );
    msleep(polltime+100);
    CHECK_FALSE( mbs->getForceSingleCoilCmd() );
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: 0x06 (write register outputs or memories)","[modbus][0x06][mbmaster][mbtcpmaster]")
{
    InitTest();

    CHECK( ui->isExist(mbID) );
    ui->setValue(1018,0);
    REQUIRE( ui->getValue(1018) == 0 );
    msleep(polltime+100);
    REQUIRE( mbs->getLastWriteOutputSingleRegister() == 0 );

    ui->setValue(1018,100);
    REQUIRE( ui->getValue(1018) == 100 );
    msleep(polltime+100);
    REQUIRE( mbs->getLastWriteOutputSingleRegister() == 100 );

    ui->setValue(1018,-100);
    REQUIRE( ui->getValue(1018) == -100 );
    msleep(polltime+100);
    REQUIRE( mbs->getLastWriteOutputSingleRegister() == -100 );

    ui->setValue(1018,0);
    REQUIRE( ui->getValue(1018) == 0 );
    msleep(polltime+100);
    REQUIRE( mbs->getLastWriteOutputSingleRegister() == 0 );
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: 0x0F (force multiple coils)","[modbus][0x0F][mbmaster][mbtcpmaster]")
{
    InitTest();

    // три бита..
    {
        ui->setValue(1024,0);
        ui->setValue(1025,0);
        ui->setValue(1026,0);
        REQUIRE( ui->getValue(1024) == 0 );
        REQUIRE( ui->getValue(1025) == 0 );
        REQUIRE( ui->getValue(1026) == 0 );
        msleep(polltime+100);

        ModbusRTU::ForceCoilsMessage q = mbs->getLastForceCoilsQ();
        REQUIRE( q.start == 38 );
        REQUIRE( q.quant == 3 );
        REQUIRE( q.data[0] == 0 );
    }

    {
        ui->setValue(1025,1);
        REQUIRE( ui->getValue(1025) == 1 );
        msleep(polltime+100);

        ModbusRTU::ForceCoilsMessage q = mbs->getLastForceCoilsQ();
        REQUIRE( q.start == 38 );
        REQUIRE( q.quant == 3 );
        REQUIRE( q.data[0] == 2 );
    }

    {
        ui->setValue(1024,1);
        REQUIRE( ui->getValue(1024) == 1 );
        msleep(polltime+100);

        ModbusRTU::ForceCoilsMessage q = mbs->getLastForceCoilsQ();
        REQUIRE( q.start == 38 );
        REQUIRE( q.quant == 3 );
        REQUIRE( q.data[0] == 3 );
    }

    {
        ui->setValue(1024,0);
        ui->setValue(1025,0);
        ui->setValue(1026,0);
        REQUIRE( ui->getValue(1024) == 0 );
        REQUIRE( ui->getValue(1025) == 0 );
        REQUIRE( ui->getValue(1026) == 0 );
        msleep(polltime+100);

        ModbusRTU::ForceCoilsMessage q = mbs->getLastForceCoilsQ();
        REQUIRE( q.start == 38 );
        REQUIRE( q.quant == 3 );
        REQUIRE( q.data[0] == 0 );
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: 0x10 (write register outputs or memories)","[modbus][0x10][mbmaster][mbtcpmaster]")
{
    InitTest();

    {
        ui->setValue(1019,0);
        ui->setValue(1020,0);
        ui->setValue(1021,0);
        ui->setValue(1022,0);
        REQUIRE( ui->getValue(1019) == 0 );
        REQUIRE( ui->getValue(1020) == 0 );
        REQUIRE( ui->getValue(1021) == 0 );
        REQUIRE( ui->getValue(1022) == 0 );
        msleep(polltime+100);

        ModbusRTU::WriteOutputMessage q = mbs->getLastWriteOutput();
        REQUIRE( q.addr == slaveADDR );
        REQUIRE( q.start == 31 );
        REQUIRE( q.quant == 6 );
        REQUIRE( q.data[0] == 0 );
        REQUIRE( q.data[1] == 0 );
        REQUIRE( q.data[2] == 0 );
        REQUIRE( q.data[3] == 0 );
    }
    {
        ui->setValue(1019,100);
        ui->setValue(1020,1);
        ui->setValue(1021,10);
        ui->setValue(1022,65535);
        REQUIRE( ui->getValue(1019) == 100 );
        REQUIRE( ui->getValue(1020) == 1 );
        REQUIRE( ui->getValue(1021) == 10 );
        REQUIRE( ui->getValue(1022) == 65535 );
        msleep(polltime+100);

        ModbusRTU::WriteOutputMessage q = mbs->getLastWriteOutput();
        REQUIRE( q.addr == slaveADDR );
        REQUIRE( q.start == 31 );
        REQUIRE( q.quant == 6 );
        REQUIRE( q.data[0] == 100 );
        REQUIRE( q.data[1] == 1 );
        REQUIRE( q.data[2] == 10 );
        REQUIRE( q.data[3] == 65535 );
    }
    {
        ui->setValue(1019,-100);
        ui->setValue(1021,-10);
        ui->setValue(1022,-32767);
        REQUIRE( ui->getValue(1019) == -100 );
        REQUIRE( ui->getValue(1021) == -10 );
        REQUIRE( ui->getValue(1022) == -32767 );
        msleep(polltime+100);

        ModbusRTU::WriteOutputMessage q = mbs->getLastWriteOutput();
        REQUIRE( q.addr == slaveADDR );
        REQUIRE( q.start == 31 );
        REQUIRE( q.quant == 6 );
        REQUIRE( q.data[0] == (unsigned short)(-100) );
        REQUIRE( q.data[2] == (unsigned short)(-10) );
        REQUIRE( q.data[3] == (unsigned short)(-32767) );
    }

    SECTION("I2")
    {
        ui->setValue(1023,0xffffffff);
        REQUIRE( ui->getValue(1023) == 0xffffffff );
        msleep(polltime+100);

        ModbusRTU::WriteOutputMessage q = mbs->getLastWriteOutput();
        REQUIRE( q.addr == slaveADDR );
        REQUIRE( q.start == 31 );
        REQUIRE( q.quant == 6 );
        REQUIRE( q.data[4] == 0xffff );
        REQUIRE( q.data[5] == 0xffff );
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: exchangeMode","[modbus][exchangemode][mbmaster][mbtcpmaster]")
{
    InitTest();

    SECTION("None")
    {
        SECTION("read")
        {
            mbs->setReply(10);
            msleep(polltime+100);
            REQUIRE( ui->getValue(1003) == 10 );
        }
        SECTION("write")
        {
            ui->setValue(1018,10);
            REQUIRE( ui->getValue(1018) == 10 );
            msleep(polltime+100);
            REQUIRE( mbs->getLastWriteOutputSingleRegister() == 10 );
        }
    }

    SECTION("WriteOnly")
    {
        // emWriteOnly=1,     /*!< "только посылка данных" (работают только write-функции) */
        ui->setValue(exchangeMode,MBExchange::emWriteOnly );
        REQUIRE( ui->getValue(exchangeMode) == MBExchange::emWriteOnly );

        SECTION("read")
        {
            mbs->setReply(150);
            msleep(2*polltime+100);
            REQUIRE( ui->getValue(1003) != 150 );
            mbs->setReply(-10);
            msleep(2*polltime+100);
            REQUIRE( ui->getValue(1003) != -10 );
            REQUIRE( ui->getValue(1003) != 150 );
        }
        SECTION("write")
        {
            ui->setValue(1018,150);
            REQUIRE( ui->getValue(1018) == 150 );
            msleep(polltime+100);
            REQUIRE( mbs->getLastWriteOutputSingleRegister() == 150 );
            ui->setValue(1018,155);
            REQUIRE( ui->getValue(1018) == 155 );
            msleep(polltime+100);
            REQUIRE( mbs->getLastWriteOutputSingleRegister() == 155 );
        }
    }

    SECTION("ReadOnly")
    {
        // emReadOnly=2,        /*!< "только чтение" (работают только read-функции) */
        ui->setValue(exchangeMode,MBExchange::emReadOnly );
        REQUIRE( ui->getValue(exchangeMode) == MBExchange::emReadOnly );

        SECTION("read")
        {
            mbs->setReply(150);
            msleep(polltime+100);
            REQUIRE( ui->getValue(1003) == 150 );
            mbs->setReply(-100);
            msleep(polltime+100);
            REQUIRE( ui->getValue(1003) == -100 );
        }
        SECTION("write")
        {
            ui->setValue(1018,50);
            REQUIRE( ui->getValue(1018) == 50 );
            msleep(2*polltime+100);
            REQUIRE( mbs->getLastWriteOutputSingleRegister() != 50 );
            ui->setValue(1018,55);
            REQUIRE( ui->getValue(1018) == 55 );
            msleep(2*polltime+100);
            REQUIRE( mbs->getLastWriteOutputSingleRegister() != 55 );
            REQUIRE( mbs->getLastWriteOutputSingleRegister() != 50 );
        }
    }

    SECTION("SkipSaveToSM")
    {
        // emSkipSaveToSM=3,    /*!< не писать данные в SM (при этом работают и read и write функции */
        ui->setValue(exchangeMode,MBExchange::emSkipSaveToSM );
        REQUIRE( ui->getValue(exchangeMode) == MBExchange::emSkipSaveToSM );

        SECTION("read")
        {
            mbs->setReply(50);
            msleep(polltime+100);
            REQUIRE( ui->getValue(1003) != 50 );
        }
        SECTION("write")
        {
            // а write работает в этом режиме.. (а чем отличается от writeOnly?)
            ui->setValue(1018,60);
            REQUIRE( ui->getValue(1018) == 60 );
            msleep(polltime+100);
            REQUIRE( mbs->getLastWriteOutputSingleRegister() == 60 );
            ui->setValue(1018,65);
            REQUIRE( ui->getValue(1018) == 65 );
            msleep(polltime+100);
            REQUIRE( mbs->getLastWriteOutputSingleRegister() == 65 );
        }
    }

    SECTION("SkipExchange")
    {
        // emSkipExchange=4  /*!< отключить обмен */
        ui->setValue(exchangeMode,MBExchange::emSkipExchange );
        REQUIRE( ui->getValue(exchangeMode) == MBExchange::emSkipExchange );

        SECTION("read")
        {
            mbs->setReply(70);
            msleep(polltime+100);
            REQUIRE( ui->getValue(1003) != 70 );
        }
        SECTION("write")
        {
            ui->setValue(1018,70);
            REQUIRE( ui->getValue(1018) == 70 );
            msleep(polltime+100);
            REQUIRE( mbs->getLastWriteOutputSingleRegister() != 70 );
        }

        SECTION("check connection")
        {
            msleep(1100);
            CHECK( ui->getValue(slaveNotRespond) == 1 );
            ui->setValue(exchangeMode,MBExchange::emNone );
            REQUIRE( ui->getValue(exchangeMode) == MBExchange::emNone );
            msleep(1100);
            CHECK( ui->getValue(slaveNotRespond) == 0 );
        }
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: iobase functions","[modbus][iobase][mbmaster][mbtcpmaster]")
{
    WARN("Test of 'iobase functions'..not yet.. ");
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: reconnection","[modbus][reconnection][mbmaster][mbtcpmaster]")
{
    WARN("Test of 'reconnection'..not yet.. ");
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: 0x14 (read file record)","[modbus][0x14][mbmaster][mbtcpmaster]")
{
    WARN("Test of '0x14'..not yet.. ");
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: 0x15 (write file record)","[modbus][0x15][mbmaster][mbtcpmaster]")
{
    WARN("Test of '0x15'..not yet.. ");
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: 0x2B (Modbus Encapsulated Interface / MEI /)","[modbus][0x2B][mbmaster][mbtcpmaster]")
{
    WARN("Test of '0x2B'..not yet.. ");
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: 0x50 (set date and time)","[modbus][0x50][mbmaster][mbtcpmaster]")
{
    WARN("Test of '0x50'..not yet.. ");
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: 0x53 (call remote service)","[modbus][0x53][mbmaster][mbtcpmaster]")
{
    WARN("Test of '0x53'..not yet.. ");
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: 0x65 (read,write,delete alarm journal)","[modbus][0x65][mbmaster][mbtcpmaster]")
{
    WARN("Test of '0x65'..not yet.. ");
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: 0x66 (file transfer)","[modbus][0x66][mbmaster][mbtcpmaster]")
{
    WARN("Test of '0x66'..not yet.. ");
}
// -----------------------------------------------------------------------------
