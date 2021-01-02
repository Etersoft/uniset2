#include <catch.hpp>

#include "TriggerOR.h"
#include "UniSetTypes.h"

using namespace std;
using namespace uniset;

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

TEST_CASE("TriggerOR", "[TriggerOR]" )
{
    SECTION("Constructor")
    {
        MyTestClass tc;
        TriggerOR<MyTestClass, int> tr(&tc, &MyTestClass::setOut);

        REQUIRE( tc.getNum() == 0 );
        tr.add(1, true);
        tr.add(2, false);
        tr.add(3, false);
        REQUIRE( tc.getNum() == 1 );
        CHECK( tr.state() );
        CHECK( tr.getState(1) );
        CHECK_FALSE( tr.getState(2) );
        CHECK_FALSE( tr.getState(3) );
    }

    SECTION("Working")
    {
        MyTestClass tc;
        TriggerOR<MyTestClass> tr(&tc, &MyTestClass::setOut);
        REQUIRE( tc.getNum() == 0 );
        tr.add(1, true);
        REQUIRE( tc.getNum() == 1 );
        CHECK( tr.state() );
        tr.add(2, false);
        CHECK( tr.state() );
        REQUIRE( tc.getNum() == 1 );
        tr.add(3, false);
        REQUIRE( tc.getNum() == 1 );

        CHECK( tr.state() );
        CHECK( tr.getState(1) );
        CHECK_FALSE( tr.getState(2) );
        CHECK_FALSE( tr.getState(3) );

        tr.reset();
        REQUIRE( tc.getNum() == 2 );
        CHECK_FALSE( tr.state() );
        CHECK( tr.getState(1) );
        CHECK_FALSE( tr.getState(2) );
        CHECK_FALSE( tr.getState(3) );

        tr.commit(1, false);
        tr.commit(2, true);
        CHECK( tr.state() );
        REQUIRE( tc.getNum() == 3 );
        CHECK_FALSE( tr.getState(1) );
        CHECK( tr.getState(2) );
        CHECK_FALSE( tr.getState(3) );

        tr.remove(2);
        CHECK_FALSE( tr.getState(1) );
        CHECK_FALSE( tr.getState(3) );
        CHECK_FALSE( tr.state() );
        REQUIRE( tc.getNum() == 4 );

        // обращение к несуществующему входу
        CHECK_FALSE( tr.getState(10) );
        CHECK_FALSE( tr.getState(-10) );
        CHECK_FALSE( tr.getState(0) );

        tr.commit(10, true);
        CHECK_FALSE( tr.state() );
        tr.commit(-10, true);
        CHECK_FALSE( tr.state() );
        tr.commit(0, true);
        CHECK_FALSE( tr.state() );
    }
}
