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
const int rdI101 = 101;
const ObjectId sidAttr1 = 1000;
const ObjectId sidThr = 1001;
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
        ui = make_shared<UInterface>(uniset::AdminID);
        CHECK( ui->getObjectIndex() != nullptr );
        CHECK( ui->getConf() == conf );
    }

    if( !smi )
    {
        if( shm == nullptr )
            throw SystemError("SharedMemory don`t initialize..");

        if( ui == nullptr )
            throw SystemError("UInterface don`t initialize..");

        smi = make_shared<SMInterface>(ui->getId(), ui, ui->getId(), shm );
    }

    CHECK(opcTestServer1 != nullptr );
    CHECK(opcTestServer1->isRunning() );

    CHECK(opcTestServer2 != nullptr );
    CHECK(opcTestServer2->isRunning() );

    REQUIRE( opc != nullptr );
    REQUIRE_NOTHROW( ui->setValue(exchangeMode, OPCUAExchange::emNone) );
}
// -----------------------------------------------------------------------------
TEST_CASE("OPCUAExchange: check thr", "[opcua][checkThr]")
{
    InitTest();

    opcTestServer1->setI32(rdAttr1, 2);
    opcTestServer2->setI32(rdAttr1, 20);
    REQUIRE(opcTestServer1->getI32(rdAttr1) == 2 );
    msleep(step_pause_msec);
    REQUIRE(ui->getValue(sidAttr1) == 2);
    REQUIRE(ui->getValue(sidThr) == 0);

    opcTestServer1->setI32(rdAttr1, 7);
    REQUIRE(opcTestServer1->getI32(rdAttr1) == 7 );
    msleep(step_pause_msec);
    REQUIRE(ui->getValue(sidAttr1) == 7);
    REQUIRE(ui->getValue(sidThr) == 0);

    opcTestServer1->setI32(rdAttr1, 15);
    REQUIRE(opcTestServer1->getI32(rdAttr1) == 15 );
    msleep(step_pause_msec);
    REQUIRE(ui->getValue(sidAttr1) == 15);
    REQUIRE(ui->getValue(sidThr) == 1);

    opcTestServer1->setI32(rdAttr1, 8);
    opcTestServer2->setI32(rdAttr1, 2);
    REQUIRE(opcTestServer1->getI32(rdAttr1) == 8 );
    msleep(step_pause_msec);
    REQUIRE(ui->getValue(sidAttr1) == 8);
    REQUIRE(ui->getValue(sidThr) == 1);

    opcTestServer1->setI32(rdAttr1, 3);
    REQUIRE(opcTestServer1->getI32(rdAttr1) == 3 );
    msleep(step_pause_msec);
    REQUIRE(ui->getValue(sidAttr1) == 3);
    REQUIRE(ui->getValue(sidThr) == 0);
}
// -----------------------------------------------------------------------------
