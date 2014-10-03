#include <Catch/catch.hpp>

#include "PassiveTimer.h"
#include "UniSetTypes.h"
using namespace std;

TEST_CASE("PassiveTimer", "[PassiveTimer]" ) 
{
    SECTION( "Default constructor" ) 
    {
		PassiveTimer pt;

		REQUIRE_FALSE( pt.checkTime() );
		msleep(15);
        REQUIRE( pt.getCurrent() >= 10 );
		REQUIRE( pt.getInterval() == 0 ); // TIMEOUT_INF );
		REQUIRE( pt.getLeft( pt.getCurrent() + 10 ) == 10 );
    }

    SECTION( "Init constructor" ) 
    {
		PassiveTimer pt(100);
		msleep(15); // т.к. точность +-10 мсек.. делаем паузу 60.. а проверяем 50
        REQUIRE( pt.getCurrent() >= 10 );
		REQUIRE( pt.getInterval() == 100 );
		REQUIRE( pt.getLeft(50) > 0 );
    }

    SECTION( "Init zero" ) 
    {
		PassiveTimer pt(0);
		REQUIRE( pt.getInterval() == 0 ); // TIMEOUT_INF );
		REQUIRE( pt.getLeft(100) == 100 );
    }

    SECTION( "Init < 0 " ) 
    {
		PassiveTimer pt(-10);
		REQUIRE( pt.getInterval() >= (timeout_t)(-10) ); // '>=' т.к. переданное время может быть округлено в большую сторону.
		REQUIRE( pt.getLeft(10) == 10 );
    }

    SECTION( "Check working" ) 
    {
		PassiveTimer pt(100);
		msleep(120); // т.к. точность +-10 мсек.. делаем паузу 60.. а проверяем 50
		REQUIRE( pt.getCurrent() >= 110 );
		CHECK( pt.checkTime() );
		INFO("Check reset");
		pt.reset();
		REQUIRE_FALSE( pt.checkTime() );

		INFO("Check setTiming");
		REQUIRE_FALSE( pt.checkTime() );
		pt.setTiming(50);
		msleep(55); // т.к. точность +-10 мсек.. делаем паузу 55..
		CHECK( pt.checkTime() );
    }

    SECTION( "Copy" ) 
    {
		PassiveTimer pt1(100);
		PassiveTimer pt2(200);

		REQUIRE( pt1.getInterval() == 100 );
		REQUIRE( pt2.getInterval() == 200 );

		pt2 = pt1;
		REQUIRE( pt1.getInterval() == 100 );
		REQUIRE( pt2.getInterval() == 100 );

		msleep(110);
		CHECK( pt1.checkTime() );
		CHECK( pt2.checkTime() );
    }
}
