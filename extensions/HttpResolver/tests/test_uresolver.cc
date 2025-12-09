#include <catch.hpp>
// -----------------------------------------------------------------------------
#include "UniSetTypes.h"
#include "Exceptions.h"
#include "UInterface.h"
#include "UHttpClient.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// -----------------------------------------------------------------------------
static shared_ptr<UInterface> ui;
static const ObjectId TestProc = 6000;
static const ObjectId Node1 = 3001;
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
    }

    REQUIRE( conf->getHttpResovlerPort() == 8008 );
    REQUIRE( conf->isLocalIOR() );
    REQUIRE_NOTHROW( ui->resolve(TestProc) );
    REQUIRE( ui->isExist(TestProc) );
}
// -----------------------------------------------------------------------------
TEST_CASE("HttpResolver: cli resolve", "[httpresolver][cli]")
{
    InitTest();

    UHttp::UHttpClient cli;

    auto ret = cli.get("localhost", 8008, "api/v2/resolve/text?" + std::to_string(TestProc));
    REQUIRE_FALSE( ret.empty() );
}
// -----------------------------------------------------------------------------
TEST_CASE("HttpResolver: resolve", "[httpresolver][ui]")
{
    InitTest();

    REQUIRE_NOTHROW( ui->resolve(TestProc, Node1) );
    REQUIRE_NOTHROW( ui->resolve("UNISET_PLC/UniObjects/TestProc") );
    REQUIRE_THROWS_AS( ui->resolve(DefaultObjectId, Node1), uniset::ResolveNameError );
    REQUIRE( ui->isExist(TestProc, Node1) );
}
// -----------------------------------------------------------------------------

