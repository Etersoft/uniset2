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
extern shared_ptr<OPCUATestServer> opcTestServer1;
extern shared_ptr<OPCUATestServer> opcTestServer2;
// -----------------------------------------------------------------------------
const std::string rdAttr1 = "Attr1";
const std::string rdAttr2 = "Attr2";
const std::string wrAttr3 = "Attr3";
const std::string wrAttr4 = "Attr4";
const std::string ignAttr5 = "Attr5";
const int rdI101 = 101;
const ObjectId sidAttr1 = 1000;
const ObjectId sidAttr2 = 1001;
const ObjectId sidAttr3 = 1010;
const ObjectId sidAttr4 = 1011;
const ObjectId sidAttr5 = 1020;
const ObjectId sidAttrI101 = 1021;
const ObjectId sidRespond = 10;
const ObjectId sidRespond1 = 11;
const ObjectId sidRespond2 = 12;
const timeout_t step_pause_msec = 350;
const timeout_t timeout_msec = 6000;
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

    CHECK(opcTestServer1 != nullptr );
    CHECK(opcTestServer1->isRunning() );

    CHECK(opcTestServer2 != nullptr );
    CHECK(opcTestServer2->isRunning() );

    REQUIRE( opc != nullptr );
}
// -----------------------------------------------------------------------------
TEST_CASE("OPCUAExchange: read", "[opcua][exchange][read]")
{
    InitTest();

    opcTestServer1->setI32(rdAttr1, 10);
    opcTestServer1->setBool(rdAttr2, true);

    REQUIRE(opcTestServer1->getI32(rdAttr1) == 10 );
    REQUIRE(opcTestServer1->getBool(rdAttr2) == true );

    msleep(step_pause_msec);
    REQUIRE(shm->getValue(sidAttr1) == 10);
    REQUIRE(shm->getValue(sidAttr2) == 1);

    opcTestServer1->setI32(rdAttr1, 20);
    opcTestServer1->setBool(rdAttr2, false);
    msleep(step_pause_msec);

    REQUIRE(opcTestServer1->getI32(rdAttr1) == 20 );
    REQUIRE(opcTestServer1->getBool(rdAttr2) == false );
}
// -----------------------------------------------------------------------------
TEST_CASE("OPCUAExchange: write", "[opcua][exchange][write]")
{
    InitTest();

    opcTestServer1->setI32(wrAttr3, 100);
    opcTestServer1->setBool(wrAttr4, false);

    REQUIRE(opcTestServer1->getI32(wrAttr3) == 100 );
    REQUIRE(opcTestServer1->getBool(wrAttr4) == false );

    REQUIRE_NOTHROW(shm->setValue(sidAttr3, 1000));
    REQUIRE_NOTHROW(shm->setValue(sidAttr4, 1));
    msleep(step_pause_msec);
    REQUIRE(opcTestServer1->getI32(wrAttr3) == 1000);
    REQUIRE(opcTestServer1->getBool(wrAttr4) == true);

    REQUIRE_NOTHROW(shm->setValue(sidAttr3, 20));
    REQUIRE_NOTHROW(shm->setValue(sidAttr4, 0));
    msleep(step_pause_msec);
    REQUIRE(opcTestServer1->getI32(wrAttr3) == 20 );
    REQUIRE(opcTestServer1->getBool(wrAttr4) == false);
}
// -----------------------------------------------------------------------------
TEST_CASE("OPCUAExchange: ignore", "[opcua][exchange][ignore]")
{
    InitTest();
    opcTestServer1->setI32(ignAttr5, 100);
    REQUIRE(opcTestServer1->getI32(ignAttr5) == 100 );
    REQUIRE( shm->getValue(sidAttr5) == 0 );
    msleep(step_pause_msec);
    REQUIRE( shm->getValue(sidAttr5) == 0 );

    opcTestServer1->setI32(ignAttr5, 10);
    REQUIRE(opcTestServer1->getI32(ignAttr5) == 10 );
    msleep(step_pause_msec);
    REQUIRE( shm->getValue(sidAttr5) == 0 );
}
// -----------------------------------------------------------------------------
TEST_CASE("OPCUAExchange: read numeric nodeId", "[opcua][exchange][readI]")
{
    InitTest();

    opcTestServer1->setI32(rdI101, 10);
    REQUIRE(opcTestServer1->getI32(rdI101) == 10 );
    msleep(step_pause_msec);
    REQUIRE(shm->getValue(sidAttrI101) == 10);

    opcTestServer1->setI32(rdI101, 20);
    REQUIRE(opcTestServer1->getI32(rdI101) == 20 );
    msleep(step_pause_msec);
    REQUIRE(shm->getValue(sidAttrI101) == 20);
}
// -----------------------------------------------------------------------------
TEST_CASE("OPCUAExchange: read types", "[opcua][exchange][types]")
{
    InitTest();

    // init
    REQUIRE_NOTHROW(opcTestServer1->setX(1002, 2, opcua::Type::Int16));
    REQUIRE_NOTHROW(opcTestServer1->setX(1003, 3, opcua::Type::UInt16));
    REQUIRE_NOTHROW(opcTestServer1->setX(1004, 4, opcua::Type::Int64));
    REQUIRE_NOTHROW(opcTestServer1->setX(1005, 5, opcua::Type::UInt64));
    REQUIRE_NOTHROW(opcTestServer1->setX(1006, 6, opcua::Type::Byte));
    // check
    REQUIRE(opcTestServer1->getX(1002, opcua::Type::Int16) == 2 );
    REQUIRE(opcTestServer1->getX(1003, opcua::Type::UInt16) == 3 );
    REQUIRE(opcTestServer1->getX(1004, opcua::Type::Int64) == 4 );
    REQUIRE(opcTestServer1->getX(1005, opcua::Type::UInt64) == 5 );
    REQUIRE(opcTestServer1->getX(1006, opcua::Type::Byte) == 6 );

    REQUIRE_NOTHROW(shm->setValue(1022, 102)); // int16
    REQUIRE_NOTHROW(shm->setValue(1023, 103)); // uint16
    REQUIRE_NOTHROW(shm->setValue(1024, 104)); // int64
    REQUIRE_NOTHROW(shm->setValue(1025, 105)); // uint64
    REQUIRE_NOTHROW(shm->setValue(1026, 106)); // byte
    msleep(step_pause_msec);

    REQUIRE(opcTestServer1->getX(1002, opcua::Type::Int16) == 102 );
    REQUIRE(opcTestServer1->getX(1003, opcua::Type::UInt16) == 103 );
    REQUIRE(opcTestServer1->getX(1004, opcua::Type::Int64) == 104 );
    REQUIRE(opcTestServer1->getX(1005, opcua::Type::UInt64) == 105 );
    REQUIRE(opcTestServer1->getX(1006, opcua::Type::Byte) == 106 );
}
// -----------------------------------------------------------------------------
TEST_CASE("OPCUAExchange: change channel", "[opcua][exchange][connection]")
{
    InitTest();

    opcTestServer1->setI32(rdI101, 10);
    opcTestServer2->setI32(rdI101, 10);
    REQUIRE(opcTestServer1->getI32(rdI101) == 10 );
    REQUIRE(opcTestServer2->getI32(rdI101) == 10 );
    msleep(step_pause_msec);
    REQUIRE(shm->getValue(sidAttrI101) == 10);

    REQUIRE(shm->getValue(sidRespond) == 1);
    REQUIRE(shm->getValue(sidRespond1) == 1);
    REQUIRE(shm->getValue(sidRespond2) == 1);

    opcTestServer1->setI32(rdI101, 20);
    opcTestServer2->setI32(rdI101, 50);
    REQUIRE(opcTestServer1->getI32(rdI101) == 20 );
    REQUIRE(opcTestServer2->getI32(rdI101) == 50 );

    opcTestServer1->stop();
    msleep(timeout_msec);
    REQUIRE(shm->getValue(sidAttrI101) == 50);
    REQUIRE(shm->getValue(sidRespond) == 1);
    REQUIRE(shm->getValue(sidRespond1) == 0);
    REQUIRE(shm->getValue(sidRespond2) == 1);

    opcTestServer2->setI32(rdI101, 150);
    REQUIRE(opcTestServer2->getI32(rdI101) == 150 );
    msleep(step_pause_msec);
    REQUIRE(shm->getValue(sidAttrI101) == 150);

    REQUIRE(shm->getValue(sidRespond) == 1);
    REQUIRE(shm->getValue(sidRespond1) == 0);
    REQUIRE(shm->getValue(sidRespond2) == 1);

    opcTestServer2->stop();
    msleep(timeout_msec);
    REQUIRE(shm->getValue(sidRespond) == 0);
    REQUIRE(shm->getValue(sidRespond1) == 0);
    REQUIRE(shm->getValue(sidRespond2) == 0);
}
// -----------------------------------------------------------------------------
