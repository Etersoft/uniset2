#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <time.h>
#include <memory>
#include <unordered_set>
#include <limits>
#include <Poco/Net/NetException.h>
#include "UniSetTypes.h"
#include "MBTCPTestServer.h"
#include "MBTCPMultiMaster.h"
#include "MBTCPMaster.h"
#include "UniSetActivator.h"

#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/Net/HTTPClientSession.h"
#include "Poco/JSON/Parser.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// -----------------------------------------------------------------------------
static ModbusRTU::ModbusAddr slaveADDR = 0x01; // conf->getArgInt("--mbs-my-addr");
static int port = 20048; // conf->getArgInt("--mbs-inet-port");
static string iaddr("127.0.0.1"); // conf->getArgParam("--mbs-inet-addr");
static unordered_set<ModbusRTU::ModbusAddr> vaddr = { slaveADDR, 0x02, 0x03 };
static shared_ptr<MBTCPTestServer> mbs;
static shared_ptr<UInterface> ui;
static std::shared_ptr<SMInterface> smi;
static ObjectId mbID = 6004; // MBTCPMaster1
static int polltime = 100; // conf->getArgInt("--mbtcp-polltime");
static ObjectId slaveNotRespond = 10; // Slave_Not_Respond_S
static const ObjectId exchangeMode = 11; // MBTCPMaster_Mode_AS
static const string confile2 = "mbmaster-test-configure2.xml";
static const string httpAddr = "127.0.0.1";
static const uint16_t httpPort = 9090;
// -----------------------------------------------------------------------------
extern std::shared_ptr<SharedMemory> shm;
extern std::shared_ptr<MBTCPMaster> mbm;
// -----------------------------------------------------------------------------
static void InitTest()
{
    auto conf = uniset_conf();
    CHECK( conf != nullptr );

    if( !ui )
    {
        ui = make_shared<UInterface>();
        // UI понадобиться для проверки записанных в SM значений.
        CHECK( ui->getObjectIndex() != nullptr );
        CHECK( ui->getConf() == conf );
        CHECK( ui->waitReady(slaveNotRespond, 8000) );
    }

    if( !smi )
    {
        if( shm == nullptr )
            throw SystemError("SharedMemory don`t initialize..");

        if( ui == nullptr )
            throw SystemError("UInterface don`t initialize..");

        smi = make_shared<SMInterface>(shm->getId(), ui, mbID, shm );
    }

    if( !mbs )
    {
        try
        {
            mbs = make_shared<MBTCPTestServer>(vaddr, iaddr, port, false);
        }
        catch( const Poco::Net::NetException& e )
        {
            ostringstream err;
            err << "(mbs): Can`t create socket " << iaddr << ":" << port << " err: " << e.message() << endl;
            cerr << err.str() << endl;
            throw SystemError(err.str());
        }
        catch( const std::exception& ex )
        {
            cerr << "(mbs): Can`t create socket " << iaddr << ":" << port << " err: " << ex.what() << endl;
            throw;
        }

        //      mbs->setVerbose(true);
        CHECK( mbs != nullptr );
        mbs->execute();

        for( int i = 0; !mbs->isRunning() && i < 10; i++ )
            msleep(200);

        CHECK( mbs->isRunning() );
        msleep(2000);
        CHECK( ui->getValue(slaveNotRespond) == 0 );
    }

    REQUIRE( mbm != nullptr );
}
// -----------------------------------------------------------------------------
static bool exchangeIsOk()
{
    PassiveTimer pt(5000);

    while( !pt.checkTime() && ui->getValue(slaveNotRespond) )
        msleep(300);

    return !pt.checkTime();
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: reconnect", "[modbus][mbmaster][mbtcpmaster]")
{
    InitTest();

    ModbusTCPMaster mb;
    mb.setTimeout(500);

    // подключение к несуществующему адресу
    REQUIRE_FALSE(mb.connect("dummyhost", 2048));
    REQUIRE_FALSE(mb.isConnection());

    // нормальное подключение
    REQUIRE(mb.connect(iaddr, port));
    REQUIRE(mb.isConnection());

    // переподключение (при активном текущем)
    REQUIRE(mb.reconnect());
    REQUIRE(mb.isConnection());

    // отключение
    mb.disconnect();
    REQUIRE_FALSE(mb.isConnection());

    // переподключение после отключения
    REQUIRE(mb.reconnect());
    REQUIRE(mb.isConnection());

    // переподключение к несуществующему (при наличии активного подключения)
    REQUIRE_FALSE(mb.connect("dummyhost", 2048));
    REQUIRE_FALSE(mb.isConnection());

    // нормальное подключение
    REQUIRE(mb.connect(iaddr, port));
    REQUIRE(mb.isConnection());

    // принудительное отключение
    mb.forceDisconnect();
    REQUIRE_FALSE(mb.isConnection());
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: forceDisconnect", "[modbus][mbmaster][mbtcpmaster][forceDisconnect]")
{
    InitTest();
    ModbusTCPMaster mb;
    mb.setTimeout(500);

    for( size_t i = 0; i < 1000; i++ )
    {
        // подключение к несуществующему адресу
        REQUIRE_FALSE(mb.connect(iaddr, 2048));

        try
        {
            mb.read03(slaveADDR, 10, 1);
        }
        catch(...) {}

        REQUIRE_FALSE(mb.isConnection());
        mb.forceDisconnect();
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: disconnect", "[modbus][mbmaster][mbtcpmaster][disconnect]")
{
    InitTest();
    ModbusTCPMaster mb;
    mb.setTimeout(500);
    mb.setForceDisconnect(true);

    for( size_t i = 0; i < 1000; i++ )
    {
        // подключение к несуществующему адресу
        REQUIRE_FALSE(mb.connect(iaddr, 2048));

        try
        {
            mb.read03(slaveADDR, 10, 1);
        }
        catch(...) {}

        REQUIRE_FALSE(mb.isConnection());
        mb.disconnect();
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: 0x01 (read coil status)", "[modbus][0x01][mbmaster][mbtcpmaster]")
{
    InitTest();

    REQUIRE(exchangeIsOk());
    CHECK( ui->isExist(mbID) );
    mbs->setReply(65535);
    msleep(polltime + 5000);
    REQUIRE( ui->getValue(1000) == 1 );
    REQUIRE( ui->getValue(1001) == 1 );
    REQUIRE( ui->getValue(1002) == 1 );

    mbs->setReply(0);
    msleep(polltime + 200);
    REQUIRE( ui->getValue(1000) == 0 );
    REQUIRE( ui->getValue(1001) == 0 );
    REQUIRE( ui->getValue(1002) == 0 );
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: 0x02 (read input status)", "[modbus][0x02][mbmaster][mbtcpmaster]")
{
    InitTest();
    REQUIRE(exchangeIsOk());
    CHECK( ui->isExist(mbID) );
    mbs->setReply(65535);
    msleep(polltime + 200);
    REQUIRE( ui->getValue(1040) == 1 );
    REQUIRE( ui->getValue(1041) == 1 );
    REQUIRE( ui->getValue(1042) == 1 );

    mbs->setReply(0);
    msleep(polltime + 200);
    REQUIRE( ui->getValue(1040) == 0 );
    REQUIRE( ui->getValue(1041) == 0 );
    REQUIRE( ui->getValue(1042) == 0 );
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: 0x03 (read register outputs or memories or read word outputs or memories)", "[modbus][0x03][mbmaster][mbtcpmaster]")
{
    InitTest();
    REQUIRE(exchangeIsOk());
    CHECK( ui->isExist(mbID) );
    mbs->setReply(10);
    msleep(polltime + 200);
    REQUIRE( ui->getValue(1003) == 10 );
    REQUIRE( ui->getValue(1004) == 10 );
    REQUIRE( ui->getValue(1005) == 10 );
    REQUIRE( ui->getValue(1006) == 10 );
    mbs->setReply(-10);
    msleep(polltime + 200);
    REQUIRE( ui->getValue(1003) == -10 );
    REQUIRE( ui->getValue(1004) == -10 );
    REQUIRE( ui->getValue(1005) == -10 );
    REQUIRE( ui->getValue(1006) == -10 );
    mbs->setReply(1);
    msleep(polltime + 200);
    REQUIRE( ui->getValue(1003) == 1 );
    REQUIRE( ui->getValue(1004) == 1 );
    REQUIRE( ui->getValue(1005) == 1 );
    REQUIRE( ui->getValue(1006) == 1 );
    mbs->setReply(65535);
    msleep(polltime + 200);
    REQUIRE( ui->getValue(1003) == -1 );
    REQUIRE( ui->getValue(1004) == -1 );
    REQUIRE( ui->getValue(1005) == -1 );
    REQUIRE( ui->getValue(1006) == -1 );
    REQUIRE( ui->getValue(1007) == 65535 ); // unsigned

    mbs->setReply( std::numeric_limits<uint16_t>::max() );
    msleep(polltime + 200);
    REQUIRE( (uint16_t)ui->getValue(1009) == std::numeric_limits<uint16_t>::max() ); // U2

    mbs->setReply( std::numeric_limits<int16_t>::max() );
    msleep(polltime + 200);
    REQUIRE( (int16_t)ui->getValue(1008) == std::numeric_limits<int16_t>::max() ); // I2

    mbs->setReply(0xff);
    msleep(polltime + 200);
    REQUIRE( ui->getValue(1008) == 0x00ff00ff ); // I2
    REQUIRE( ui->getValue(1009) == 0x00ff00ff ); // U2

    // т.к. для VTypes есть отдельный тест, то смысла проверять отдельно типы нету.
    // потому-что по протоколу приходит просто поток байт (регистров), а уже дальше проиходит преобразование
    // при помощи vtype.. т.е. вроде как ModbusMaster уже не причём..
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: 0x04 (read input registers or memories or read word outputs or memories)", "[modbus][0x04][mbmaster][mbtcpmaster]")
{
    InitTest();
    REQUIRE(exchangeIsOk());
    CHECK( ui->isExist(mbID) );
    mbs->setReply(10);
    msleep(polltime + 200);
    REQUIRE( ui->getValue(1010) == 10 );
    REQUIRE( ui->getValue(1011) == 10 );
    REQUIRE( ui->getValue(1012) == 10 );
    REQUIRE( ui->getValue(1013) == 10 );
    mbs->setReply(-10);
    msleep(polltime + 200);
    REQUIRE( ui->getValue(1010) == -10 );
    REQUIRE( ui->getValue(1011) == -10 );
    REQUIRE( ui->getValue(1012) == -10 );
    REQUIRE( ui->getValue(1013) == -10 );
    mbs->setReply(0);
    msleep(polltime + 200);
    REQUIRE( ui->getValue(1010) == 0 );
    REQUIRE( ui->getValue(1011) == 0 );
    REQUIRE( ui->getValue(1012) == 0 );
    REQUIRE( ui->getValue(1013) == 0 );
    mbs->setReply(65535);
    msleep(polltime + 200);
    REQUIRE( ui->getValue(1010) == -1 );
    REQUIRE( ui->getValue(1011) == -1 );
    REQUIRE( ui->getValue(1012) == -1 );
    REQUIRE( ui->getValue(1013) == -1 );
    REQUIRE( ui->getValue(1014) == 65535 ); // unsigned

    mbs->setReply( std::numeric_limits<uint16_t>::max() );
    msleep(polltime + 200);
    REQUIRE( (uint16_t)ui->getValue(1009) == std::numeric_limits<uint16_t>::max() ); // U2

    mbs->setReply( std::numeric_limits<int16_t>::max() );
    msleep(polltime + 200);
    REQUIRE( (int16_t)ui->getValue(1008) == std::numeric_limits<int16_t>::max() ); // I2

    mbs->setReply(0xff);
    msleep(polltime + 200);
    REQUIRE( ui->getValue(1015) == 0x00ff00ff ); // I2
    REQUIRE( ui->getValue(1016) == 0x00ff00ff ); // U2

    // т.к. для VTypes есть отдельный тест, то смысла проверять отдельно типы нету.
    // потому-что по протоколу приходит просто поток байт (регистров), а уже дальше проиходит преобразование
    // при помощи vtype.. т.е. вроде как ModbusMaster уже не причём..
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: 0x05 (forces a single coil to either ON or OFF)", "[modbus][0x05][mbmaster][mbtcpmaster]")
{
    InitTest();
    REQUIRE(exchangeIsOk());
    CHECK( ui->isExist(mbID) );
    ui->setValue(1017, 0);
    REQUIRE( ui->getValue(1017) == 0 );
    msleep(polltime + 200);
    CHECK_FALSE( mbs->getForceSingleCoilCmd() );

    ui->setValue(1017, 1);
    REQUIRE( ui->getValue(1017) == 1 );
    msleep(polltime + 200);
    CHECK( mbs->getForceSingleCoilCmd() );

    ui->setValue(1017, 0);
    REQUIRE( ui->getValue(1017) == 0 );
    msleep(polltime + 200);
    CHECK_FALSE( mbs->getForceSingleCoilCmd() );
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: 0x06 (write register outputs or memories)", "[modbus][0x06][mbmaster][mbtcpmaster]")
{
    InitTest();
    REQUIRE(exchangeIsOk());
    CHECK( ui->isExist(mbID) );
    ui->setValue(1018, 0);
    REQUIRE( ui->getValue(1018) == 0 );
    msleep(polltime + 200);
    REQUIRE( mbs->getLastWriteRegister(30) == 0 );

    ui->setValue(1018, 100);
    REQUIRE( ui->getValue(1018) == 100 );
    msleep(polltime + 200);
    REQUIRE( mbs->getLastWriteRegister(30) == 100 );

    ui->setValue(1018, -100);
    REQUIRE( ui->getValue(1018) == -100 );
    msleep(polltime + 200);
    REQUIRE( mbs->getLastWriteRegister(30) == -100 );

    ui->setValue(1018, 0);
    REQUIRE( ui->getValue(1018) == 0 );
    msleep(polltime + 200);
    REQUIRE( mbs->getLastWriteRegister(30) == 0 );
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: 0x0F (force multiple coils)", "[modbus][0x0F][mbmaster][mbtcpmaster]")
{
    InitTest();
    REQUIRE(exchangeIsOk());
    // три бита..
    {
        ui->setValue(1024, 0);
        ui->setValue(1025, 0);
        ui->setValue(1026, 0);
        REQUIRE( ui->getValue(1024) == 0 );
        REQUIRE( ui->getValue(1025) == 0 );
        REQUIRE( ui->getValue(1026) == 0 );
        msleep(polltime + 200);

        ModbusRTU::ForceCoilsMessage q = mbs->getLastForceCoilsQ();
        REQUIRE( q.start == 38 );
        REQUIRE( q.quant == 3 );
        REQUIRE( q.data[0] == 0 );
    }

    {
        ui->setValue(1025, 1);
        REQUIRE( ui->getValue(1025) == 1 );
        msleep(polltime + 200);

        ModbusRTU::ForceCoilsMessage q = mbs->getLastForceCoilsQ();
        REQUIRE( q.start == 38 );
        REQUIRE( q.quant == 3 );
        REQUIRE( q.data[0] == 2 );
    }

    {
        ui->setValue(1024, 1);
        REQUIRE( ui->getValue(1024) == 1 );
        msleep(polltime + 200);

        ModbusRTU::ForceCoilsMessage q = mbs->getLastForceCoilsQ();
        REQUIRE( q.start == 38 );
        REQUIRE( q.quant == 3 );
        REQUIRE( q.data[0] == 3 );
    }

    {
        ui->setValue(1024, 0);
        ui->setValue(1025, 0);
        ui->setValue(1026, 0);
        REQUIRE( ui->getValue(1024) == 0 );
        REQUIRE( ui->getValue(1025) == 0 );
        REQUIRE( ui->getValue(1026) == 0 );
        msleep(polltime + 200);

        ModbusRTU::ForceCoilsMessage q = mbs->getLastForceCoilsQ();
        REQUIRE( q.start == 38 );
        REQUIRE( q.quant == 3 );
        REQUIRE( q.data[0] == 0 );
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: 0x10 (write register outputs or memories)", "[modbus][0x10][mbmaster][mbtcpmaster]")
{
    InitTest();
    REQUIRE(exchangeIsOk());
    {
        ui->setValue(1019, 0);
        ui->setValue(1020, 0);
        ui->setValue(1021, 0);
        ui->setValue(1022, 0);
        REQUIRE( ui->getValue(1019) == 0 );
        REQUIRE( ui->getValue(1020) == 0 );
        REQUIRE( ui->getValue(1021) == 0 );
        REQUIRE( ui->getValue(1022) == 0 );
        msleep(polltime + 200);

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
        ui->setValue(1019, 100);
        ui->setValue(1020, 1);
        ui->setValue(1021, 10);
        ui->setValue(1022, 65535);
        REQUIRE( ui->getValue(1019) == 100 );
        REQUIRE( ui->getValue(1020) == 1 );
        REQUIRE( ui->getValue(1021) == 10 );
        REQUIRE( ui->getValue(1022) == 65535 );
        msleep(polltime + 200);

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
        ui->setValue(1019, -100);
        ui->setValue(1021, -10);
        ui->setValue(1022, -32767);
        REQUIRE( ui->getValue(1019) == -100 );
        REQUIRE( ui->getValue(1021) == -10 );
        REQUIRE( ui->getValue(1022) == -32767 );
        msleep(polltime + 200);

        ModbusRTU::WriteOutputMessage q = mbs->getLastWriteOutput();
        REQUIRE( q.addr == slaveADDR );
        REQUIRE( q.start == 31 );
        REQUIRE( q.quant == 6 );
        REQUIRE( q.data[0] == (uint16_t)(-100) );
        REQUIRE( q.data[2] == (uint16_t)(-10) );
        REQUIRE( q.data[3] == (uint16_t)(-32767) );
    }

    SECTION("I2")
    {
        ui->setValue(1023, std::numeric_limits<uint32_t>::max());
        REQUIRE( (uint32_t)ui->getValue(1023) == std::numeric_limits<uint32_t>::max() );
        msleep(polltime + 200);

        ModbusRTU::WriteOutputMessage q = mbs->getLastWriteOutput();
        REQUIRE( q.addr == slaveADDR );
        REQUIRE( q.start == 31 );
        REQUIRE( q.quant == 6 );
        REQUIRE( q.data[4] == std::numeric_limits<uint16_t>::max() );
        REQUIRE( q.data[5] == std::numeric_limits<uint16_t>::max() );
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: read/write mask", "[modbus][mbmask][mbmaster][mbtcpmaster]")
{
    InitTest();
    REQUIRE(exchangeIsOk());

    // mbmask = 3

    CHECK( ui->isExist(mbID) );
    mbs->setReply(1);
    msleep(polltime + 200);
    REQUIRE( ui->getValue(1090) == 1 );
    REQUIRE( ui->getValue(1091) == 0 );
    REQUIRE( ui->getValue(1092) == 1 );

    mbs->setReply(12);
    msleep(polltime + 200);
    REQUIRE( ui->getValue(1090) == 0 );
    REQUIRE( ui->getValue(1091) == 3 );
    REQUIRE( ui->getValue(1092) == 0 );

    mbs->setReply(3);
    msleep(polltime + 200);
    REQUIRE( ui->getValue(1090) == 3 );
    REQUIRE( ui->getValue(1091) == 0 );
    REQUIRE( ui->getValue(1092) == 1 );

    mbs->setReply(9);
    msleep(polltime + 200);
    REQUIRE( ui->getValue(1090) == 1 );
    REQUIRE( ui->getValue(1091) == 2 );
    REQUIRE( ui->getValue(1092) == 1 );

    ui->setValue(1093, 0);
    ui->setValue(1094, 0);
    msleep(polltime + 200);
    REQUIRE( mbs->getLastWriteRegister(262) == 0 );

    ui->setValue(1093, 1);
    ui->setValue(1094, 0);
    msleep(polltime + 200);
    REQUIRE( mbs->getLastWriteRegister(262) == 1 );

    ui->setValue(1093, 3);
    ui->setValue(1094, 0);
    msleep(polltime + 200);
    REQUIRE( mbs->getLastWriteRegister(262) == 3 );

    ui->setValue(1093, 5);
    ui->setValue(1094, 0);
    msleep(polltime + 200);
    REQUIRE( mbs->getLastWriteRegister(262) == 1 );

    ui->setValue(1093, 3);
    ui->setValue(1094, 1);
    msleep(polltime + 200);
    REQUIRE( mbs->getLastWriteRegister(262) == 7 );

    ui->setValue(1093, 3);
    ui->setValue(1094, 3);
    msleep(polltime + 200);
    REQUIRE( mbs->getLastWriteRegister(262) == 15 );

    ui->setValue(1093, 3);
    ui->setValue(1094, 5);
    msleep(polltime + 200);
    REQUIRE( mbs->getLastWriteRegister(262) == 7 );

    ui->setValue(1093, 0);
    ui->setValue(1094, 0);
    msleep(polltime + 200);
    REQUIRE( mbs->getLastWriteRegister(262) == 0 );

    ui->setValue(1093, 2);
    ui->setValue(1094, 2);
    msleep(polltime + 200);
    REQUIRE( mbs->getLastWriteRegister(262) == 10 );
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: exchangeMode", "[modbus][exchangemode][mbmaster][mbtcpmaster]")
{
    InitTest();
    REQUIRE(exchangeIsOk());

    SECTION("None")
    {
        SECTION("read")
        {
            mbs->setReply(10);
            msleep(polltime + 200);
            REQUIRE( ui->getValue(1003) == 10 );
        }
        SECTION("write")
        {
            ui->setValue(1018, 10);
            REQUIRE( ui->getValue(1018) == 10 );
            msleep(polltime + 200);
            REQUIRE( mbs->getLastWriteRegister(30) == 10 );
        }
    }

    SECTION("WriteOnly")
    {
        // emWriteOnly=1,     /*!< "только посылка данных" (работают только write-функции) */
        ui->setValue(exchangeMode, MBConfig::emWriteOnly );
        REQUIRE( ui->getValue(exchangeMode) == MBConfig::emWriteOnly );

        SECTION("read")
        {
            mbs->setReply(150);
            msleep(2 * polltime + 200);
            REQUIRE( ui->getValue(1003) != 150 );
            mbs->setReply(-10);
            msleep(2 * polltime + 200);
            REQUIRE( ui->getValue(1003) != -10 );
            REQUIRE( ui->getValue(1003) != 150 );
        }
        SECTION("write")
        {
            ui->setValue(1018, 150);
            REQUIRE( ui->getValue(1018) == 150 );
            msleep(polltime + 200);
            REQUIRE( mbs->getLastWriteRegister(30) == 150 );
            ui->setValue(1018, 155);
            REQUIRE( ui->getValue(1018) == 155 );
            msleep(polltime + 200);
            REQUIRE( mbs->getLastWriteRegister(30) == 155 );
        }
    }

    SECTION("ReadOnly")
    {
        // emReadOnly=2,        /*!< "только чтение" (работают только read-функции) */
        ui->setValue(exchangeMode, MBConfig::emReadOnly );
        REQUIRE( ui->getValue(exchangeMode) == MBConfig::emReadOnly );

        SECTION("read")
        {
            mbs->setReply(150);
            msleep(polltime + 200);
            REQUIRE( ui->getValue(1003) == 150 );
            mbs->setReply(-100);
            msleep(polltime + 200);
            REQUIRE( ui->getValue(1003) == -100 );
        }
        SECTION("write")
        {
            ui->setValue(1018, 50);
            REQUIRE( ui->getValue(1018) == 50 );
            msleep(2 * polltime + 200);
            REQUIRE( mbs->getLastWriteRegister(30) != 50 );
            ui->setValue(1018, 55);
            REQUIRE( ui->getValue(1018) == 55 );
            msleep(2 * polltime + 200);
            REQUIRE( mbs->getLastWriteRegister(30) != 55 );
            REQUIRE( mbs->getLastWriteRegister(30) != 50 );
        }
    }

    SECTION("SkipSaveToSM")
    {
        // emSkipSaveToSM=3,    /*!< не писать данные в SM (при этом работают и read и write функции */
        ui->setValue(exchangeMode, MBConfig::emSkipSaveToSM );
        REQUIRE( ui->getValue(exchangeMode) == MBConfig::emSkipSaveToSM );

        SECTION("read")
        {
            mbs->setReply(50);
            msleep(polltime + 200);
            REQUIRE( ui->getValue(1003) != 50 );
        }
        SECTION("write")
        {
            // а write работает в этом режиме.. (а чем отличается от writeOnly?)
            ui->setValue(1018, 60);
            REQUIRE( ui->getValue(1018) == 60 );
            msleep(polltime + 200);
            REQUIRE( mbs->getLastWriteRegister(30) == 60 );
            ui->setValue(1018, 65);
            REQUIRE( ui->getValue(1018) == 65 );
            msleep(polltime + 200);
            REQUIRE( mbs->getLastWriteRegister(30) == 65 );
        }
    }

    SECTION("SkipExchange")
    {
        // emSkipExchange=4  /*!< отключить обмен */
        ui->setValue(exchangeMode, MBConfig::emSkipExchange );
        REQUIRE( ui->getValue(exchangeMode) == MBConfig::emSkipExchange );

        SECTION("read")
        {
            mbs->setReply(70);
            msleep(polltime + 200);
            REQUIRE( ui->getValue(1003) != 70 );
        }
        SECTION("write")
        {
            ui->setValue(1018, 70);
            REQUIRE( ui->getValue(1018) == 70 );
            msleep(polltime + 200);
            REQUIRE( mbs->getLastWriteRegister(30) != 70 );
        }

        SECTION("check connection")
        {
            msleep(1100);
            CHECK( ui->getValue(slaveNotRespond) == 1 );
            ui->setValue(exchangeMode, MBConfig::emNone );
            REQUIRE( ui->getValue(exchangeMode) == MBConfig::emNone );
            msleep(1100);
            CHECK( ui->getValue(slaveNotRespond) == 0 );
        }
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: check respond sensor", "[modbus][respond][mbmaster][mbtcpmaster]")
{
    InitTest();
    REQUIRE(exchangeIsOk());
    mbs->disableExchange(false);
    msleep(3500);
    CHECK( ui->getValue(slaveNotRespond) == 0 );
    mbs->disableExchange(true);
    msleep(3000);
    CHECK( ui->getValue(slaveNotRespond) == 1 );
    mbs->disableExchange(false);
    msleep(3000);
    CHECK( ui->getValue(slaveNotRespond) == 0 );
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: iobase functions", "[modbus][iobase][mbmaster][mbtcpmaster]")
{
    WARN("Test of 'iobase functions'..not yet.. ");
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: reconnection", "[modbus][reconnection][mbmaster][mbtcpmaster]")
{
    WARN("Test of 'reconnection'..not yet.. ");
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: 0x14 (read file record)", "[modbus][0x14][mbmaster][mbtcpmaster]")
{
    WARN("Test of '0x14'..not yet.. ");
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: 0x15 (write file record)", "[modbus][0x15][mbmaster][mbtcpmaster]")
{
    WARN("Test of '0x15'..not yet.. ");
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: 0x2B (Modbus Encapsulated Interface / MEI /)", "[modbus][0x2B][mbmaster][mbtcpmaster]")
{
    WARN("Test of '0x2B'..not yet.. ");
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: 0x50 (set date and time)", "[modbus][0x50][mbmaster][mbtcpmaster]")
{
    WARN("Test of '0x50'..not yet.. ");
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: 0x53 (call remote service)", "[modbus][0x53][mbmaster][mbtcpmaster]")
{
    WARN("Test of '0x53'..not yet.. ");
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: 0x65 (read,write,delete alarm journal)", "[modbus][0x65][mbmaster][mbtcpmaster]")
{
    WARN("Test of '0x65'..not yet.. ");
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: 0x66 (file transfer)", "[modbus][0x66][mbmaster][mbtcpmaster]")
{
    WARN("Test of '0x66'..not yet.. ");
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: safe mode", "[modbus][safemode][mbmaster][mbtcpmaster]")
{
    InitTest();
    ui->setValue(1050, 0); // отключаем safeMode
    REQUIRE(exchangeIsOk());

    mbs->setReply(53);
    msleep(polltime + 200);
    REQUIRE( ui->getValue(1051) == 53 );
    REQUIRE( ui->getValue(1052) == 1 );

    mbs->setReply(0);
    msleep(polltime + 200);
    REQUIRE( ui->getValue(1051) == 0 );
    REQUIRE( ui->getValue(1052) == 0 );

    ui->setValue(1050, 42); // включаем safeMode
    msleep(polltime + 200);
    REQUIRE( ui->getValue(1051) == 42 );
    REQUIRE( ui->getValue(1052) == 1 );

    mbs->setReply(53);
    msleep(polltime + 200);
    REQUIRE( ui->getValue(1051) == 42 );
    REQUIRE( ui->getValue(1052) == 1 );

    ui->setValue(1050, 0); // отключаем safeMode
    msleep(polltime + 200);
    REQUIRE( ui->getValue(1051) == 53 );
    REQUIRE( ui->getValue(1052) == 1 );
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: safe mode (resetIfNotRespond)", "[modbus][safemode][mbmaster][mbtcpmaster]")
{
    InitTest();
    REQUIRE(exchangeIsOk());

    mbs->setReply(53);
    msleep(polltime + 200);
    REQUIRE( ui->getValue(1053) == 53 );
    REQUIRE( ui->getValue(1054) == 1 );

    mbs->setReply(0);
    msleep(polltime + 200);
    REQUIRE( ui->getValue(1053) == 0 );
    REQUIRE( ui->getValue(1054) == 0 );

    mbs->disableExchange(true); // отключаем связь
    msleep(5000);
    REQUIRE( ui->getValue(1053) == 42 );
    REQUIRE( ui->getValue(1054) == 1 );

    mbs->setReply(53);
    msleep(polltime + 200);
    REQUIRE( ui->getValue(1053) == 42 );
    REQUIRE( ui->getValue(1054) == 1 );

    mbs->disableExchange(false); // включаем связь
    msleep(5000);
    REQUIRE( ui->getValue(1053) == 53 );
    REQUIRE( ui->getValue(1054) == 1 );
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: undefined value", "[modbus][undefined][mbmaster][mbtcpmaster]")
{
    InitTest();
    REQUIRE(exchangeIsOk());

    mbs->setReply(120);
    msleep(polltime + 200);
    REQUIRE( ui->getValue(1070) == 120 );

    mbs->setReply(10);
    msleep(polltime + 200);

    try
    {
        ui->getValue(1070);
    }
    catch( IOController_i::Undefined& ex )
    {
        REQUIRE( ex.value == 65535 );
    }

    mbs->setReply(120);
    msleep(polltime + 200);
    REQUIRE( ui->getValue(1070) == 120 );
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: reload config", "[modbus][reload][mbmaster][mbtcpmaster]")
{
    InitTest();

    mbs->setReply(160);
    msleep(polltime + 200);
    REQUIRE( ui->getValue(1070) == 160 );
    REQUIRE( ui->getValue(1080) == 1080 );

    // add new mbreg
    REQUIRE( mbm->reload(confile2) );

    msleep(polltime + 600);
    REQUIRE( ui->getValue(1080) == 160 );
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: reload config (HTTP API)", "[modbus][reload-api][mbmaster][mbtcpmaster]")
{
    InitTest();

    using Poco::Net::HTTPClientSession;
    using Poco::Net::HTTPRequest;
    using Poco::Net::HTTPResponse;

    // default reconfigure
    {
        HTTPClientSession cs(httpAddr, httpPort);
        HTTPRequest req(HTTPRequest::HTTP_GET, "/api/v2/MBTCPMaster1/reload", HTTPRequest::HTTP_1_1);
        HTTPResponse res;

        cs.sendRequest(req);
        std::istream& rs = cs.receiveResponse(res);

        std::ostringstream oss;
        oss << rs.rdbuf();
        std::string info = oss.str();

        REQUIRE( res.getStatus() == HTTPResponse::HTTP_OK );
        REQUIRE_FALSE( info.empty() );
        REQUIRE( info.find("OK") != std::string::npos );
    }

    // reconfigure from other file
    {
        HTTPClientSession cs(httpAddr, httpPort);
        HTTPRequest req(HTTPRequest::HTTP_GET, "/api/v2/MBTCPMaster1/reload?confile=" + confile2, HTTPRequest::HTTP_1_1);
        HTTPResponse res;

        cs.sendRequest(req);
        std::istream& rs = cs.receiveResponse(res);

        std::ostringstream oss;
        oss << rs.rdbuf();
        std::string info = oss.str();

        REQUIRE( res.getStatus() == HTTPResponse::HTTP_OK );
        REQUIRE_FALSE( info.empty() );
        REQUIRE( info.find("OK") != std::string::npos );
        REQUIRE( info.find(confile2) != std::string::npos );
    }

    // reconfigure FAIL
    {
        HTTPClientSession cs(httpAddr, httpPort);
        HTTPRequest req(HTTPRequest::HTTP_GET, "/api/v2/MBTCPMaster1/reload?confile=BADFILE", HTTPRequest::HTTP_1_1);
        HTTPResponse res;

        cs.sendRequest(req);
        std::istream& rs = cs.receiveResponse(res);

        std::ostringstream oss;
        oss << rs.rdbuf();
        std::string info = oss.str();

        REQUIRE_FALSE( info.empty() );
        REQUIRE( info.find("OK") == std::string::npos );
    }
}
// -----------------------------------------------------------------------------
#if 0
// -----------------------------------------------------------------------------
static bool init_iobase( IOBase* ib, const std::string& sensor )
{
    InitTest();

    auto conf = uniset_conf();
    xmlNode* snode = conf->getXMLObjectNode( conf->getSensorID(sensor) );
    CHECK( snode != 0 );
    UniXML::iterator it(snode);
    smi->initIterator(ib->d_it);
    smi->initIterator(ib->ioit);
    smi->initIterator(ib->t_ait);
    return IOBase::initItem(ib, it, smi, "", false);
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: 0x10 (F2)", "[modbus][0x10][F2][mbmaster][mbtcpmaster]")
{
    InitTest();

    ui->setValue(1027, 112);
    REQUIRE( ui->getValue(1027) == 112 );
    msleep(polltime + 200);
    ModbusRTU::WriteOutputMessage q = mbs->getLastWriteOutput();
    REQUIRE( q.addr == slaveADDR );
    REQUIRE( q.start == 41 );
    REQUIRE( q.quant == 2 );

    VTypes::F2 f2(q.data, VTypes::F2::wsize());
    float f = f2;
    REQUIRE( f == 11.2f );
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: 0x10 (F4)", "[modbus][0x10][F4][mbmaster][mbtcpmaster]")
{
    InitTest();

    long v = 10000000;

    ui->setValue(1028, v);
    REQUIRE( ui->getValue(1028) == v );
    msleep(polltime + 200);
    ModbusRTU::WriteOutputMessage q = mbs->getLastWriteOutput();
    REQUIRE( q.addr == slaveADDR );
    REQUIRE( q.start == 45 );
    REQUIRE( q.quant == 4 );

    VTypes::F4 f4(q.data, VTypes::F4::wsize());
    float f = f4;
    REQUIRE( f == v );
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: FasAO -> FasAI", "[modbus][float]")
{
    InitTest();

    IOBase ib;
    CHECK( init_iobase(&ib, "TestWrite1027_F2") );

    ui->setValue(1027, 116);
    REQUIRE( ui->getValue(1027) == 116 );
    msleep(polltime + 200);
    ModbusRTU::WriteOutputMessage q = mbs->getLastWriteOutput();
    REQUIRE( q.addr == slaveADDR );
    REQUIRE( q.start == 41 );
    REQUIRE( q.quant == 2 );

    float f2 = mbs->getF2TestValue();

    ui->setValue(1027, 0);
    IOBase::processingFasAI( &ib, f2, smi, true );
    REQUIRE( ui->getValue(1027) == 116 );

    float f3 = IOBase::processingFasAO( &ib, smi, true );
    REQUIRE( f3 == 11.6f );
}
#endif
// -----------------------------------------------------------------------------
#if 0
TEST_CASE("MBTCPMaster: F2 to DI", "[modbus][ftodi]")
{
    InitTest();

    CHECK( ui->isExist(mbID) );
    mbs->setReply(10);
    msleep(polltime + 200);
    REQUIRE( ui->getValue(1028) == 1 );

    mbs->setReply(0);
    msleep(polltime + 200);
    REQUIRE( ui->getValue(1028) == 0 );

    mbs->setReply(10);
    msleep(polltime + 200);
    REQUIRE( ui->getValue(1028) == 1 );
}
#endif
// -----------------------------------------------------------------------------
#ifndef DISABLE_REST_API
TEST_CASE("MBTCPMaster: HTTP /mode?supported", "[http][rest][mbtcpmaster][supported]")
{
    InitTest();
    using Poco::Net::HTTPClientSession;
    using Poco::Net::HTTPRequest;
    using Poco::Net::HTTPResponse;

    HTTPClientSession cs(httpAddr, httpPort);
    HTTPRequest req(HTTPRequest::HTTP_GET, "/api/v2/MBTCPMaster1/mode?supported=1", HTTPRequest::HTTP_1_1);
    HTTPResponse res;

    try
    {
        cs.sendRequest(req);
        std::istream& rs = cs.receiveResponse(res);
        REQUIRE(res.getStatus() == HTTPResponse::HTTP_OK);

        std::stringstream ss;
        ss << rs.rdbuf();

        Poco::JSON::Parser parser;
        auto result = parser.parse(ss.str());
        Poco::JSON::Object::Ptr json = result.extract<Poco::JSON::Object::Ptr>();
        REQUIRE(json);
        REQUIRE(json->get("result").toString() == "OK");

        auto arr = json->get("supported").extract<Poco::JSON::Array::Ptr>();
        REQUIRE(arr);
        REQUIRE(arr->size() >= 3);
    }
    catch(const std::exception& e)
    {
        FAIL(std::string("HTTP /mode?supported failed: ") + e.what());
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: HTTP /mode?get", "[http][rest][mbtcpmaster][get]")
{
    InitTest();

    using Poco::Net::HTTPClientSession;
    using Poco::Net::HTTPRequest;
    using Poco::Net::HTTPResponse;

    HTTPClientSession cs(httpAddr, httpPort);
    HTTPRequest req(HTTPRequest::HTTP_GET, "/api/v2/MBTCPMaster1/mode?get", HTTPRequest::HTTP_1_1);
    HTTPResponse res;

    cs.sendRequest(req);
    std::istream& rs = cs.receiveResponse(res);
    REQUIRE(res.getStatus() == HTTPResponse::HTTP_OK);

    std::stringstream ss;
    ss << rs.rdbuf();

    Poco::JSON::Parser parser;
    auto result = parser.parse(ss.str());
    Poco::JSON::Object::Ptr json = result.extract<Poco::JSON::Object::Ptr>();
    REQUIRE(json);
    REQUIRE(json->get("result").toString() == "OK");
    REQUIRE(json->has("mode"));
    REQUIRE(json->has("mode_id"));
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: HTTP /mode?set=writeOnly (sensor-bound -> error)", "[http][rest][mbtcpmaster][set][negative]")
{
    InitTest();

    using Poco::Net::HTTPClientSession;
    using Poco::Net::HTTPRequest;
    using Poco::Net::HTTPResponse;

    HTTPClientSession cs(httpAddr, httpPort);
    HTTPRequest req(HTTPRequest::HTTP_GET, "/api/v2/MBTCPMaster1/mode?set=writeOnly", HTTPRequest::HTTP_1_1);
    HTTPResponse res;

    cs.sendRequest(req);
    std::istream& rs = cs.receiveResponse(res);

    //  Должна быть ошибка, т.к. управление режимом привязано к датчику
    REQUIRE(res.getStatus() != HTTPResponse::HTTP_OK);
    REQUIRE(res.getStatus() >= HTTPResponse::HTTP_BAD_REQUEST);

    std::stringstream ss;
    ss << rs.rdbuf();
    const std::string body = ss.str();

    cerr << body << endl;

    // Сообщение об ошибке из кода: "control via sensor is enabled"
    REQUIRE(body.find("control via sensor is enabled") != std::string::npos);
}
// -----------------------------------------------------------------------------
// /getparam: базовая проверка на три параметра
TEST_CASE("MBTCPMaster: HTTP /getparam (force, force_out, maxHeartBeat)", "[http][rest][mbtcpmaster][getparam]")
{
    InitTest();

    using Poco::Net::HTTPClientSession;
    using Poco::Net::HTTPRequest;
    using Poco::Net::HTTPResponse;

    HTTPClientSession cs(httpAddr, httpPort);
    HTTPRequest req(HTTPRequest::HTTP_GET,
                    "/api/v2/MBTCPMaster1/getparam?name=force&name=force_out&name=maxHeartBeat",
                    HTTPRequest::HTTP_1_1);
    HTTPResponse res;

    cs.sendRequest(req);
    std::istream& rs = cs.receiveResponse(res);
    REQUIRE(res.getStatus() == HTTPResponse::HTTP_OK);

    std::stringstream ss;
    ss << rs.rdbuf();

    Poco::JSON::Parser parser;
    auto result = parser.parse(ss.str());
    Poco::JSON::Object::Ptr json = result.extract<Poco::JSON::Object::Ptr>();
    REQUIRE(json);
    REQUIRE(json->get("result").toString() == "OK");
    REQUIRE(json->has("params"));

    auto params = json->get("params").extract<Poco::JSON::Object::Ptr>();
    REQUIRE(params);
    REQUIRE(params->has("force"));
    REQUIRE(params->has("force_out"));
    REQUIRE(params->has("maxHeartBeat"));
}
// -----------------------------------------------------------------------------
// /setparam: переключение force и force_out (с возвратом исходных значений)
TEST_CASE("MBTCPMaster: HTTP /setparam (force & force_out)", "[http][rest][mbtcpmaster][setparam][bools]")
{
    InitTest();

    using Poco::Net::HTTPClientSession;
    using Poco::Net::HTTPRequest;
    using Poco::Net::HTTPResponse;

    // Сначала читаем текущие значения
    {
        HTTPClientSession cs(httpAddr, httpPort);
        HTTPRequest req(HTTPRequest::HTTP_GET,
                        "/api/v2/MBTCPMaster1/getparam?name=force&name=force_out",
                        HTTPRequest::HTTP_1_1);
        HTTPResponse res;

        cs.sendRequest(req);
        std::istream& rs = cs.receiveResponse(res);
        REQUIRE(res.getStatus() == HTTPResponse::HTTP_OK);

        std::stringstream ss;
        ss << rs.rdbuf();
        Poco::JSON::Parser parser;
        auto result = parser.parse(ss.str());
        Poco::JSON::Object::Ptr json = result.extract<Poco::JSON::Object::Ptr>();
        auto params = json->get("params").extract<Poco::JSON::Object::Ptr>();
        int prev_force = params->get("force");
        int prev_force_out = params->get("force_out");

        // Выберем новые значения (переключим 0<->1)
        int new_force = prev_force ? 0 : 1;
        int new_force_out = prev_force_out ? 0 : 1;

        // Устанавливаем новые
        {
            HTTPRequest reqSet(HTTPRequest::HTTP_GET,
                               std::string("/api/v2/MBTCPMaster1/setparam?force=")
                               + std::to_string(new_force)
                               + "&force_out=" + std::to_string(new_force_out),
                               HTTPRequest::HTTP_1_1);
            HTTPResponse resSet;
            cs.sendRequest(reqSet);
            std::istream& rsSet = cs.receiveResponse(resSet);
            REQUIRE(resSet.getStatus() == HTTPResponse::HTTP_OK);

            std::stringstream ssSet;
            ssSet << rsSet.rdbuf();
            auto rSet = parser.parse(ssSet.str());
            auto jSet = rSet.extract<Poco::JSON::Object::Ptr>();
            REQUIRE(jSet->get("result").toString() == "OK");
            REQUIRE(jSet->has("updated"));
        }

        // Проверяем, что применилось
        {
            HTTPRequest reqGet2(HTTPRequest::HTTP_GET,
                                "/api/v2/MBTCPMaster1/getparam?name=force&name=force_out",
                                HTTPRequest::HTTP_1_1);
            HTTPResponse resGet2;
            cs.sendRequest(reqGet2);
            std::istream& rsGet2 = cs.receiveResponse(resGet2);
            REQUIRE(resGet2.getStatus() == HTTPResponse::HTTP_OK);

            std::stringstream ssGet2;
            ssGet2 << rsGet2.rdbuf();
            auto r2 = parser.parse(ssGet2.str());
            auto j2 = r2.extract<Poco::JSON::Object::Ptr>();
            auto p2 = j2->get("params").extract<Poco::JSON::Object::Ptr>();

            REQUIRE((int)p2->get("force") == new_force);
            REQUIRE((int)p2->get("force_out") == new_force_out);
        }

        // Возвращаем исходные значения
        {
            HTTPRequest reqBack(HTTPRequest::HTTP_GET,
                                std::string("/api/v2/MBTCPMaster1/setparam?force=")
                                + std::to_string(prev_force)
                                + "&force_out=" + std::to_string(prev_force_out),
                                HTTPRequest::HTTP_1_1);
            HTTPResponse resBack;
            cs.sendRequest(reqBack);
            std::istream& rsBack = cs.receiveResponse(resBack);
            REQUIRE(resBack.getStatus() == HTTPResponse::HTTP_OK);
        }
    }
}
// -----------------------------------------------------------------------------
// /setparam: изменение maxHeartBeat (с возвратом исходного)
TEST_CASE("MBTCPMaster: HTTP /setparam (maxHeartBeat)", "[http][rest][mbtcpmaster][setparam][maxHeartBeat]")
{
    InitTest();

    using Poco::Net::HTTPClientSession;
    using Poco::Net::HTTPRequest;
    using Poco::Net::HTTPResponse;

    HTTPClientSession cs(httpAddr, httpPort);
    Poco::JSON::Parser parser;

    // читаем текущее значение
    int prev_mhb = [&]()
    {
        HTTPRequest req(HTTPRequest::HTTP_GET,
                        "/api/v2/MBTCPMaster1/getparam?name=maxHeartBeat",
                        HTTPRequest::HTTP_1_1);
        HTTPResponse res;
        cs.sendRequest(req);
        std::istream& rs = cs.receiveResponse(res);
        REQUIRE(res.getStatus() == HTTPResponse::HTTP_OK);

        std::stringstream ss;
        ss << rs.rdbuf();
        auto r = parser.parse(ss.str());
        auto j = r.extract<Poco::JSON::Object::Ptr>();
        auto p = j->get("params").extract<Poco::JSON::Object::Ptr>();
        return (int) p->get("maxHeartBeat");
    }
    ();

    // ставим новое значение (сдвиг +1234, минимум 1)
    int new_mhb = prev_mhb + 1234;
    {
        HTTPRequest reqSet(HTTPRequest::HTTP_GET,
                           std::string("/api/v2/MBTCPMaster1/setparam?maxHeartBeat=") + std::to_string(new_mhb),
                           HTTPRequest::HTTP_1_1);
        HTTPResponse resSet;
        cs.sendRequest(reqSet);
        std::istream& rsSet = cs.receiveResponse(resSet);
        REQUIRE(resSet.getStatus() == HTTPResponse::HTTP_OK);

        std::stringstream ssSet;
        ssSet << rsSet.rdbuf();
        auto rSet = parser.parse(ssSet.str());
        auto jSet = rSet.extract<Poco::JSON::Object::Ptr>();
        REQUIRE(jSet->get("result").toString() == "OK");
    }

    // проверяем новое значение
    {
        HTTPRequest reqGet2(HTTPRequest::HTTP_GET,
                            "/api/v2/MBTCPMaster1/getparam?name=maxHeartBeat",
                            HTTPRequest::HTTP_1_1);
        HTTPResponse resGet2;
        cs.sendRequest(reqGet2);
        std::istream& rsGet2 = cs.receiveResponse(resGet2);
        REQUIRE(resGet2.getStatus() == HTTPResponse::HTTP_OK);

        std::stringstream ssGet2;
        ssGet2 << rsGet2.rdbuf();
        auto r2 = parser.parse(ssGet2.str());
        auto j2 = r2.extract<Poco::JSON::Object::Ptr>();
        auto p2 = j2->get("params").extract<Poco::JSON::Object::Ptr>();
        REQUIRE((int) p2->get("maxHeartBeat") == new_mhb);
    }

    // возвращаем исходное
    {
        HTTPRequest reqBack(HTTPRequest::HTTP_GET,
                            std::string("/api/v2/MBTCPMaster1/setparam?maxHeartBeat=") + std::to_string(prev_mhb),
                            HTTPRequest::HTTP_1_1);
        HTTPResponse resBack;
        cs.sendRequest(reqBack);
        std::istream& rsBack = cs.receiveResponse(resBack);
        REQUIRE(resBack.getStatus() == HTTPResponse::HTTP_OK);
    }
}
// -----------------------------------------------------------------------------
// /getparam и /setparam: recv_timeout и polltime (set -> verify -> revert)
TEST_CASE("MBTCPMaster: HTTP /setparam (/getparam) recv_timeout & polltime", "[http][rest][mbtcpmaster][setparam][getparam]")
{
    InitTest();

    using Poco::Net::HTTPClientSession;
    using Poco::Net::HTTPRequest;
    using Poco::Net::HTTPResponse;

    HTTPClientSession cs(httpAddr, httpPort);
    Poco::JSON::Parser parser;

    // 1) читаем текущие значения
    int prev_recv_to = 0, prev_polltime = 0;
    {
        HTTPRequest req(HTTPRequest::HTTP_GET,
                        "/api/v2/MBTCPMaster1/getparam?name=recv_timeout&name=polltime",
                        HTTPRequest::HTTP_1_1);
        HTTPResponse res;
        cs.sendRequest(req);
        std::istream& rs = cs.receiveResponse(res);
        REQUIRE(res.getStatus() == HTTPResponse::HTTP_OK);

        std::stringstream ss;
        ss << rs.rdbuf();
        auto r = parser.parse(ss.str());
        Poco::JSON::Object::Ptr json = r.extract<Poco::JSON::Object::Ptr>();
        auto params = json->get("params").extract<Poco::JSON::Object::Ptr>();
        prev_recv_to = (int)params->get("recv_timeout");
        prev_polltime = (int)params->get("polltime");
    }

    // новые значения (сдвиг на +123/+456)
    int new_recv_to = prev_recv_to + 123;
    int new_polltime = prev_polltime + 456;

    // 2) устанавливаем новые
    {
        HTTPRequest req(HTTPRequest::HTTP_GET,
                        std::string("/api/v2/MBTCPMaster1/setparam?recv_timeout=") + std::to_string(new_recv_to) +
                        "&polltime=" + std::to_string(new_polltime),
                        HTTPRequest::HTTP_1_1);
        HTTPResponse res;
        cs.sendRequest(req);
        std::istream& rs = cs.receiveResponse(res);
        REQUIRE(res.getStatus() == HTTPResponse::HTTP_OK);

        std::stringstream ss;
        ss << rs.rdbuf();
        auto r = parser.parse(ss.str());
        Poco::JSON::Object::Ptr json = r.extract<Poco::JSON::Object::Ptr>();
        REQUIRE(json->get("result").toString() == "OK");
    }

    // 3) проверяем, что применилось
    {
        HTTPRequest req(HTTPRequest::HTTP_GET,
                        "/api/v2/MBTCPMaster1/getparam?name=recv_timeout&name=polltime",
                        HTTPRequest::HTTP_1_1);
        HTTPResponse res;
        cs.sendRequest(req);
        std::istream& rs = cs.receiveResponse(res);
        REQUIRE(res.getStatus() == HTTPResponse::HTTP_OK);

        std::stringstream ss;
        ss << rs.rdbuf();
        auto r = parser.parse(ss.str());
        Poco::JSON::Object::Ptr json = r.extract<Poco::JSON::Object::Ptr>();
        auto params = json->get("params").extract<Poco::JSON::Object::Ptr>();
        REQUIRE((int)params->get("recv_timeout") == new_recv_to);
        REQUIRE((int)params->get("polltime") == new_polltime);
    }

    // 4) возвращаем исходные
    {
        HTTPRequest req(HTTPRequest::HTTP_GET,
                        std::string("/api/v2/MBTCPMaster1/setparam?recv_timeout=") + std::to_string(prev_recv_to) +
                        "&polltime=" + std::to_string(prev_polltime),
                        HTTPRequest::HTTP_1_1);
        HTTPResponse res;
        cs.sendRequest(req);
        std::istream& rs = cs.receiveResponse(res);
        REQUIRE(res.getStatus() == HTTPResponse::HTTP_OK);
    }
}
// -----------------------------------------------------------------------------
// /getparam и /setparam: sleepPause_msec и default_timeout (set -> verify -> revert)
TEST_CASE("MBTCPMaster: HTTP /setparam (/getparam) sleepPause_msec & default_timeout", "[http][rest][mbtcpmaster][setparam][getparam]")
{
    InitTest();

    using Poco::Net::HTTPClientSession;
    using Poco::Net::HTTPRequest;
    using Poco::Net::HTTPResponse;

    HTTPClientSession cs(httpAddr, httpPort);
    Poco::JSON::Parser parser;

    // 1) читаем текущие значения
    int prev_sleep = 0, prev_default_to = 0;
    {
        HTTPRequest req(HTTPRequest::HTTP_GET,
                        "/api/v2/MBTCPMaster1/getparam?name=sleepPause_msec&name=default_timeout",
                        HTTPRequest::HTTP_1_1);
        HTTPResponse res;
        cs.sendRequest(req);
        std::istream& rs = cs.receiveResponse(res);
        REQUIRE(res.getStatus() == HTTPResponse::HTTP_OK);

        std::stringstream ss;
        ss << rs.rdbuf();
        auto r = parser.parse(ss.str());
        Poco::JSON::Object::Ptr json = r.extract<Poco::JSON::Object::Ptr>();
        auto params = json->get("params").extract<Poco::JSON::Object::Ptr>();
        prev_sleep = (int)params->get("sleepPause_msec");
        prev_default_to = (int)params->get("default_timeout");
    }

    // новые значения (сдвиг на +111/+222)
    int new_sleep = prev_sleep + 111;
    int new_default_to = prev_default_to + 222;

    // 2) устанавливаем новые
    {
        HTTPRequest req(HTTPRequest::HTTP_GET,
                        std::string("/api/v2/MBTCPMaster1/setparam?sleepPause_msec=") + std::to_string(new_sleep) +
                        "&default_timeout=" + std::to_string(new_default_to),
                        HTTPRequest::HTTP_1_1);
        HTTPResponse res;
        cs.sendRequest(req);
        std::istream& rs = cs.receiveResponse(res);
        REQUIRE(res.getStatus() == HTTPResponse::HTTP_OK);

        std::stringstream ss;
        ss << rs.rdbuf();
        auto r = parser.parse(ss.str());
        Poco::JSON::Object::Ptr json = r.extract<Poco::JSON::Object::Ptr>();
        REQUIRE(json->get("result").toString() == "OK");
    }

    // 3) проверяем, что применилось
    {
        HTTPRequest req(HTTPRequest::HTTP_GET,
                        "/api/v2/MBTCPMaster1/getparam?name=sleepPause_msec&name=default_timeout",
                        HTTPRequest::HTTP_1_1);
        HTTPResponse res;
        cs.sendRequest(req);
        std::istream& rs = cs.receiveResponse(res);
        REQUIRE(res.getStatus() == HTTPResponse::HTTP_OK);

        std::stringstream ss;
        ss << rs.rdbuf();
        auto r = parser.parse(ss.str());
        Poco::JSON::Object::Ptr json = r.extract<Poco::JSON::Object::Ptr>();
        auto params = json->get("params").extract<Poco::JSON::Object::Ptr>();
        REQUIRE((int)params->get("sleepPause_msec") == new_sleep);
        REQUIRE((int)params->get("default_timeout") == new_default_to);
    }

    // 4) возвращаем исходные
    {
        HTTPRequest req(HTTPRequest::HTTP_GET,
                        std::string("/api/v2/MBTCPMaster1/setparam?sleepPause_msec=") + std::to_string(prev_sleep) +
                        "&default_timeout=" + std::to_string(prev_default_to),
                        HTTPRequest::HTTP_1_1);
        HTTPResponse res;
        cs.sendRequest(req);
        std::istream& rs = cs.receiveResponse(res);
        REQUIRE(res.getStatus() == HTTPResponse::HTTP_OK);
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: HTTP /status extended fields", "[http][rest][mbtcpmaster][status][extended]")
{
    InitTest();

    using Poco::Net::HTTPClientSession;
    using Poco::Net::HTTPRequest;
    using Poco::Net::HTTPResponse;

    HTTPClientSession cs(httpAddr, httpPort);
    HTTPRequest req(HTTPRequest::HTTP_GET, "/api/v2/MBTCPMaster1/status", HTTPRequest::HTTP_1_1);
    HTTPResponse res;

    cs.sendRequest(req);
    std::istream& rs = cs.receiveResponse(res);
    REQUIRE(res.getStatus() == HTTPResponse::HTTP_OK);

    std::stringstream ss;
    ss << rs.rdbuf();
    Poco::JSON::Parser parser;
    auto parsed = parser.parse(ss.str());
    Poco::JSON::Object::Ptr root = parsed.extract<Poco::JSON::Object::Ptr>();
    REQUIRE(root);
    REQUIRE(root->has("result"));
    REQUIRE(root->get("result").toString() == "OK");
    REQUIRE(root->has("status"));

    auto status = root->getObject("status");
    REQUIRE(status);

    // mode-block
    REQUIRE(status->has("mode"));
    {
        auto mode = status->getObject("mode");
        REQUIRE(mode);
        REQUIRE(mode->has("name"));
        REQUIRE(mode->has("id"));
        REQUIRE(mode->has("control"));

        // sid присутствует только если control == "sensor"
        if( mode->get("control").toString() == "sensor" )
            REQUIRE(mode->has("sid"));
    }

    // flat fields
    REQUIRE(status->has("maxHeartBeat"));
    REQUIRE(status->has("force"));
    REQUIRE(status->has("force_out"));
    REQUIRE(status->has("activateTimeout"));
    REQUIRE(status->has("reopenTimeout"));
    REQUIRE(status->has("notUseExchangeTimer"));

    if( status->has("httpEnabledSetParams") )
    {
        // значение проверяем как наличие поля; само значение зависит от конфигурации теста
        (void)status->get("httpEnabledSetParams");
    }

    // config_params block
    REQUIRE(status->has("config_params"));
    {
        auto cfg = status->getObject("config_params");
        REQUIRE(cfg);
        REQUIRE(cfg->has("recv_timeout"));
        REQUIRE(cfg->has("sleepPause_msec"));
        REQUIRE(cfg->has("polltime"));
        REQUIRE(cfg->has("default_timeout"));
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: HTTP /registers (basic)", "[http][rest][mbtcpmaster][registers]")
{
    InitTest();

    using Poco::Net::HTTPClientSession;
    using Poco::Net::HTTPRequest;
    using Poco::Net::HTTPResponse;

    HTTPClientSession cs(httpAddr, httpPort);
    HTTPRequest req(HTTPRequest::HTTP_GET, "/api/v2/MBTCPMaster1/registers", HTTPRequest::HTTP_1_1);
    HTTPResponse res;

    cs.sendRequest(req);
    std::istream& rs = cs.receiveResponse(res);
    REQUIRE(res.getStatus() == HTTPResponse::HTTP_OK);

    std::stringstream ss;
    ss << rs.rdbuf();

    Poco::JSON::Parser parser;
    auto parsed = parser.parse(ss.str());
    Poco::JSON::Object::Ptr root = parsed.extract<Poco::JSON::Object::Ptr>();
    REQUIRE(root);
    REQUIRE(root->get("result").toString() == "OK");
    REQUIRE(root->has("registers"));
    REQUIRE(root->has("devices"));
    REQUIRE(root->has("total"));
    REQUIRE(root->has("count"));
    REQUIRE(root->has("offset"));
    REQUIRE(root->has("limit"));

    auto registers = root->getArray("registers");
    REQUIRE(registers);

    auto devices = root->getObject("devices");
    REQUIRE(devices);

    // Проверяем структуру первого регистра (если есть)
    if(registers->size() > 0)
    {
        auto reg = registers->getObject(0);
        REQUIRE(reg);
        REQUIRE(reg->has("id"));
        REQUIRE(reg->has("name"));
        REQUIRE(reg->has("iotype"));
        REQUIRE(reg->has("value"));
        REQUIRE(reg->has("device"));
        REQUIRE(reg->has("register"));

        // device is now just addr (int), details in devices dict
        int deviceAddr = reg->getValue<int>("device");
        REQUIRE(deviceAddr > 0);

        // Check device details in devices dictionary
        std::string addrKey = std::to_string(deviceAddr);
        REQUIRE(devices->has(addrKey));
        auto deviceInfo = devices->getObject(addrKey);
        REQUIRE(deviceInfo);
        REQUIRE(deviceInfo->has("respond"));
        REQUIRE(deviceInfo->has("dtype"));

        auto regInfo = reg->getObject("register");
        REQUIRE(regInfo);
        REQUIRE(regInfo->has("mbreg"));
        REQUIRE(regInfo->has("mbfunc"));
        REQUIRE(regInfo->has("mbval"));
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: HTTP /registers (pagination)", "[http][rest][mbtcpmaster][registers][pagination]")
{
    InitTest();

    using Poco::Net::HTTPClientSession;
    using Poco::Net::HTTPRequest;
    using Poco::Net::HTTPResponse;

    HTTPClientSession cs(httpAddr, httpPort);
    Poco::JSON::Parser parser;

    // Сначала получаем общее количество
    int total = 0;
    {
        HTTPRequest req(HTTPRequest::HTTP_GET, "/api/v2/MBTCPMaster1/registers", HTTPRequest::HTTP_1_1);
        HTTPResponse res;
        cs.sendRequest(req);
        std::istream& rs = cs.receiveResponse(res);
        REQUIRE(res.getStatus() == HTTPResponse::HTTP_OK);

        std::stringstream ss;
        ss << rs.rdbuf();
        auto parsed = parser.parse(ss.str());
        Poco::JSON::Object::Ptr root = parsed.extract<Poco::JSON::Object::Ptr>();
        total = root->getValue<int>("total");
    }

    // Теперь проверяем пагинацию с limit=2
    if(total > 2)
    {
        HTTPRequest req(HTTPRequest::HTTP_GET, "/api/v2/MBTCPMaster1/registers?offset=0&limit=2", HTTPRequest::HTTP_1_1);
        HTTPResponse res;
        cs.sendRequest(req);
        std::istream& rs = cs.receiveResponse(res);
        REQUIRE(res.getStatus() == HTTPResponse::HTTP_OK);

        std::stringstream ss;
        ss << rs.rdbuf();
        auto parsed = parser.parse(ss.str());
        Poco::JSON::Object::Ptr root = parsed.extract<Poco::JSON::Object::Ptr>();

        REQUIRE(root->getValue<int>("count") == 2);
        REQUIRE(root->getValue<int>("limit") == 2);
        REQUIRE(root->getValue<int>("offset") == 0);
        REQUIRE(root->getValue<int>("total") == total);

        auto registers = root->getArray("registers");
        REQUIRE(registers->size() == 2);
    }

    // Проверяем offset
    if(total > 3)
    {
        HTTPRequest req(HTTPRequest::HTTP_GET, "/api/v2/MBTCPMaster1/registers?offset=1&limit=2", HTTPRequest::HTTP_1_1);
        HTTPResponse res;
        cs.sendRequest(req);
        std::istream& rs = cs.receiveResponse(res);
        REQUIRE(res.getStatus() == HTTPResponse::HTTP_OK);

        std::stringstream ss;
        ss << rs.rdbuf();
        auto parsed = parser.parse(ss.str());
        Poco::JSON::Object::Ptr root = parsed.extract<Poco::JSON::Object::Ptr>();

        REQUIRE(root->getValue<int>("offset") == 1);
        REQUIRE(root->getValue<int>("count") <= 2);
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: HTTP /registers (filter by iotype)", "[http][rest][mbtcpmaster][registers][filter]")
{
    InitTest();

    using Poco::Net::HTTPClientSession;
    using Poco::Net::HTTPRequest;
    using Poco::Net::HTTPResponse;

    HTTPClientSession cs(httpAddr, httpPort);
    Poco::JSON::Parser parser;

    // Фильтруем по типу AI
    {
        HTTPRequest req(HTTPRequest::HTTP_GET, "/api/v2/MBTCPMaster1/registers?iotype=AI", HTTPRequest::HTTP_1_1);
        HTTPResponse res;
        cs.sendRequest(req);
        std::istream& rs = cs.receiveResponse(res);
        REQUIRE(res.getStatus() == HTTPResponse::HTTP_OK);

        std::stringstream ss;
        ss << rs.rdbuf();
        auto parsed = parser.parse(ss.str());
        Poco::JSON::Object::Ptr root = parsed.extract<Poco::JSON::Object::Ptr>();
        REQUIRE(root->get("result").toString() == "OK");

        auto registers = root->getArray("registers");
        // Все записи должны быть типа AI
        for(size_t i = 0; i < registers->size(); ++i)
        {
            auto reg = registers->getObject(i);
            REQUIRE(reg->getValue<std::string>("iotype") == "AI");
        }
    }

    // Фильтруем по типу DI
    {
        HTTPRequest req(HTTPRequest::HTTP_GET, "/api/v2/MBTCPMaster1/registers?iotype=DI", HTTPRequest::HTTP_1_1);
        HTTPResponse res;
        cs.sendRequest(req);
        std::istream& rs = cs.receiveResponse(res);
        REQUIRE(res.getStatus() == HTTPResponse::HTTP_OK);

        std::stringstream ss;
        ss << rs.rdbuf();
        auto parsed = parser.parse(ss.str());
        Poco::JSON::Object::Ptr root = parsed.extract<Poco::JSON::Object::Ptr>();
        REQUIRE(root->get("result").toString() == "OK");

        auto registers = root->getArray("registers");
        for(size_t i = 0; i < registers->size(); ++i)
        {
            auto reg = registers->getObject(i);
            REQUIRE(reg->getValue<std::string>("iotype") == "DI");
        }
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: HTTP /devices", "[http][rest][mbtcpmaster][devices]")
{
    InitTest();

    using Poco::Net::HTTPClientSession;
    using Poco::Net::HTTPRequest;
    using Poco::Net::HTTPResponse;

    HTTPClientSession cs(httpAddr, httpPort);
    HTTPRequest req(HTTPRequest::HTTP_GET, "/api/v2/MBTCPMaster1/devices", HTTPRequest::HTTP_1_1);
    HTTPResponse res;

    cs.sendRequest(req);
    std::istream& rs = cs.receiveResponse(res);
    REQUIRE(res.getStatus() == HTTPResponse::HTTP_OK);

    std::stringstream ss;
    ss << rs.rdbuf();

    Poco::JSON::Parser parser;
    auto parsed = parser.parse(ss.str());
    Poco::JSON::Object::Ptr root = parsed.extract<Poco::JSON::Object::Ptr>();
    REQUIRE(root);
    REQUIRE(root->get("result").toString() == "OK");
    REQUIRE(root->has("devices"));
    REQUIRE(root->has("count"));

    auto devices = root->getArray("devices");
    REQUIRE(devices);

    int count = root->getValue<int>("count");
    REQUIRE(devices->size() == (size_t)count);

    // Проверяем структуру первого устройства (если есть)
    if(devices->size() > 0)
    {
        auto dev = devices->getObject(0);
        REQUIRE(dev);
        REQUIRE(dev->has("addr"));
        REQUIRE(dev->has("respond"));
        REQUIRE(dev->has("dtype"));
        REQUIRE(dev->has("regCount"));
        REQUIRE(dev->has("mode"));
        REQUIRE(dev->has("safeMode"));
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: HTTP /info extensionType and transportType", "[http][rest][mbtcpmaster][extensionType]")
{
    InitTest();

    using Poco::Net::HTTPClientSession;
    using Poco::Net::HTTPRequest;
    using Poco::Net::HTTPResponse;

    HTTPClientSession cs(httpAddr, httpPort);
    HTTPRequest req(HTTPRequest::HTTP_GET, "/api/v2/MBTCPMaster1/", HTTPRequest::HTTP_1_1);
    HTTPResponse res;

    cs.sendRequest(req);
    std::istream& rs = cs.receiveResponse(res);
    REQUIRE(res.getStatus() == HTTPResponse::HTTP_OK);

    std::stringstream ss;
    ss << rs.rdbuf();

    Poco::JSON::Parser parser;
    auto parsed = parser.parse(ss.str());
    Poco::JSON::Object::Ptr root = parsed.extract<Poco::JSON::Object::Ptr>();
    REQUIRE(root);

    // httpGetMyInfo возвращает данные в поле "object"
    REQUIRE(root->has("object"));
    auto obj = root->getObject("object");
    REQUIRE(obj);

    // Проверяем extensionType
    REQUIRE(obj->has("extensionType"));
    REQUIRE(obj->get("extensionType").toString() == "ModbusMaster");

    // Проверяем transportType (для MBTCPMaster должен быть "tcp")
    REQUIRE(obj->has("transportType"));
    REQUIRE(obj->get("transportType").toString() == "tcp");
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: HTTP /status httpControl fields", "[http][rest][mbtcpmaster][httpControl]")
{
    InitTest();

    using Poco::Net::HTTPClientSession;
    using Poco::Net::HTTPRequest;
    using Poco::Net::HTTPResponse;

    HTTPClientSession cs(httpAddr, httpPort);
    HTTPRequest req(HTTPRequest::HTTP_GET, "/api/v2/MBTCPMaster1/status", HTTPRequest::HTTP_1_1);
    HTTPResponse res;

    cs.sendRequest(req);
    std::istream& rs = cs.receiveResponse(res);
    REQUIRE(res.getStatus() == HTTPResponse::HTTP_OK);

    std::stringstream ss;
    ss << rs.rdbuf();

    Poco::JSON::Parser parser;
    auto parsed = parser.parse(ss.str());
    Poco::JSON::Object::Ptr root = parsed.extract<Poco::JSON::Object::Ptr>();
    REQUIRE(root);
    REQUIRE(root->has("status"));

    auto status = root->getObject("status");
    REQUIRE(status);

    // Проверяем наличие httpControl полей
    REQUIRE(status->has("httpControlAllow"));
    REQUIRE(status->has("httpControlActive"));
    REQUIRE(status->has("httpEnabledSetParams"));

    // По умолчанию httpControlActive должен быть 0
    REQUIRE(status->getValue<int>("httpControlActive") == 0);
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: HTTP /takeControl and /releaseControl", "[http][rest][mbtcpmaster][httpControl][takeRelease]")
{
    InitTest();

    using Poco::Net::HTTPClientSession;
    using Poco::Net::HTTPRequest;
    using Poco::Net::HTTPResponse;

    HTTPClientSession cs(httpAddr, httpPort);
    Poco::JSON::Parser parser;

    // 1) Пытаемся взять контроль (httpControlAllow=0 в тесте, должен вернуть ошибку)
    {
        HTTPRequest req(HTTPRequest::HTTP_GET, "/api/v2/MBTCPMaster1/takeControl", HTTPRequest::HTTP_1_1);
        HTTPResponse res;
        cs.sendRequest(req);
        std::istream& rs = cs.receiveResponse(res);
        REQUIRE(res.getStatus() == HTTPResponse::HTTP_OK);

        std::stringstream ss;
        ss << rs.rdbuf();
        auto parsed = parser.parse(ss.str());
        Poco::JSON::Object::Ptr root = parsed.extract<Poco::JSON::Object::Ptr>();
        REQUIRE(root);
        REQUIRE(root->has("result"));
        // httpControlAllow=0 в тесте, поэтому result == "ERROR"
        REQUIRE(root->get("result").toString() == "ERROR");
        REQUIRE(root->has("error"));
    }

    // 2) releaseControl всегда работает
    {
        HTTPRequest req(HTTPRequest::HTTP_GET, "/api/v2/MBTCPMaster1/releaseControl", HTTPRequest::HTTP_1_1);
        HTTPResponse res;
        cs.sendRequest(req);
        std::istream& rs = cs.receiveResponse(res);
        REQUIRE(res.getStatus() == HTTPResponse::HTTP_OK);

        std::stringstream ss;
        ss << rs.rdbuf();
        auto parsed = parser.parse(ss.str());
        Poco::JSON::Object::Ptr root = parsed.extract<Poco::JSON::Object::Ptr>();
        REQUIRE(root);
        REQUIRE(root->has("result"));
        REQUIRE(root->get("result").toString() == "OK");
        REQUIRE(root->has("httpControlActive"));
        REQUIRE(root->getValue<int>("httpControlActive") == 0);
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: HTTP /registers (filter by id/name)", "[http][rest][mbtcpmaster][registers][filter]")
{
    InitTest();

    using Poco::Net::HTTPClientSession;
    using Poco::Net::HTTPRequest;
    using Poco::Net::HTTPResponse;

    HTTPClientSession cs(httpAddr, httpPort);
    Poco::JSON::Parser parser;

    // Сначала получаем список всех регистров чтобы узнать реальные ID и имена
    std::vector<int> allIds;
    std::vector<std::string> allNames;
    {
        HTTPRequest req(HTTPRequest::HTTP_GET, "/api/v2/MBTCPMaster1/registers?limit=0", HTTPRequest::HTTP_1_1);
        HTTPResponse res;
        cs.sendRequest(req);
        std::istream& rs = cs.receiveResponse(res);
        REQUIRE(res.getStatus() == HTTPResponse::HTTP_OK);

        std::stringstream ss;
        ss << rs.rdbuf();
        auto parsed = parser.parse(ss.str());
        Poco::JSON::Object::Ptr root = parsed.extract<Poco::JSON::Object::Ptr>();
        REQUIRE(root);

        auto registers = root->getArray("registers");
        REQUIRE(registers);
        REQUIRE(registers->size() >= 2);

        for(size_t i = 0; i < registers->size() && i < 3; i++)
        {
            auto reg = registers->getObject(i);
            allIds.push_back(reg->getValue<int>("id"));
            allNames.push_back(reg->getValue<std::string>("name"));
        }
    }

    // Запрос с filter по двум ID
    REQUIRE(allIds.size() >= 2);
    std::string filterParam = std::to_string(allIds[0]) + "," + std::to_string(allIds[1]);

    {
        HTTPRequest req(HTTPRequest::HTTP_GET, "/api/v2/MBTCPMaster1/registers?filter=" + filterParam, HTTPRequest::HTTP_1_1);
        HTTPResponse res;
        cs.sendRequest(req);
        std::istream& rs = cs.receiveResponse(res);
        REQUIRE(res.getStatus() == HTTPResponse::HTTP_OK);

        std::stringstream ss;
        ss << rs.rdbuf();
        auto parsed = parser.parse(ss.str());
        Poco::JSON::Object::Ptr root = parsed.extract<Poco::JSON::Object::Ptr>();
        REQUIRE(root);
        REQUIRE(root->get("result").toString() == "OK");

        auto registers = root->getArray("registers");
        REQUIRE(registers);
        REQUIRE(registers->size() == 2);

        // Проверяем что вернулись именно запрошенные ID
        std::set<int> returnedIds;
        for(size_t i = 0; i < registers->size(); i++)
        {
            auto reg = registers->getObject(i);
            returnedIds.insert(reg->getValue<int>("id"));
        }

        REQUIRE(returnedIds.count(allIds[0]) == 1);
        REQUIRE(returnedIds.count(allIds[1]) == 1);
    }

    // Запрос с filter по имени (mixed id/name)
    REQUIRE(allNames.size() >= 2);
    {
        // Смешанный фильтр: первый по ID, второй по имени
        std::string mixedFilter = std::to_string(allIds[0]) + "," + allNames[1];
        HTTPRequest req(HTTPRequest::HTTP_GET, "/api/v2/MBTCPMaster1/registers?filter=" + mixedFilter, HTTPRequest::HTTP_1_1);
        HTTPResponse res;
        cs.sendRequest(req);
        std::istream& rs = cs.receiveResponse(res);
        REQUIRE(res.getStatus() == HTTPResponse::HTTP_OK);

        std::stringstream ss;
        ss << rs.rdbuf();
        auto parsed = parser.parse(ss.str());
        Poco::JSON::Object::Ptr root = parsed.extract<Poco::JSON::Object::Ptr>();
        REQUIRE(root);

        auto registers = root->getArray("registers");
        REQUIRE(registers);
        REQUIRE(registers->size() == 2);
    }

    // Проверяем запрос с одним ID
    {
        HTTPRequest req(HTTPRequest::HTTP_GET, "/api/v2/MBTCPMaster1/registers?filter=" + std::to_string(allIds[0]), HTTPRequest::HTTP_1_1);
        HTTPResponse res;
        cs.sendRequest(req);
        std::istream& rs = cs.receiveResponse(res);
        REQUIRE(res.getStatus() == HTTPResponse::HTTP_OK);

        std::stringstream ss;
        ss << rs.rdbuf();
        auto parsed = parser.parse(ss.str());
        Poco::JSON::Object::Ptr root = parsed.extract<Poco::JSON::Object::Ptr>();
        REQUIRE(root);

        auto registers = root->getArray("registers");
        REQUIRE(registers);
        REQUIRE(registers->size() == 1);
        REQUIRE(registers->getObject(0)->getValue<int>("id") == allIds[0]);
    }

    // Проверяем запрос с несуществующим ID
    {
        HTTPRequest req(HTTPRequest::HTTP_GET, "/api/v2/MBTCPMaster1/registers?filter=999999999", HTTPRequest::HTTP_1_1);
        HTTPResponse res;
        cs.sendRequest(req);
        std::istream& rs = cs.receiveResponse(res);
        REQUIRE(res.getStatus() == HTTPResponse::HTTP_OK);

        std::stringstream ss;
        ss << rs.rdbuf();
        auto parsed = parser.parse(ss.str());
        Poco::JSON::Object::Ptr root = parsed.extract<Poco::JSON::Object::Ptr>();
        REQUIRE(root);

        auto registers = root->getArray("registers");
        REQUIRE(registers);
        REQUIRE(registers->size() == 0);
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: HTTP /get endpoint", "[http][rest][mbtcpmaster][get]")
{
    InitTest();

    using Poco::Net::HTTPClientSession;
    using Poco::Net::HTTPRequest;
    using Poco::Net::HTTPResponse;

    HTTPClientSession cs(httpAddr, httpPort);
    Poco::JSON::Parser parser;

    // Сначала получаем список всех регистров
    std::vector<int> allIds;
    std::vector<std::string> allNames;
    {
        HTTPRequest req(HTTPRequest::HTTP_GET, "/api/v2/MBTCPMaster1/registers?limit=3", HTTPRequest::HTTP_1_1);
        HTTPResponse res;
        cs.sendRequest(req);
        std::istream& rs = cs.receiveResponse(res);
        REQUIRE(res.getStatus() == HTTPResponse::HTTP_OK);

        std::stringstream ss;
        ss << rs.rdbuf();
        auto parsed = parser.parse(ss.str());
        Poco::JSON::Object::Ptr root = parsed.extract<Poco::JSON::Object::Ptr>();
        REQUIRE(root);

        auto registers = root->getArray("registers");
        REQUIRE(registers);
        REQUIRE(registers->size() >= 2);

        for(size_t i = 0; i < registers->size(); i++)
        {
            auto reg = registers->getObject(i);
            allIds.push_back(reg->getValue<int>("id"));
            allNames.push_back(reg->getValue<std::string>("name"));
        }
    }

    // Тест /get с filter по ID
    {
        std::string filterParam = std::to_string(allIds[0]) + "," + std::to_string(allIds[1]);
        HTTPRequest req(HTTPRequest::HTTP_GET, "/api/v2/MBTCPMaster1/get?filter=" + filterParam, HTTPRequest::HTTP_1_1);
        HTTPResponse res;
        cs.sendRequest(req);
        std::istream& rs = cs.receiveResponse(res);
        REQUIRE(res.getStatus() == HTTPResponse::HTTP_OK);

        std::stringstream ss;
        ss << rs.rdbuf();
        auto parsed = parser.parse(ss.str());
        Poco::JSON::Object::Ptr root = parsed.extract<Poco::JSON::Object::Ptr>();
        REQUIRE(root);
        REQUIRE(root->has("sensors"));

        auto sensors = root->getArray("sensors");
        REQUIRE(sensors);
        REQUIRE(sensors->size() == 2);

        // Проверяем структуру ответа
        auto sensor = sensors->getObject(0);
        REQUIRE(sensor->has("id"));
        REQUIRE(sensor->has("name"));
        REQUIRE(sensor->has("value"));
        REQUIRE(sensor->has("iotype"));
    }

    // Тест /get с filter по имени
    {
        HTTPRequest req(HTTPRequest::HTTP_GET, "/api/v2/MBTCPMaster1/get?filter=" + allNames[0], HTTPRequest::HTTP_1_1);
        HTTPResponse res;
        cs.sendRequest(req);
        std::istream& rs = cs.receiveResponse(res);
        REQUIRE(res.getStatus() == HTTPResponse::HTTP_OK);

        std::stringstream ss;
        ss << rs.rdbuf();
        auto parsed = parser.parse(ss.str());
        Poco::JSON::Object::Ptr root = parsed.extract<Poco::JSON::Object::Ptr>();
        REQUIRE(root);

        auto sensors = root->getArray("sensors");
        REQUIRE(sensors);
        REQUIRE(sensors->size() == 1);
        REQUIRE(sensors->getObject(0)->getValue<std::string>("name") == allNames[0]);
    }

    // Тест /get без filter - должен вернуть ошибку
    {
        HTTPRequest req(HTTPRequest::HTTP_GET, "/api/v2/MBTCPMaster1/get", HTTPRequest::HTTP_1_1);
        HTTPResponse res;
        cs.sendRequest(req);
        std::istream& rs = cs.receiveResponse(res);
        REQUIRE(res.getStatus() == HTTPResponse::HTTP_OK);

        std::stringstream ss;
        ss << rs.rdbuf();
        auto parsed = parser.parse(ss.str());
        Poco::JSON::Object::Ptr root = parsed.extract<Poco::JSON::Object::Ptr>();
        REQUIRE(root);
        REQUIRE(root->has("error"));
    }

    // Тест /get с несуществующим сенсором - должен вернуть error в элементе
    {
        HTTPRequest req(HTTPRequest::HTTP_GET, "/api/v2/MBTCPMaster1/get?filter=NonExistentSensor123", HTTPRequest::HTTP_1_1);
        HTTPResponse res;
        cs.sendRequest(req);
        std::istream& rs = cs.receiveResponse(res);
        REQUIRE(res.getStatus() == HTTPResponse::HTTP_OK);

        std::stringstream ss;
        ss << rs.rdbuf();
        auto parsed = parser.parse(ss.str());
        Poco::JSON::Object::Ptr root = parsed.extract<Poco::JSON::Object::Ptr>();
        REQUIRE(root);

        auto sensors = root->getArray("sensors");
        REQUIRE(sensors);
        REQUIRE(sensors->size() == 1);
        REQUIRE(sensors->getObject(0)->has("error"));
    }
}
// -----------------------------------------------------------------------------
#endif // DISABLE_REST_API
