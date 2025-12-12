#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <iostream>
#include <sstream>
// -----------------------------------------------------------------------------
#include "Exceptions.h"
#include "ObjectIndex_hashXML.h"
#include "ORepHelpers.h"
#include "UniSetTypes.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// -----------------------------------------------------------------------------
TEST_CASE("ObjectIndexHash", "[oindex_hash][basic]" )
{
    ObjectIndex_hashXML oi("tests_oindex_hash_config.xml");
    REQUIRE( oi.getIdByName("UNISET_PLC/Sensors/Input1_S") == uniset::hash32("Input1_S") );
    REQUIRE( oi.getIdByName("UNISET_PLC/Sensors/Input2_S") == uniset::hash32("Input2_S") );

    auto oinf = oi.getObjectInfo(uniset::hash32("Input1_S"));
    REQUIRE( oinf != nullptr );
    REQUIRE( oinf->name == "Input1_S");
    REQUIRE( oi.getMapName(uniset::hash32("Input1_S")) == "UNISET_PLC/Sensors/Input1_S" );
    REQUIRE( oi.getTextName(uniset::hash32("Input1_S")) == "Команда 1" );
}
// -----------------------------------------------------------------------------
TEST_CASE("ObjectIndexHash: getShortName", "[oindex_hash][basic]" )
{
    ObjectIndex_hashXML oi("tests_oindex_hash_config.xml");

    // getShortName should return the short name directly from ObjectInfo
    // without parsing the full path from getMapName
    auto id1 = uniset::hash32("Input1_S");
    auto id2 = uniset::hash32("Input2_S");

    REQUIRE( oi.getShortName(id1) == "Input1_S" );
    REQUIRE( oi.getShortName(id2) == "Input2_S" );

    // Verify it matches ORepHelpers::getShortName(getMapName(id)) result
    REQUIRE( oi.getShortName(id1) == ORepHelpers::getShortName(oi.getMapName(id1)) );

    // getShortName should return empty string for non-existent id
    REQUIRE( oi.getShortName(999999) == "" );
}
// -----------------------------------------------------------------------------
TEST_CASE("ObjectIndexHash: collision", "[oindex_hash][base][collision]" )
{
    REQUIRE_THROWS_AS( ObjectIndex_hashXML("tests_oindex_hash_collision_config.xml"), uniset::SystemError );
}
