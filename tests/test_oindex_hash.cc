#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <iostream>
#include <sstream>
// -----------------------------------------------------------------------------
#include "Exceptions.h"
#include "ObjectIndex_hashXML.h"
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
}
// -----------------------------------------------------------------------------
TEST_CASE("ObjectIndexHash: collision", "[oindex_hash][base][collision]" )
{
	REQUIRE_THROWS_AS( ObjectIndex_hashXML("tests_oindex_hash_collision_config.xml"), uniset::SystemError );
}
