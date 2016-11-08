#include <catch.hpp>

#include "TriggerOUT.h"
#include "UniSetTypes.h"
using namespace std;
using namespace uniset;

class MyTestClass
{
	public:
		MyTestClass(): out1(0), out2(0), out3(0) {}
		~MyTestClass() {}

		void setOut( int outID, bool state )
		{
			if( outID == 1 )
				out1++;
			else if( outID == 2 )
				out2++;
			else if( outID == 3 )
				out3++;
		}

		inline int getOut1Counter()
		{
			return out1;
		}
		inline int getOut2Counter()
		{
			return out2;
		}
		inline int getOut3Counter()
		{
			return out3;
		}

	private:
		int out1;
		int out2;
		int out3;
};

TEST_CASE("TriggerOUT", "[TriggerOUT]" )
{
	SECTION("Constructor")
	{
		MyTestClass tc;
		TriggerOUT<MyTestClass> tr(&tc, &MyTestClass::setOut);

		tr.add(1, true);
		REQUIRE( tc.getOut1Counter() == 1 );
		CHECK( tr.getState(1) );

		tr.add(2, false);
		REQUIRE( tc.getOut1Counter() == 2 );
		REQUIRE( tc.getOut2Counter() == 1 );
		CHECK_FALSE( tr.getState(1) );
		CHECK_FALSE( tr.getState(2) );

		tr.add(3, true);
		REQUIRE( tc.getOut2Counter() == 1 );
		REQUIRE( tc.getOut3Counter() == 1 );
		CHECK_FALSE( tr.getState(1) );
		CHECK_FALSE( tr.getState(2) );
		CHECK( tr.getState(3) );
	}

	SECTION("Working")
	{
		MyTestClass tc;
		TriggerOUT<MyTestClass> tr(&tc, &MyTestClass::setOut);

		tr.add(1, false);
		tr.add(2, false);
		tr.add(3, false);
		REQUIRE( tc.getOut1Counter() == 1 );
		REQUIRE( tc.getOut2Counter() == 1 );
		REQUIRE( tc.getOut3Counter() == 1 );
		CHECK_FALSE( tr.getState(1) );
		CHECK_FALSE( tr.getState(2) );
		CHECK_FALSE( tr.getState(3) );

		tr.set(1, true);
		REQUIRE( tc.getOut1Counter() == 2 );
		REQUIRE( tc.getOut2Counter() == 1 );
		REQUIRE( tc.getOut3Counter() == 1 );
		CHECK( tr.getState(1) );
		CHECK_FALSE( tr.getState(2) );
		CHECK_FALSE( tr.getState(3) );

		tr.set(2, true);
		REQUIRE( tc.getOut1Counter() == 3 );
		REQUIRE( tc.getOut2Counter() == 2 );
		REQUIRE( tc.getOut3Counter() == 1 );
		CHECK_FALSE( tr.getState(1) );
		CHECK( tr.getState(2) );
		CHECK_FALSE( tr.getState(3) );

		tr.set(3, false);
		CHECK_FALSE( tr.getState(1) );
		CHECK_FALSE( tr.getState(2) );
		CHECK_FALSE( tr.getState(3) );
		REQUIRE( tc.getOut1Counter() == 3 );
		REQUIRE( tc.getOut2Counter() == 3 );
		REQUIRE( tc.getOut3Counter() == 1 );

		tr.set(3, true);
		REQUIRE( tc.getOut1Counter() == 3 );
		REQUIRE( tc.getOut2Counter() == 3 );
		REQUIRE( tc.getOut3Counter() == 2 );
		CHECK_FALSE( tr.getState(1) );
		CHECK_FALSE( tr.getState(2) );
		CHECK( tr.getState(3) );

		// обращение к несуществующему выходу
		tr.set(10, true);
		REQUIRE( tc.getOut1Counter() == 3 );
		REQUIRE( tc.getOut2Counter() == 3 );
		REQUIRE( tc.getOut3Counter() == 2 );
		CHECK_FALSE( tr.getState(1) );
		CHECK_FALSE( tr.getState(2) );
		CHECK( tr.getState(3) );
		tr.set(-10, true);
		REQUIRE( tc.getOut1Counter() == 3 );
		REQUIRE( tc.getOut2Counter() == 3 );
		REQUIRE( tc.getOut3Counter() == 2 );
		CHECK_FALSE( tr.getState(1) );
		CHECK_FALSE( tr.getState(2) );
		CHECK( tr.getState(3) );
		tr.set(0, true);
		REQUIRE( tc.getOut1Counter() == 3 );
		REQUIRE( tc.getOut2Counter() == 3 );
		REQUIRE( tc.getOut3Counter() == 2 );
		CHECK_FALSE( tr.getState(1) );
		CHECK_FALSE( tr.getState(2) );
		CHECK( tr.getState(3) );
	}
}
