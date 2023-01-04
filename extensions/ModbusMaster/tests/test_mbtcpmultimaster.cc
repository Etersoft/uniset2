#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <time.h>
#include <limits>
#include <unordered_set>
#include <Poco/Net/NetException.h>
#include "UniSetTypes.h"
#include "MBTCPMultiMaster.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// -----------------------------------------------------------------------------
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
using namespace uniset;
// -----------------------------------------------------------------------------
static ModbusRTU::ModbusAddr slaveADDR = 0x01; // conf->getArgInt("--mbs-my-addr");
static unordered_set<ModbusRTU::ModbusAddr> vaddr = { slaveADDR, 0x02, 0x03 };
static int port = 20053; // conf->getArgInt("--mbs-inet-port");
static const string iaddr("127.0.0.1"); // conf->getArgParam("--mbs-inet-addr");
static int port2 = 20055;
static const string iaddr2("127.0.0.1");
static shared_ptr<MBTCPTestServer> mbs1;
static shared_ptr<MBTCPTestServer> mbs2;
static shared_ptr<UInterface> ui;
static ObjectId mbID = 6005; // MBTCPMultiMaster1
static int polltime = 100; // conf->getArgInt("--mbtcp-polltime");
static ObjectId slaveNotRespond = 10; // Slave_Not_Respond_S
static ObjectId slave1NotRespond = 12; // Slave1_Not_Respond_S
static ObjectId slave2NotRespond = 13; // Slave2_Not_Respond_S
static const ObjectId exchangeMode = 11; // MBTCPMaster_Mode_AS
static const string confile2 = "mbmaster-test-configure2.xml";
// -----------------------------------------------------------------------------
extern std::shared_ptr<MBTCPMultiMaster> mbmm;
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

    if( !mbs1 )
    {
        try
        {
            mbs1 = make_shared<MBTCPTestServer>(vaddr, iaddr, port, false);
        }
        catch( const Poco::Net::NetException& e )
        {
            ostringstream err;
            err << "(mb1): Can`t create socket " << iaddr << ":" << port << " err: " << e.message() << endl;
            cerr << err.str() << endl;
            throw SystemError(err.str());
        }
        catch( const std::exception& ex )
        {
            cerr << "(mb1): Can`t create socket " << iaddr << ":" << port << " err: " << ex.what() << endl;
            throw;
        }

        CHECK( mbs1 != nullptr );
        mbs1->setReply(0);
        mbs1->execute();

        for( int i = 0; !mbs1->isRunning() && i < 10; i++ )
            msleep(200);

        CHECK( mbs1->isRunning() );
        msleep(7200);
        CHECK( ui->getValue(slaveNotRespond) == 0 );
    }

    if( !mbs2 )
    {
        try
        {
            mbs2 = make_shared<MBTCPTestServer>(vaddr, iaddr2, port2, false);
        }
        catch( const Poco::Net::NetException& e )
        {
            ostringstream err;
            err << "(mb2): Can`t create socket " << iaddr << ":" << port << " err: " << e.message() << endl;
            cerr << err.str() << endl;
            throw SystemError(err.str());
        }
        catch( const std::exception& ex )
        {
            cerr << "(mb2): Can`t create socket " << iaddr << ":" << port << " err: " << ex.what() << endl;
            throw;
        }

        CHECK( mbs2 != nullptr );
        mbs2->setReply(0);
        mbs2->execute();

        for( int i = 0; !mbs2->isRunning() && i < 10; i++ )
            msleep(200);

        CHECK( mbs2->isRunning() );
    }

}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMultiMaster: rotate channel", "[modbus][mbmaster][mbtcpmultimaster]")
{
    // Т.к. respond/notrespond проверяется по возможности создать соединение
    // а мы имитируем отключение просто отключением обмена
    // то датчик связи всё-равно будет показывать что канал1 доступен
    // поэтому датчик 12 - не проверяем..
    // а просто проверяем что теперь значение приходит по другому каналу
    // (см. setReply)
    // ----------------------------

    InitTest();
    CHECK( ui->isExist(mbID) );

    mbs1->setReply(0);
    msleep(polltime + 1000);
    REQUIRE( ui->getValue(1003) == 0 );
    mbs1->setReply(100);
    mbs2->setReply(10);
    msleep(polltime + 1000);
    REQUIRE( ui->getValue(1003) == 100 );
    mbs1->disableExchange(true);
    mbs2->disableExchange(false);
    msleep(4000); // --mbtcp-timeout 3000 (см. run_test_mbtcmultipmaster.sh)
    REQUIRE( ui->getValue(1003) == 10 );

    // проверяем что канал остался на втором, хотя на первом мы включили связь
    mbs1->disableExchange(false);
    mbs2->disableExchange(false);
    msleep(4000); // --mbtcp-timeout 3000 (см. run_test_mbtcmultipmaster.sh)
    REQUIRE( ui->getValue(1003) == 10 );

    mbs2->disableExchange(true);
    mbs1->disableExchange(false);
    msleep(4000); // --mbtcp-timeout 3000 (см. run_test_mbtcmultipmaster.sh)
    REQUIRE( ui->getValue(1003) == 100 );
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMultiMaster: safe mode", "[modbus][safemode][mbmaster][mbtcpmultimaster]")
{
    InitTest();

    ui->setValue(1050, 0); // отключаем safeMode

    mbs1->setReply(53);
    msleep(polltime + 200);
    REQUIRE( ui->getValue(1051) == 53 );
    REQUIRE( ui->getValue(1052) == 1 );

    mbs1->setReply(0);
    msleep(polltime + 200);
    REQUIRE( ui->getValue(1051) == 0 );
    REQUIRE( ui->getValue(1052) == 0 );

    ui->setValue(1050, 42); // включаем safeMode
    msleep(polltime + 200);
    REQUIRE( ui->getValue(1051) == 42 );
    REQUIRE( ui->getValue(1052) == 1 );


    mbs1->setReply(53);
    msleep(polltime + 200);
    REQUIRE( ui->getValue(1051) == 42 );
    REQUIRE( ui->getValue(1052) == 1 );

    ui->setValue(1050, 0); // отключаем safeMode
    msleep(polltime + 200);
    REQUIRE( ui->getValue(1051) == 53 );
    REQUIRE( ui->getValue(1052) == 1 );
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMultiMaster: safe mode (resetIfNotRespond)", "[modbus][safemode][mbmaster][mbtcpmultimaster]")
{
    InitTest();

    mbs1->disableExchange(false); // включаем связь
    mbs2->disableExchange(false); // включаем связь
    msleep(2000);

    mbs1->setReply(53);
    mbs2->setReply(53);
    msleep(polltime + 200);
    REQUIRE( ui->getValue(1053) == 53 );
    REQUIRE( ui->getValue(1054) == 1 );

    mbs1->setReply(0);
    mbs2->setReply(0);
    msleep(polltime + 200);
    REQUIRE( ui->getValue(1053) == 0 );
    REQUIRE( ui->getValue(1054) == 0 );

    mbs1->disableExchange(true); // отключаем связь
    mbs2->disableExchange(true); // отключаем связь
    msleep(5000);
    REQUIRE( ui->getValue(1053) == 42 );
    REQUIRE( ui->getValue(1054) == 1 );

    mbs1->setReply(53);
    mbs2->setReply(53);
    msleep(polltime + 200);
    REQUIRE( ui->getValue(1053) == 42 );
    REQUIRE( ui->getValue(1054) == 1 );

    mbs1->disableExchange(false); // включаем связь
    mbs2->disableExchange(false); // включаем связь
    msleep(5000);
    REQUIRE( ui->getValue(1053) == 53 );
    REQUIRE( ui->getValue(1054) == 1 );
    mbs1->setReply(0);
    mbs2->setReply(0);
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMultiMaster: udefined value", "[modbus][undefined][mbmaster][mbtcpmultimaster]")
{
    InitTest();

    mbs1->setReply(120);
    mbs2->setReply(120);
    msleep(polltime + 200);
    REQUIRE( ui->getValue(1070) == 120 );

    mbs1->setReply(10);
    mbs2->setReply(10);
    msleep(polltime + 200);

    try
    {
        ui->getValue(1070);
    }
    catch( IOController_i::Undefined& ex )
    {
        REQUIRE( ex.value == 65535 );
    }

    mbs1->setReply(120);
    mbs2->setReply(120);
    msleep(polltime + 200);
    REQUIRE( ui->getValue(1070) == 120 );
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMultiMaster: reload config", "[modbus][reload][mbmaster][mbtcpmultimaster]")
{
    InitTest();

    mbs1->setReply(160);
    mbs2->setReply(160);
    msleep(polltime + 500);
    REQUIRE( ui->getValue(1070) == 160 );
    REQUIRE( ui->getValue(1080) == 1080 );

    // add new mbreg
    REQUIRE( mbmm->reload(confile2) );

    msleep(polltime + 600);
    REQUIRE( ui->getValue(1080) == 160 );
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMultiMaster: reload config (HTTP API)", "[modbus][reload-api][mbmaster][mbtcpmaster]")
{
    InitTest();

    // default reconfigure
    std::string request = "/api/v01/reload";
    uniset::SimpleInfo_var ret = mbmm->apiRequest(request.c_str());

    ostringstream sinfo;
    sinfo << ret->info;
    std::string info = sinfo.str();

    REQUIRE( ret->id == mbmm->getId() );
    REQUIRE_FALSE( info.empty() );
    REQUIRE( info.find("OK") != std::string::npos );


    // reconfigure from other file
    request = "/api/v01/reload?confile=" + confile2;
    ret = mbmm->apiRequest(request.c_str());

    sinfo.str("");
    sinfo << ret->info;
    info = sinfo.str();

    REQUIRE( ret->id == mbmm->getId() );
    REQUIRE_FALSE( info.empty() );
    REQUIRE( info.find("OK") != std::string::npos );
    REQUIRE( info.find(confile2) != std::string::npos );

    // reconfigure FAIL
    request = "/api/v01/reload?confile=BADFILE";
    ret = mbmm->apiRequest(request.c_str());
    sinfo.str("");
    sinfo << ret->info;
    info = sinfo.str();

    REQUIRE( ret->id == mbmm->getId() );
    REQUIRE_FALSE( info.empty() );
    REQUIRE( info.find("OK") == std::string::npos );
}
// -----------------------------------------------------------------------------
