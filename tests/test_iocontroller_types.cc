#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <sstream>
#include <limits>
#include "Configuration.h"
#include "UniSetTypes.h"
#include "IOController.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// -----------------------------------------------------------------------------
//! \todo test_iocontroller_types: Дописать больше тестов
// -----------------------------------------------------------------------------
TEST_CASE("IOController: USensorInfo", "[ioc][usi]" )
{
    IOController::USensorInfo usi;
    usi.supplier = 100;
    usi.blocked = true;
    usi.value = 9;

    SECTION( "makeSensorIOInfo" )
    {
        IOController_i::SensorIOInfo si( usi.makeSensorIOInfo() );
        REQUIRE( si.supplier == 100 );
        REQUIRE( si.blocked == true );
        REQUIRE( si.value == 9 );
    }

    SECTION( "makeSensorMessage" )
    {
        uniset::SensorMessage sm( usi.makeSensorMessage() );
        REQUIRE( sm.supplier == 100 );
        REQUIRE( sm.value == 9 );
    }

    WARN("Tests for 'UniSetTypes' incomplete...");
}

TEST_CASE("UniSetTypes: parse negative sensor id", "[types][getSInfoList]")
{
    auto lst = getSInfoList("-100", nullptr);
    REQUIRE(lst.size() == 1);
    REQUIRE(lst.front().si.id == -100);
    REQUIRE(lst.front().si.node == DefaultObjectId);
}
// -----------------------------------------------------------------------------
