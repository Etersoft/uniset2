#include <catch.hpp>
// --------------------------------------------------------------------------
#include "Trigger.h"
// --------------------------------------------------------------------------
using namespace std;
// --------------------------------------------------------------------------
TEST_CASE( "Trigger", "[Trigger]" )
{
	SECTION("Constructor")
	{
		Trigger tr;
		CHECK_FALSE( tr.hi(false) );
		CHECK( tr.hi(true) );

		Trigger trHi;
		CHECK( tr.low(false) );
		CHECK( tr.hi(true) );
	}
	
	SECTION("Working")
	{
		Trigger tr;
		CHECK_FALSE( tr.low(false) );
		CHECK_FALSE( tr.change(false) );
		CHECK( tr.hi(true) );
		CHECK_FALSE( tr.change(true) );
		CHECK( tr.low(false) );
		CHECK( tr.change(true) );
		CHECK( tr.low(false) );
		CHECK( tr.hi(true) );
		CHECK_FALSE( tr.hi(false) );
	}
}
