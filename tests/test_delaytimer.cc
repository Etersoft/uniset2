#include <catch.hpp>
// -----------------------------------------------------------------------------
#include "DelayTimer.h"
#include "UniSetTypes.h"
using namespace std;
// -----------------------------------------------------------------------------
TEST_CASE("[DelayTimer]: default", "[DelayTimer]" )
{
	SECTION( "Default constructor" )
	{
		// Работа без задержки..(нулевые задержки)
		DelayTimer dt;
		CHECK_FALSE( dt.get() );
		CHECK_FALSE( dt.check(false) );
		CHECK( dt.check(true) );
		msleep(15);
		CHECK( dt.get() );
		CHECK_FALSE( dt.check(false) );
		CHECK_FALSE( dt.get() );
		CHECK( dt.check(true) );
		CHECK( dt.get() );
	}

	SECTION( "Init constructor" )
	{
		DelayTimer dt(100, 50);
		CHECK_FALSE( dt.get() );
		CHECK_FALSE( dt.check(false) );
		CHECK_FALSE( dt.check(true) );
	}
	SECTION( "Copy" )
	{
		DelayTimer dt1(100, 50);
		DelayTimer dt2(200, 100);

		dt1 = dt2;

		REQUIRE( dt1.getOnDelay() == 200 );
		REQUIRE( dt1.getOffDelay() == 100 );

		dt1.check(true);
		msleep(220);
		CHECK( dt1.get() );
		CHECK_FALSE( dt2.get() );
	}

	SECTION( "Other" )
	{
		DelayTimer dt(100, 50);
		REQUIRE( dt.getOnDelay() == 100 );
		REQUIRE( dt.getOffDelay() == 50 );

		dt.set(150, 200);
		REQUIRE( dt.getOnDelay() == 150 );
		REQUIRE( dt.getOffDelay() == 200 );
	}
}
// -----------------------------------------------------------------------------
TEST_CASE("[DelayTimer]: working", "[DelayTimer]" )
{
	DelayTimer dt(100, 60);
	CHECK_FALSE( dt.get() );
	CHECK_FALSE( dt.check(false) );

	// проверяем срабатывание..
	CHECK_FALSE( dt.check(true) );
	msleep(50);
	CHECK_FALSE( dt.check(true) );
	msleep(60);
	CHECK( dt.check(true) );
	CHECK( dt.get() );

	// проверяем отпускание
	// несмотря на вызов check(false).. должно ещё 60 мсек возвращать true
	CHECK( dt.check(false) );
	CHECK( dt.get() );
	msleep(20);
	CHECK( dt.check(false) );
	CHECK( dt.get() );
	msleep(50); // в сумме уже 20+50=70 > 60, значит должно "отпустить"
	CHECK_FALSE( dt.check(false) );
	CHECK_FALSE( dt.get() );

	dt.reset();
	CHECK_FALSE( dt.check(true) );
	msleep(50);
	CHECK_FALSE( dt.check(true) );
	dt.reset();
	CHECK_FALSE( dt.check(true) );
	msleep(60);
	CHECK_FALSE( dt.check(true) );
	msleep(60);
	CHECK( dt.check(true) );
	CHECK( dt.get() );
}
// -----------------------------------------------------------------------------
TEST_CASE("[DelayTimer]: debounce", "[DelayTimer]" )
{
	DelayTimer dt(150, 100);
	CHECK_FALSE( dt.get() );
	CHECK_FALSE( dt.check(false) );

	// проверяем срабатывание.. (при скакании сигнала)
	CHECK_FALSE( dt.check(true) );
	msleep(50);
	CHECK_FALSE( dt.check(false) );
	msleep(60);
	CHECK_FALSE( dt.check(true) );
	CHECK_FALSE( dt.get() );

	msleep(100);
	CHECK_FALSE( dt.check(true) );
	CHECK_FALSE( dt.get() );
	msleep(60);
	CHECK( dt.check(true) );
	CHECK( dt.get() );

	// проверяем отпускание при скакании сигнала
	CHECK( dt.check(false) );
	CHECK( dt.get() );
	msleep(60);
	CHECK( dt.check(true) );
	CHECK( dt.get() );
	dt.check(false);
	msleep(80);
	CHECK( dt.check(false) );
	CHECK( dt.get() );
	msleep(40);
	CHECK_FALSE( dt.check(false) );
	CHECK_FALSE( dt.get() );

	// проверяем срабатвание (одноразовый скачок)
	CHECK_FALSE( dt.check(true) );
	CHECK_FALSE( dt.check(false) );
	msleep(160);
	CHECK_FALSE( dt.check(true) );
	CHECK_FALSE( dt.check(false) );
	msleep(160);
	CHECK_FALSE( dt.check(false) );
}
// -----------------------------------------------------------------------------
TEST_CASE("[DelayTimer]: zero time", "[DelayTimer]" )
{
	SECTION( "ondelay=0" )
	{
		DelayTimer dt(0, 100);
		CHECK_FALSE( dt.get() );
		CHECK_FALSE( dt.check(false) );
		CHECK( dt.check(true) );
		CHECK( dt.check(false) );
		CHECK( dt.check(true) );
		CHECK( dt.check(false) );
		msleep(80);
		CHECK( dt.check(false) );
		msleep(40);
		CHECK_FALSE( dt.check(false) );
		CHECK( dt.check(true) );
		msleep(40);
		CHECK( dt.check(true) );
		msleep(40);
		CHECK( dt.check(true) );
		CHECK( dt.check(false) );
		msleep(80);
		CHECK( dt.check(false) );
		msleep(40);
		CHECK_FALSE( dt.check(false) );
	}

	SECTION( "offdelay=0" )
	{
		DelayTimer dt(100, 0);
		CHECK_FALSE( dt.get() );
		CHECK_FALSE( dt.check(false) );
		CHECK_FALSE( dt.check(true) );
		CHECK_FALSE( dt.check(false) );
		CHECK_FALSE( dt.check(true) );
		msleep(80);
		CHECK_FALSE( dt.check(true) );
		msleep(40);
		CHECK( dt.check(true) );
		CHECK_FALSE( dt.check(false) );
		msleep(40);
		CHECK_FALSE( dt.check(false) );
		CHECK_FALSE( dt.check(true) );
		msleep(80);
		CHECK_FALSE( dt.check(true) );
		msleep(40);
		CHECK( dt.check(true) );
	}

	SECTION("off delay")
	{
		DelayTimer dt(0,2000);
		PassiveTimer pt;
		dt.check(true);
		while( dt.check(false) )
			msleep(100);

		REQUIRE( pt.getCurrent() >= 2000 );
	}
}
// -----------------------------------------------------------------------------
