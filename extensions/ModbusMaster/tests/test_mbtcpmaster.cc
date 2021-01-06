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

    CHECK( ui->isExist(mbID) );
    mbs->setReply(65535);
    msleep(polltime + 200);
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

    CHECK( ui->isExist(mbID) );
    ui->setValue(1018, 0);
    REQUIRE( ui->getValue(1018) == 0 );
    msleep(polltime + 200);
    REQUIRE( mbs->getLastWriteOutputSingleRegister() == 0 );

    ui->setValue(1018, 100);
    REQUIRE( ui->getValue(1018) == 100 );
    msleep(polltime + 200);
    REQUIRE( mbs->getLastWriteOutputSingleRegister() == 100 );

    ui->setValue(1018, -100);
    REQUIRE( ui->getValue(1018) == -100 );
    msleep(polltime + 200);
    REQUIRE( mbs->getLastWriteOutputSingleRegister() == -100 );

    ui->setValue(1018, 0);
    REQUIRE( ui->getValue(1018) == 0 );
    msleep(polltime + 200);
    REQUIRE( mbs->getLastWriteOutputSingleRegister() == 0 );
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: 0x0F (force multiple coils)", "[modbus][0x0F][mbmaster][mbtcpmaster]")
{
    InitTest();

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
TEST_CASE("MBTCPMaster: exchangeMode", "[modbus][exchangemode][mbmaster][mbtcpmaster]")
{
    InitTest();

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
            REQUIRE( mbs->getLastWriteOutputSingleRegister() == 10 );
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
            REQUIRE( mbs->getLastWriteOutputSingleRegister() == 150 );
            ui->setValue(1018, 155);
            REQUIRE( ui->getValue(1018) == 155 );
            msleep(polltime + 200);
            REQUIRE( mbs->getLastWriteOutputSingleRegister() == 155 );
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
            REQUIRE( mbs->getLastWriteOutputSingleRegister() != 50 );
            ui->setValue(1018, 55);
            REQUIRE( ui->getValue(1018) == 55 );
            msleep(2 * polltime + 200);
            REQUIRE( mbs->getLastWriteOutputSingleRegister() != 55 );
            REQUIRE( mbs->getLastWriteOutputSingleRegister() != 50 );
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
            REQUIRE( mbs->getLastWriteOutputSingleRegister() == 60 );
            ui->setValue(1018, 65);
            REQUIRE( ui->getValue(1018) == 65 );
            msleep(polltime + 200);
            REQUIRE( mbs->getLastWriteOutputSingleRegister() == 65 );
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
            REQUIRE( mbs->getLastWriteOutputSingleRegister() != 70 );
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
TEST_CASE("MBTCPMaster: check respond resnsor", "[modbus][respond][mbmaster][mbtcpmaster]")
{
    InitTest();
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
TEST_CASE("MBTCPMaster: udefined value", "[modbus][undefined][mbmaster][mbtcpmaster]")
{
    InitTest();

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

    // default reconfigure
    std::string request = "/api/v01/reload";
    uniset::SimpleInfo_var ret = mbm->apiRequest(request.c_str());

    ostringstream sinfo;
    sinfo << ret->info;
    std::string info = sinfo.str();

    REQUIRE( ret->id == mbm->getId() );
    REQUIRE_FALSE( info.empty() );
    REQUIRE( info.find("OK") != std::string::npos );


    // reconfigure from other file
    request = "/api/v01/reload?confile=" + confile2;
    ret = mbm->apiRequest(request.c_str());

    sinfo.str("");
    sinfo << ret->info;
    info = sinfo.str();

    REQUIRE( ret->id == mbm->getId() );
    REQUIRE_FALSE( info.empty() );
    REQUIRE( info.find("OK") != std::string::npos );
    REQUIRE( info.find(confile2) != std::string::npos );

    // reconfigure FAIL
    request = "/api/v01/reload?confile=BADFILE";
    ret = mbm->apiRequest(request.c_str());
    sinfo.str("");
    sinfo << ret->info;
    info = sinfo.str();

    REQUIRE( ret->id == mbm->getId() );
    REQUIRE_FALSE( info.empty() );
    REQUIRE( info.find("OK") == std::string::npos );
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
