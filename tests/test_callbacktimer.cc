#include <catch.hpp>

#include <atomic>

#include "CallbackTimer.h"
#include "UniSetTypes.h"
using namespace std;

class MyTestClass
{
	public:
		MyTestClass(): num1(0), num2(0), num3(0) {}
		~MyTestClass() {}

		void Time( int id )
		{
			if( id == 1 )
				num1++;
			else if( id == 2 )
				num2++;
			else if( id == 3 )
				num3++;
		}

		inline int getNum1()
		{
			return num1;
		}
		inline int getNum2()
		{
			return num2;
		}
		inline int getNum3()
		{
			return num3;
		}

	private:
		std::atomic_int num1;
		std::atomic_int num2;
		std::atomic_int num3;
};

TEST_CASE("CallbackTimer", "[CallbackTimer]" )
{
	SECTION("Basic tests")
	{
		MyTestClass tc;
		CallbackTimer<MyTestClass> tmr(&tc, &MyTestClass::Time);

		tmr.add(1, 50 );
		tmr.add(2, 150 );
		tmr.add(3, 300 );
		tmr.run();

		msleep(60);
		REQUIRE( tc.getNum1() >= 1 );
		REQUIRE( tc.getNum2() == 0 );
		REQUIRE( tc.getNum3() == 0 );

		msleep(110);
		REQUIRE( tc.getNum1() >= 2 );
		REQUIRE( tc.getNum2() >= 1 );
		REQUIRE( tc.getNum3() == 0 );

		msleep(210);
		REQUIRE( tc.getNum1() >= 5 );
		REQUIRE( tc.getNum2() >= 2 );
		REQUIRE( tc.getNum3() == 1 );

		tmr.remove(1);
		msleep(60);
		REQUIRE( tc.getNum1() >= 6 );

		tmr.terminate();
		REQUIRE( tc.getNum2() >= 2 );
		REQUIRE( tc.getNum3() >= 1 );
	}

	SECTION("other tests")
	{
		MyTestClass tc;
		CallbackTimer<MyTestClass> tmr(&tc, &MyTestClass::Time);
		unsigned int i = 0;

		for( ; i < tmr.MAXCallbackTimer; i++ )
			tmr.add(i, 100 );

		REQUIRE_THROWS_AS( tmr.add(++i, 100), UniSetTypes::LimitTimers );
	}
}
