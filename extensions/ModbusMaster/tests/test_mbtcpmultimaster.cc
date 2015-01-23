#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <time.h>
#include <limits>
#include "UniSetTypes.h"
#include "MBTCPMultiMaster.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
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
using namespace UniSetTypes;
// -----------------------------------------------------------------------------
static ModbusRTU::ModbusAddr slaveaddr = 0x01; // conf->getArgInt("--mbs-my-addr");
static int port = 20050; // conf->getArgInt("--mbs-inet-port");
static string addr("127.0.0.1"); // conf->getArgParam("--mbs-inet-addr");
static int port2 = 20052;
static string addr2("127.0.0.1");
static ModbusRTU::ModbusAddr slaveADDR = 0x01;
static shared_ptr<MBTCPTestServer> mbs1;
static shared_ptr<MBTCPTestServer> mbs2;
static shared_ptr<UInterface> ui;
static ObjectId mbID = 6005; // MBTCPMultiMaster1
static int polltime=50; // conf->getArgInt("--mbtcp-polltime");
static ObjectId slaveNotRespond = 10; // Slave_Not_Respond_S
static ObjectId slave1NotRespond = 12; // Slave1_Not_Respond_S
static ObjectId slave2NotRespond = 13; // Slave2_Not_Respond_S
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

    if( !mbs1 )
    {
        mbs1 = make_shared<MBTCPTestServer>(slaveADDR,addr,port,false);
        CHECK( mbs1!= nullptr );
        mbs1->setReply(0);
        mbs1->runThread();
        for( int i=0; !mbs1->isRunning() && i<10; i++ )
            msleep(200);
        CHECK( mbs1->isRunning() );
        msleep(7000);
        CHECK( ui->getValue(slaveNotRespond) == 0 );
    }

    if( !mbs2 )
    {
        mbs2 = make_shared<MBTCPTestServer>(slaveADDR,addr2,port2,false);
        CHECK( mbs2!= nullptr );
        mbs2->setReply(0);
        mbs2->runThread();
        for( int i=0; !mbs2->isRunning() && i<10; i++ )
            msleep(200);
        CHECK( mbs2->isRunning() );
    }

}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMultiMaster: rotate channel","[modbus][mbmaster][mbtcpmultimaster]")
{
    InitTest();
    CHECK( ui->isExist(mbID) );

    REQUIRE( ui->getValue(1003) == 0 );
    mbs1->setReply(100);
    mbs2->setReply(10);
    msleep(polltime+1000);
    REQUIRE( ui->getValue(1003) == 100 );
    mbs1->disableExchange(true);
    msleep(4000); // --mbtcp-timeout 3000 (см. run_test_mbtcmultipmaster.sh)
    REQUIRE( ui->getValue(1003) == 10 );
    mbs1->disableExchange(false);
    mbs2->disableExchange(true);
    msleep(4000); // --mbtcp-timeout 3000 (см. run_test_mbtcmultipmaster.sh)
    REQUIRE( ui->getValue(1003) == 100 );
    mbs2->disableExchange(false);
    REQUIRE( ui->getValue(1003) == 100 );
}
// -----------------------------------------------------------------------------
