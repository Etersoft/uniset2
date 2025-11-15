#include <catch.hpp>
#include <iostream>
#include <memory>

#include "Configuration.h"
#include "OPCUATestServer.h"
#include "SMInterface.h"
#include "SharedMemory.h"
#include "JSProxy.h"
#include "PassiveTimer.h"

using namespace std;
using namespace uniset;

const ObjectId sidUI_TestCommand_S = 1013;
const ObjectId sidUI_TestResult_C = 1014;
const ObjectId sidUI_TestOutput1 = 1011;
const ObjectId sidUI_TestOutput2 = 1012;
const int kOpcuaTestCommand = 15;
const int kExpectedIntValue = 55;
const int kExpectedBoolValue = 1;
constexpr uint16_t kOpcuaPort = 15480;

// -----------------------------------------------------------------------------
extern std::shared_ptr<OPCUATestServer> server;
static shared_ptr<UInterface> ui;
static std::shared_ptr<SMInterface> smi;
// -----------------------------------------------------------------------------
extern std::shared_ptr<SharedMemory> shm;
extern std::shared_ptr<JSProxy> js;
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

    if( !server )
    {
        server = std::make_shared<OPCUATestServer>("127.0.0.1", kOpcuaPort);
        REQUIRE( server->start() );
        msleep(3000);
    }

    CHECK(js != nullptr );
}
// -----------------------------------------------------------------------------
static void ResetOpcuaTestState()
{
    ui->setValue(sidUI_TestCommand_S, 0);
    ui->setValue(sidUI_TestResult_C, 0);
    ui->setValue(sidUI_TestOutput1, 0);
    ui->setValue(sidUI_TestOutput2, 0);
    msleep(100);
}
// -----------------------------------------------------------------------------
static bool ExecuteOpcuaJsTest(int maxWaitMsec = 6000)
{
    ResetOpcuaTestState();
    ui->setValue(sidUI_TestCommand_S, kOpcuaTestCommand);

    PassiveTimer wait(maxWaitMsec);

    while( !wait.checkTime() )
    {
        auto res = ui->getValue(sidUI_TestResult_C);

        if( res != 0 )
            return res == 1;

        msleep(100);
    }

    return false;
}
// -----------------------------------------------------------------------------
TEST_CASE("JSEngine: OPC UA integration script", "[jscript][opcua][15]")
{
    InitTest();
    REQUIRE( ExecuteOpcuaJsTest() );
    REQUIRE( ui->getValue(sidUI_TestOutput1) == kExpectedIntValue );
    REQUIRE( ui->getValue(sidUI_TestOutput2) == kExpectedBoolValue );
}
// -----------------------------------------------------------------------------
