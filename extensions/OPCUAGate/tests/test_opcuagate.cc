#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <limits>
#include <memory>
#include "UniSetTypes.h"
#include "UInterface.h"
#include "client.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// -----------------------------------------------------------------------------
static string addr  = "opc.tcp://localhost:44999";
static std::shared_ptr<UInterface> ui;
static uint16_t nodeId = 0;
static int pause_msec = 200;
// -----------------------------------------------------------------------------
static void InitTest()
{
    auto conf = uniset_conf();
    CHECK( conf != nullptr );

    if( !ui )
    {
        ui = std::make_shared<UInterface>();
        // UI понадобиться для проверки записанных в SM значений.
        CHECK( ui->getObjectIndex() != nullptr );
        CHECK( ui->getConf() == conf );
    }

    REQUIRE( opcuaCreateClient(addr) );
}
// -----------------------------------------------------------------------------
TEST_CASE("[OPCUAGate]: read", "[opcuagate]")
{
    InitTest();

    ui->setValue(2, 100);
    msleep(pause_msec);
    REQUIRE( opcuaRead(nodeId, "AI1_S") == 100 );

    ui->setValue(2, -100);
    msleep(pause_msec);
    REQUIRE( opcuaRead(nodeId, "AI1_S") == -100 );


    ui->setValue(1, 1);
    msleep(pause_msec);
    REQUIRE( opcuaReadBool(nodeId, "DI1_S") );

    ui->setValue(1, 0);
    msleep(pause_msec);
    REQUIRE_FALSE( opcuaReadBool(nodeId, "DI1_S") );

}
// -----------------------------------------------------------------------------
TEST_CASE("[OPCUAGate]: write", "[opcuagate]")
{
    InitTest();

    REQUIRE( ui->getValue(3) == 0 );
    REQUIRE( opcuaWrite(nodeId, "AO1_S", 10500) );
    msleep(pause_msec);
    REQUIRE( ui->getValue(3) == 10500 );

    REQUIRE( opcuaWriteBool(nodeId, "DO1_S", true) );
    msleep(pause_msec);
    REQUIRE( ui->getValue(4) == 1 );

    REQUIRE( opcuaWriteBool(nodeId, "DO1_S", false) );
    msleep(pause_msec);
    REQUIRE( ui->getValue(4) == 0 );
}
// -----------------------------------------------------------------------------
