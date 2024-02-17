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
const std::string rdAttr6 = "Attr6";
const std::string wrAttr7 = "Attr7";
const std::string wrFloatOut = "FloatOut1";
const std::string rdFloat = "Float1";
const int rdI101 = 101;
const ObjectId sidAttr1 = 1000;
const ObjectId sidAttr2 = 1001;
const ObjectId sidAttr3 = 1010;
const ObjectId sidAttr4 = 1011;
const ObjectId sidAttr5 = 1020;
const ObjectId sidAttr6 = 1027;
const ObjectId sidAttr7 = 1028;
const ObjectId sidAttrI101 = 1021;
const ObjectId sidFloat1 = 1029;
const ObjectId sidFloatOut1 = 1030;
const ObjectId sidRespond = 10;
const ObjectId sidRespond1 = 11;
const ObjectId sidRespond2 = 12;
const ObjectId exchangeMode = 13;
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
    REQUIRE_NOTHROW( ui->setValue(exchangeMode, OPCUAExchange::emNone) );
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

    // float
    opcTestServer1->setF32(rdFloat, 10.01);
    REQUIRE(opcTestServer1->getF32(rdFloat) == 10.01f );
    msleep(step_pause_msec);
    REQUIRE(shm->getValue(sidFloat1) == 1001);

    // float: round up
    opcTestServer1->setF32(rdFloat, 5.056);
    msleep(step_pause_msec);
    REQUIRE(shm->getValue(sidFloat1) == 506);

    // float: round down
    opcTestServer1->setF32(rdFloat, 5.053);
    msleep(step_pause_msec);
    REQUIRE(shm->getValue(sidFloat1) == 505);
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

    // float
    opcTestServer1->setF32(wrFloatOut, 0.0);
    REQUIRE_NOTHROW(shm->setValue(sidFloatOut1, 2000));
    msleep(step_pause_msec);
    REQUIRE( opcTestServer1->getF32(wrFloatOut) == 20.0f );

    REQUIRE_NOTHROW(shm->setValue(sidFloatOut1, 2226));
    msleep(step_pause_msec);
    REQUIRE( opcTestServer1->getF32(wrFloatOut) == 22.26f );
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
    REQUIRE_NOTHROW(opcTestServer1->setX(1002, 2, opcua::DataTypeId::Int16));
    REQUIRE_NOTHROW(opcTestServer1->setX(1003, 3, opcua::DataTypeId::UInt16));
    REQUIRE_NOTHROW(opcTestServer1->setX(1004, 4, opcua::DataTypeId::Int64));
    REQUIRE_NOTHROW(opcTestServer1->setX(1005, 5, opcua::DataTypeId::UInt64));
    REQUIRE_NOTHROW(opcTestServer1->setX(1006, 6, opcua::DataTypeId::Byte));
    // check
    REQUIRE(opcTestServer1->getX(1002, opcua::DataTypeId::Int16) == 2 );
    REQUIRE(opcTestServer1->getX(1003, opcua::DataTypeId::UInt16) == 3 );
    REQUIRE(opcTestServer1->getX(1004, opcua::DataTypeId::Int64) == 4 );
    REQUIRE(opcTestServer1->getX(1005, opcua::DataTypeId::UInt64) == 5 );
    REQUIRE(opcTestServer1->getX(1006, opcua::DataTypeId::Byte) == 6 );

    REQUIRE_NOTHROW(shm->setValue(1022, 102)); // int16
    REQUIRE_NOTHROW(shm->setValue(1023, 103)); // uint16
    REQUIRE_NOTHROW(shm->setValue(1024, 104)); // int64
    REQUIRE_NOTHROW(shm->setValue(1025, 105)); // uint64
    REQUIRE_NOTHROW(shm->setValue(1026, 106)); // byte
    msleep(step_pause_msec);

    REQUIRE(opcTestServer1->getX(1002, opcua::DataTypeId::Int16) == 102 );
    REQUIRE(opcTestServer1->getX(1003, opcua::DataTypeId::UInt16) == 103 );
    REQUIRE(opcTestServer1->getX(1004, opcua::DataTypeId::Int64) == 104 );
    REQUIRE(opcTestServer1->getX(1005, opcua::DataTypeId::UInt64) == 105 );
    REQUIRE(opcTestServer1->getX(1006, opcua::DataTypeId::Byte) == 106 );
}
// -----------------------------------------------------------------------------
TEST_CASE("OPCUAExchange: first bit", "[opcua][firstbit]")
{
    REQUIRE( OPCUAExchange::firstBit(4) == 2 );
    REQUIRE( OPCUAExchange::firstBit(1) == 0 );
    REQUIRE( OPCUAExchange::firstBit(64) == 6 );
    REQUIRE( OPCUAExchange::firstBit(12) == 2 );
    REQUIRE( OPCUAExchange::firstBit(192) == 6 );
}
// -----------------------------------------------------------------------------
TEST_CASE("OPCUAExchange: get bits", "[opcua][getbits]")
{
    REQUIRE( OPCUAExchange::getBits(1, 1, 0) == 1 );
    REQUIRE( OPCUAExchange::getBits(12, 12, 2) == 3 );
    REQUIRE( OPCUAExchange::getBits(9, 12, 2) == 2 );
    REQUIRE( OPCUAExchange::getBits(9, 3, 0) == 1 );
    REQUIRE( OPCUAExchange::getBits(15, 12, 2) == 3 );
    REQUIRE( OPCUAExchange::getBits(15, 3, 0) == 3 );
    REQUIRE( OPCUAExchange::getBits(15, 0, 0) == 15 );
    REQUIRE( OPCUAExchange::getBits(15, 0, 3) == 15 );
}
// -----------------------------------------------------------------------------
TEST_CASE("OPCUAExchange: set bits", "[opcua][setbits]")
{
    REQUIRE( OPCUAExchange::setBits(1, 1, 1, 0) == 1 );
    REQUIRE( OPCUAExchange::setBits(0, 3, 12, 2) == 12 );
    REQUIRE( OPCUAExchange::setBits(3, 3, 12, 2) == 15 );
    REQUIRE( OPCUAExchange::setBits(16, 3, 12, 2) == 28 );
    REQUIRE( OPCUAExchange::setBits(28, 1, 3, 0) == 29 );
    REQUIRE( OPCUAExchange::setBits(28, 10, 0, 0) == 28 );
    REQUIRE( OPCUAExchange::setBits(28, 10, 0, 4) == 28 );
}
// -----------------------------------------------------------------------------
TEST_CASE("OPCUAExchange: force set bits", "[opcua][forcesetbits]")
{
    REQUIRE( OPCUAExchange::forceSetBits(1, 1, 1, 0) == 1 );
    REQUIRE( OPCUAExchange::forceSetBits(0, 3, 12, 2) == 12 );
    REQUIRE( OPCUAExchange::forceSetBits(28, 10, 0, 0) == 10 );
    REQUIRE( OPCUAExchange::forceSetBits(28, 10, 0, 4) == 10 );

    REQUIRE( OPCUAExchange::forceSetBits(0, 2, 3, 0) == 2 );
    REQUIRE( OPCUAExchange::forceSetBits(2, 2, 12, 2) == 10 );

    REQUIRE( OPCUAExchange::forceSetBits(2, 0, 3, 0) == 0 );
    REQUIRE( OPCUAExchange::forceSetBits(12, 0, 12, 2) == 0 );

    REQUIRE( OPCUAExchange::forceSetBits(3, 5, 3, 0) == 1 );
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: read/write mask", "[opcua][exchange][mask]")
{
    InitTest();
    opcTestServer1->setI32(wrAttr7, 0);
    opcTestServer1->setI32(rdAttr6, 0);

    // read
    opcTestServer1->setI32(rdAttr6, 11);
    REQUIRE(opcTestServer1->getI32(rdAttr6) == 11 );
    msleep(step_pause_msec);
    REQUIRE(shm->getValue(sidAttr6) == 3 );

    opcTestServer1->setI32(rdAttr6, 5);
    REQUIRE(opcTestServer1->getI32(rdAttr6) == 5 );
    msleep(step_pause_msec);
    REQUIRE(shm->getValue(sidAttr6) == 1 );

    opcTestServer1->setI32(rdAttr6, 2);
    REQUIRE(opcTestServer1->getI32(rdAttr6) == 2 );
    msleep(step_pause_msec);
    REQUIRE(shm->getValue(sidAttr6) == 2 );

    opcTestServer1->setI32(rdAttr6, 0);
    REQUIRE(opcTestServer1->getI32(rdAttr6) == 0 );
    msleep(step_pause_msec);
    REQUIRE(shm->getValue(sidAttr6) == 0 );

    opcTestServer1->setI32(rdAttr6, 8);
    REQUIRE(opcTestServer1->getI32(rdAttr6) == 8 );
    msleep(step_pause_msec);
    REQUIRE(shm->getValue(sidAttr6) == 0 );

    // write
    opcTestServer1->setI32(wrAttr7, 100);
    REQUIRE(opcTestServer1->getI32(wrAttr7) == 100 );
    REQUIRE_NOTHROW(shm->setValue(sidAttr7, 1));
    msleep(step_pause_msec);
    REQUIRE(opcTestServer1->getI32(wrAttr7) == 1 );

    REQUIRE_NOTHROW(shm->setValue(sidAttr7, 3));
    msleep(step_pause_msec);
    REQUIRE(opcTestServer1->getI32(wrAttr7) == 3 );

    REQUIRE_NOTHROW(shm->setValue(sidAttr7, 5));
    msleep(step_pause_msec);
    REQUIRE(opcTestServer1->getI32(wrAttr7) == 1 );

    REQUIRE_NOTHROW(shm->setValue(sidAttr7, 8));
    msleep(step_pause_msec);
    REQUIRE(opcTestServer1->getI32(wrAttr7) == 0 );

    REQUIRE_NOTHROW(shm->setValue(sidAttr7, 2));
    msleep(step_pause_msec);
    REQUIRE(opcTestServer1->getI32(wrAttr7) == 2 );
}
// -----------------------------------------------------------------------------
TEST_CASE("OPCUAExchange: exchangeMode", "[opcua][exchange][exchangemode]")
{
    InitTest();
    opcTestServer1->setI32(wrAttr3, 0);
    opcTestServer1->setI32(rdAttr1, 0);

    SECTION("None")
    {
        REQUIRE_NOTHROW( ui->setValue(exchangeMode, OPCUAExchange::emNone ) );
        REQUIRE( ui->getValue(exchangeMode) == OPCUAExchange::emNone );
        msleep(step_pause_msec);

        SECTION("read")
        {
            opcTestServer1->setI32(rdAttr1, 10);
            REQUIRE(opcTestServer1->getI32(rdAttr1) == 10 );
            msleep(step_pause_msec);
            REQUIRE(shm->getValue(sidAttr1) == 10);
        }
        SECTION("write")
        {
            REQUIRE_NOTHROW(shm->setValue(sidAttr3, 10));
            msleep(step_pause_msec);
            REQUIRE(opcTestServer1->getI32(wrAttr3) == 10);
        }
    }

    SECTION("WriteOnly")
    {
        REQUIRE_NOTHROW( ui->setValue(exchangeMode, OPCUAExchange::emWriteOnly ) );
        REQUIRE( ui->getValue(exchangeMode) == OPCUAExchange::emWriteOnly );
        msleep(step_pause_msec);

        SECTION("read")
        {
            opcTestServer1->setI32(rdAttr1, 150);
            REQUIRE(opcTestServer1->getI32(rdAttr1) == 150 );
            msleep(step_pause_msec);
            REQUIRE( ui->getValue(sidAttr1) != 150 );

            opcTestServer1->setI32(rdAttr1, -10);
            REQUIRE(opcTestServer1->getI32(rdAttr1) == -10 );
            msleep(step_pause_msec);
            REQUIRE( ui->getValue(sidAttr1) != -10 );
            REQUIRE( ui->getValue(sidAttr1) != 150 );
        }
        SECTION("write")
        {
            REQUIRE_NOTHROW(shm->setValue(sidAttr3, 150));
            msleep(step_pause_msec);
            REQUIRE(opcTestServer1->getI32(wrAttr3) == 150);

            REQUIRE_NOTHROW(shm->setValue(sidAttr3, 155));
            msleep(step_pause_msec);
            REQUIRE(opcTestServer1->getI32(wrAttr3) == 155);
        }
    }

    SECTION("ReadOnly")
    {
        REQUIRE_NOTHROW( ui->setValue(exchangeMode, OPCUAExchange::emReadOnly ) );
        REQUIRE( ui->getValue(exchangeMode) == OPCUAExchange::emReadOnly );
        msleep(step_pause_msec);

        SECTION("read")
        {
            opcTestServer1->setI32(rdAttr1, 150);
            REQUIRE(opcTestServer1->getI32(rdAttr1) == 150 );
            msleep(step_pause_msec);
            REQUIRE( ui->getValue(sidAttr1) == 150 );

            opcTestServer1->setI32(rdAttr1, -10);
            REQUIRE(opcTestServer1->getI32(rdAttr1) == -10 );
            msleep(step_pause_msec);
            REQUIRE( ui->getValue(sidAttr1) == -10 );
        }
        SECTION("write")
        {
            REQUIRE_NOTHROW(shm->setValue(sidAttr3, 150));
            msleep(step_pause_msec);
            REQUIRE(opcTestServer1->getI32(wrAttr3) != 150);

            REQUIRE_NOTHROW(shm->setValue(sidAttr3, 250));
            msleep(step_pause_msec);
            REQUIRE(opcTestServer1->getI32(wrAttr3) != 250);

            msleep(step_pause_msec);
            REQUIRE(opcTestServer1->getI32(wrAttr3) != 150);
            REQUIRE(opcTestServer1->getI32(wrAttr3) != 250);
        }
    }

    SECTION("SkipSaveToSM")
    {
        REQUIRE_NOTHROW( ui->setValue(exchangeMode, OPCUAExchange::emSkipSaveToSM ) );
        REQUIRE( ui->getValue(exchangeMode) == OPCUAExchange::emSkipSaveToSM );
        msleep(step_pause_msec);
        SECTION("read")
        {
            opcTestServer1->setI32(rdAttr1, 150);
            REQUIRE(opcTestServer1->getI32(rdAttr1) == 150 );
            msleep(step_pause_msec);
            REQUIRE( ui->getValue(sidAttr1) != 150 );

            opcTestServer1->setI32(rdAttr1, -10);
            REQUIRE(opcTestServer1->getI32(rdAttr1) == -10 );
            msleep(step_pause_msec);
            REQUIRE( ui->getValue(sidAttr1) != -10 );
            REQUIRE( ui->getValue(sidAttr1) != 150 );
        }
        SECTION("write")
        {
            // в этом режиме "write" работает
            REQUIRE_NOTHROW(shm->setValue(sidAttr3, 150));
            msleep(step_pause_msec);
            REQUIRE(opcTestServer1->getI32(wrAttr3) == 150);

            REQUIRE_NOTHROW(shm->setValue(sidAttr3, 350));
            msleep(step_pause_msec);
            REQUIRE(opcTestServer1->getI32(wrAttr3) == 350);
        }
    }

    SECTION("SkipExchange")
    {
        REQUIRE_NOTHROW( ui->setValue(exchangeMode, OPCUAExchange::emSkipExchange ) );
        REQUIRE( ui->getValue(exchangeMode) == OPCUAExchange::emSkipExchange );
        msleep(step_pause_msec);

        SECTION("read")
        {
            opcTestServer1->setI32(rdAttr1, 150);
            REQUIRE(opcTestServer1->getI32(rdAttr1) == 150 );
            msleep(step_pause_msec);
            REQUIRE( ui->getValue(sidAttr1) != 150 );

            opcTestServer1->setI32(rdAttr1, -10);
            REQUIRE(opcTestServer1->getI32(rdAttr1) == -10 );
            msleep(step_pause_msec);
            REQUIRE( ui->getValue(sidAttr1) != -10 );
            REQUIRE( ui->getValue(sidAttr1) != 150 );
        }
        SECTION("write")
        {
            REQUIRE_NOTHROW(shm->setValue(sidAttr3, 150));
            msleep(step_pause_msec);
            REQUIRE(opcTestServer1->getI32(wrAttr3) != 150);

            REQUIRE_NOTHROW(shm->setValue(sidAttr3, 250));
            msleep(step_pause_msec);
            REQUIRE(opcTestServer1->getI32(wrAttr3) != 250);

            msleep(step_pause_msec);
            REQUIRE(opcTestServer1->getI32(wrAttr3) != 150);
            REQUIRE(opcTestServer1->getI32(wrAttr3) != 250);
        }
    }
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
