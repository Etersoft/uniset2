#include <catch.hpp>

#include "Pulse.h"
#include "UniSetTypes.h"
using namespace std;

TEST_CASE("Pulse", "[Test for class 'Pulse' - impulse generator]" )
{
	SECTION( "Default constructor" )
	{
		// Работа без задержки..(нулевые задержки)
		Pulse p;
		
		REQUIRE( p.getT1() == 0 );
		REQUIRE( p.getT0() == 0 );

		CHECK_FALSE( p.out() );

		p.step();
		p.step();
		CHECK_FALSE( p.out() );
	}

	SECTION( "Working" )
	{
		Pulse p;
		p.run(100, 60); // 100 msec ON..60 msec OFF
		CHECK( p.step() );

		msleep(80);
		CHECK( p.step() );

		msleep(30);
		CHECK_FALSE( p.step() ); // 10 - OFF

		msleep(70);
		CHECK( p.step() ); // 20 - ON

		msleep(100);
		CHECK_FALSE( p.step() ); // 20 OFF

		p.set(false);
		msleep(100);
		CHECK_FALSE( p.step() );
		msleep(60);
		CHECK_FALSE( p.step() );

		p.set(true);
		msleep(70);
		CHECK( p.step() );
		msleep(40);
		CHECK_FALSE( p.step() );

		p.reset();
		CHECK( p.step() );
		msleep(110);
		CHECK_FALSE( p.step() );

		// проверка того, что при познем вызове step
		// тоже всё будет хорошо..
		p.reset();
		CHECK( p.step() );
		msleep(110);
		CHECK_FALSE( p.step() );
		msleep(2050);
		CHECK( p.step() );
		msleep(110);
		CHECK_FALSE( p.step() );
		msleep(70);
		CHECK( p.step() );
	}
}
