#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <time.h>
#include <limits>
#include <memory>
#include <iostream>
#include "UniSetTypes.h"
#include "UInterface.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// -----------------------------------------------------------------------------
static int port = 8081;
static string addr("127.0.0.1");
static std::shared_ptr<UInterface> ui;
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
}
// -----------------------------------------------------------------------------
TEST_CASE("[OPCUAGAte]: init", "[opcuagate]")
{
    InitTest();
}
// -----------------------------------------------------------------------------
