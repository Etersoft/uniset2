#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <limits>
#include <memory>
#include "UniSetTypes.h"
#include "UInterface.h"
#include "client.h"
#include "OPCUAServer.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// -----------------------------------------------------------------------------
static string addr  = "opc.tcp://127.0.0.1:44999";
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
TEST_CASE("[OPCUAServer]: call", "[opcuaserver]")
{
    InitTest();

    REQUIRE( ui->getValue(1) == 0 );
    REQUIRE( opcuaCallInt32(nodeId, "LocalhostNode", "AI_S", 77) == UA_STATUSCODE_GOOD );
    msleep(pause_msec);
    REQUIRE( ui->getValue(1) == 77 );

    REQUIRE( ui->getValue(2) == 0 );
    REQUIRE( opcuaCallBool(nodeId, "LocalhostNode", "DI_S", true) == UA_STATUSCODE_GOOD );
    msleep(pause_msec);
    REQUIRE( ui->getValue(2) == 1 );

    REQUIRE( ui->getValue(3) == 0 );
    REQUIRE( opcuaCallFloat(nodeId, "LocalhostNode", "FAI_S", 37.83) == UA_STATUSCODE_GOOD );
    msleep(pause_msec);
    REQUIRE( ui->getValue(3) == 3783 );

    REQUIRE_FALSE( opcuaCallInt32(nodeId, "LocalhostNode", "FAI_S", 54) == UA_STATUSCODE_GOOD );
    REQUIRE( ui->getValue(3) == 3783 );
    REQUIRE_FALSE( opcuaCallFloat(nodeId, "LocalhostNode", "DI_S", 37.83) == UA_STATUSCODE_GOOD );
    REQUIRE( ui->getValue(2) == 1 );
    REQUIRE_FALSE( opcuaCallBool(nodeId, "LocalhostNode", "AI_S", true) == UA_STATUSCODE_GOOD );
    REQUIRE( ui->getValue(1) == 77 );
}
// -----------------------------------------------------------------------------
