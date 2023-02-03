#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <memory>
#include <unordered_set>
#include <limits>
#include "OPCUATestServer.h"
#include "OPCUAExchange.h"
#include "UniSetActivator.h"
#include "SMInterface.h"
#include "SharedMemory.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// -----------------------------------------------------------------------------
static shared_ptr<UInterface> ui;
static std::shared_ptr<SMInterface> smi;
// -----------------------------------------------------------------------------
extern std::shared_ptr<SharedMemory> shm;
extern std::shared_ptr<OPCUAExchange> opc;
extern shared_ptr<OPCUATestServer> opcTestServer;
// -----------------------------------------------------------------------------
const std::string rdAttr1 = "Attr1";
const std::string rdAttr2 = "Attr2";
const std::string wrAttr3 = "Attr3";
const std::string wrAttr4 = "Attr4";
const std::string ignAttr5 = "Attr5";
const int rdI100 = 100;
const ObjectId sidAttr1 = 1000;
const ObjectId sidAttr2 = 1001;
const ObjectId sidAttr3 = 1010;
const ObjectId sidAttr4 = 1011;
const ObjectId sidAttr5 = 1020;
const ObjectId sidAttrI100 = 1021;
const timeout_t step_pause_msec = 300;
// -----------------------------------------------------------------------------
static void InitTest()
{
    auto conf = uniset_conf();
    CHECK( conf != nullptr );

    if( !ui )
    {
        ui = make_shared<UInterface>();
        CHECK( ui->getObjectIndex() != nullptr );
        CHECK( ui->getConf() == conf );
    }

    if( !smi )
    {
        if( shm == nullptr )
            throw SystemError("SharedMemory don`t initialize..");

        if( ui == nullptr )
            throw SystemError("UInterface don`t initialize..");

        smi = make_shared<SMInterface>(shm->getId(), ui, shm->getId(), shm );
    }

    CHECK( opcTestServer != nullptr );
    CHECK( opcTestServer->isRunning() );

    REQUIRE( opc != nullptr );
}
// -----------------------------------------------------------------------------
TEST_CASE("OPCUAExchange: read", "[opcua][exchange][read]")
{
    InitTest();

    opcTestServer->setI32(rdAttr1, 10);
    opcTestServer->setBool(rdAttr2, true);

    REQUIRE(opcTestServer->getI32(rdAttr1) == 10 );
    REQUIRE(opcTestServer->getBool(rdAttr2) == true );

    msleep(step_pause_msec);
    REQUIRE(shm->getValue(sidAttr1) == 10);
    REQUIRE(shm->getValue(sidAttr2) == 1);

    opcTestServer->setI32(rdAttr1, 20);
    opcTestServer->setBool(rdAttr2, false);
    msleep(step_pause_msec);

    REQUIRE(opcTestServer->getI32(rdAttr1) == 20 );
    REQUIRE(opcTestServer->getBool(rdAttr2) == false );
}
// -----------------------------------------------------------------------------
TEST_CASE("OPCUAExchange: write", "[opcua][exchange][write]")
{
    InitTest();

    opcTestServer->setI32(wrAttr3, 100);
    opcTestServer->setBool(wrAttr4, false);

    REQUIRE(opcTestServer->getI32(wrAttr3) == 100 );
    REQUIRE(opcTestServer->getBool(wrAttr4) == false );

    REQUIRE_NOTHROW(shm->setValue(sidAttr3, 1000));
    REQUIRE_NOTHROW(shm->setValue(sidAttr4, 1));
    msleep(step_pause_msec);
    REQUIRE(opcTestServer->getI32(wrAttr3) == 1000);
    REQUIRE(opcTestServer->getBool(wrAttr4) == true);

    REQUIRE_NOTHROW(shm->setValue(sidAttr3, 20));
    REQUIRE_NOTHROW(shm->setValue(sidAttr4, 0));
    msleep(step_pause_msec);
    REQUIRE(opcTestServer->getI32(wrAttr3) == 20 );
    REQUIRE(opcTestServer->getBool(wrAttr4) == false);
}
// -----------------------------------------------------------------------------
TEST_CASE("OPCUAExchange: ignore", "[opcua][exchange][ignore]")
{
    InitTest();
    opcTestServer->setI32(ignAttr5, 100);
    REQUIRE(opcTestServer->getI32(ignAttr5) == 100 );
    REQUIRE( shm->getValue(sidAttr5) == 0 );
    msleep(step_pause_msec);
    REQUIRE( shm->getValue(sidAttr5) == 0 );

    opcTestServer->setI32(ignAttr5, 10);
    REQUIRE(opcTestServer->getI32(ignAttr5) == 10 );
    msleep(step_pause_msec);
    REQUIRE( shm->getValue(sidAttr5) == 0 );
}
// -----------------------------------------------------------------------------
TEST_CASE("OPCUAExchange: read numeric nodeId", "[opcua][exchange][readI]")
{
    InitTest();

    opcTestServer->setI32(rdI100, 10);
    REQUIRE(opcTestServer->getI32(rdI100) == 10 );
    msleep(step_pause_msec);
    REQUIRE(shm->getValue(sidAttrI100) == 10);

    opcTestServer->setI32(rdI100, 20);
    REQUIRE(opcTestServer->getI32(rdI100) == 20 );
    msleep(step_pause_msec);
    REQUIRE(shm->getValue(sidAttrI100) == 20);
}