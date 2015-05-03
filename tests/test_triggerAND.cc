#include <catch.hpp>

#include "TriggerAND.h"
#include "UniSetTypes.h"
using namespace std;

class MyTestClass
{
	public:
		MyTestClass(): num(0) {}
		~MyTestClass() {}

		void setOut( bool newstate )
		{
			num++;
		}

		inline int getNum()
		{
			return num;
		}

	private:
		int num;
};

TEST_CASE("TriggerAND", "[TriggerAND]" )
{
	SECTION("Constructor")
	{
		MyTestClass tc;
		TriggerAND<MyTestClass> tr(&tc, &MyTestClass::setOut);

		tr.add(1, true); // в этот момент вход всего один и он TRUE поэтому тут out=TRUE
		REQUIRE( tc.getNum() == 1 );
		CHECK( tr.state() );

		tr.add(2, false);
		REQUIRE( tc.getNum() == 2 );
		CHECK_FALSE( tr.state() );

		tr.add(3, false);
		REQUIRE( tc.getNum() == 2 );
		CHECK_FALSE( tr.state() );

		CHECK( tr.getState(1) );
		CHECK_FALSE( tr.getState(2) );
		CHECK_FALSE( tr.getState(3) );
	}

	SECTION("Working")
	{
		MyTestClass tc;
		TriggerAND<MyTestClass> tr(&tc, &MyTestClass::setOut);

		tr.add(1, false);
		tr.add(2, false);
		tr.add(3, false);
		REQUIRE( tc.getNum() == 0 );
		CHECK_FALSE( tr.state() );
		CHECK_FALSE( tr.getState(1) );
		CHECK_FALSE( tr.getState(2) );
		CHECK_FALSE( tr.getState(3) );

		tr.commit(1, true);
		tr.commit(2, true);
		REQUIRE( tc.getNum() == 0 );
		CHECK_FALSE( tr.state() );
		CHECK( tr.getState(1) );
		CHECK( tr.getState(2) );
		CHECK_FALSE( tr.getState(3) );

		tr.commit(3, true);
		REQUIRE( tc.getNum() == 1 );
		CHECK( tr.state() );
		CHECK( tr.getState(1) );
		CHECK( tr.getState(2) );
		CHECK( tr.getState(3) );

		tr.commit(2, false);
		CHECK_FALSE( tr.state() );
		REQUIRE( tc.getNum() == 2 );
		CHECK( tr.getState(1) );
		CHECK_FALSE( tr.getState(2) );
		CHECK( tr.getState(3) );

		tr.commit(1, false);
		tr.commit(3, false);
		CHECK_FALSE( tr.state() );
		REQUIRE( tc.getNum() == 2 );
		CHECK_FALSE( tr.getState(1) );
		CHECK_FALSE( tr.getState(2) );
		CHECK_FALSE( tr.getState(3) );

		tr.commit(1, true);
		tr.commit(3, true);
		tr.commit(2, true);
		CHECK( tr.state() );
		REQUIRE( tc.getNum() == 3 );

		tr.commit(2, false);
		CHECK_FALSE( tr.state() );
		REQUIRE( tc.getNum() == 4 );

		tr.remove(2);
		CHECK( tr.getState(1) );
		CHECK( tr.getState(3) );
		CHECK( tr.state() );
		REQUIRE( tc.getNum() == 5 );

		// обращение к несуществующему входу
		CHECK_FALSE( tr.getState(10) );
		CHECK_FALSE( tr.getState(-10) );
		CHECK_FALSE( tr.getState(0) );

		tr.commit(10, false);
		CHECK( tr.state() );
		tr.commit(-10, false);
		CHECK( tr.state() );
		tr.commit(0, false);
		CHECK( tr.state() );
	}
}
