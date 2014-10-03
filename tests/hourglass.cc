#include <catch.hpp>

#include <cmath>

using namespace std;

#include "HourGlass.h"
#include "UniSetTypes.h"

TEST_CASE("HourGlass", "[HourGlass]" ) 
{
    SECTION( "Constructor" ) 
	{
		HourGlass hg;
		REQUIRE( hg.duration() == 0 );
		msleep(60); // т.к. точность +-10 мсек.. делаем паузу 60.. а проверяем 50
		REQUIRE( std::abs(hg.current()-60) <= 10 );
		REQUIRE( hg.interval() == 0 );
		REQUIRE( hg.amount() == 0 );
		REQUIRE( hg.remain() == 0 );
		CHECK_FALSE( hg.state() );
		CHECK_FALSE( hg.check() );
	}

	SECTION( "Working" ) 
	{
		HourGlass hg;
		hg.run(100);
		CHECK( hg.state() ); // часы начали тикать.. (в нормальном положении)
		REQUIRE( hg.duration() == 100 );
		CHECK_FALSE( hg.check() );
		msleep(110);
		CHECK( hg.check() );
		
		// Переворачиваем обратно.. 
		// "песок высыпается назад" в течении 50 мсек, 
		// потом опять ставим "на ноги", ждём 60 мсек.. должно сработать
		hg.rotate(false);
		CHECK_FALSE( hg.state() );
		CHECK_FALSE( hg.check() );
		msleep(50);
		CHECK_FALSE( hg.check() );
		hg.rotate(true);
		msleep(60);
		CHECK( hg.check() );
	}

	SECTION( "Reset" ) 
	{
		HourGlass hg;
		hg.run(100);
		msleep(110);
		CHECK( hg.check() );
		hg.reset();
		CHECK_FALSE( hg.check() );
		msleep(110);
		CHECK( hg.check() );
	}

	SECTION( "Debounce" ) 
	{
		HourGlass hg;
		hg.run(100); // [ 100 / 0   ]
		REQUIRE( hg.remain() == 100 );
		REQUIRE( hg.amount() == 0 );
		msleep(110); // [   0 / 110 ] "110" --> "100"
		CHECK( hg.check() );
		REQUIRE( hg.remain() == 0 );
		REQUIRE( hg.amount() == 100 );

		hg.rotate(false); // начинает сыпаться "обратно"..
		REQUIRE( hg.amount() == 100 );
		msleep(55);
		CHECK_FALSE( hg.check() );
		REQUIRE( hg.amount() <= 50 );  // +-10 мсек..

		hg.rotate(true); // опять начал сыпаться..
		CHECK_FALSE( hg.check() );
		msleep(25);	
		CHECK_FALSE( hg.check() );
		REQUIRE( hg.amount() >= 60 );

		hg.rotate(false); // опять назад..
		msleep(80);  // по сути сигнал сбросился..(т.к. оставалось 70.. а прошло 80)
		CHECK_FALSE( hg.check() );
		REQUIRE( hg.amount() == 0 );
		REQUIRE( hg.remain() == 100 );
		
		hg.rotate(true);  // вновь запустили
		CHECK_FALSE( hg.check() );
		msleep(55);
		REQUIRE( hg.amount() >= 50 );
		CHECK_FALSE( hg.check() );
		msleep(60);
		REQUIRE( hg.remain() == 0 );
		REQUIRE( hg.amount() == 100 );
		CHECK( hg.check() );
	}
}
