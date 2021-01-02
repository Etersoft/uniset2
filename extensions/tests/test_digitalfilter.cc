#include <catch.hpp>
// -----------------------------------------------------------------------------
#include "Exceptions.h"
#include "Extensions.h"
#include "DigitalFilter.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
using namespace uniset::extensions;
// -----------------------------------------------------------------------------
TEST_CASE("[DigitalFilter]: default", "[DigitalFilter]")
{
    SECTION("..")
    {
        WARN("List of tests for [DigitalFilter] is not complete (is not sufficient)");
    }

    SECTION("Default constructor (const data)")
    {
        DigitalFilter df;
        DigitalFilter df10(10);

        REQUIRE( df.size() == 5 );
        REQUIRE( df10.size() == 10 );

        for( int i = 0; i < 20; i++ )
        {
            df.add(50);
            df10.add(50);
        }

        REQUIRE( df.current1() == 50 );
        REQUIRE( df.currentRC() == 50 );
        REQUIRE( df.currentMedian() == 50 );

        REQUIRE( df10.current1() == 50 );
        REQUIRE( df10.currentRC() == 50 );
        REQUIRE( df10.currentMedian() == 50 );
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("[DigitalFilter]: median", "[DigitalFilter][median]")
{
    DigitalFilter df;

    for( int i = 0; i < 20; i++ )
        df.median(50);

    REQUIRE( df.currentMedian() == 50 );

    DigitalFilter df1;
    DigitalFilter df10;
    vector<long> dat = {0, 234, 356, 344, 234, 320, 250, 250, 250, 250, 250, 250, 250, 251, 252, 251, 252, 252, 250};

    for( auto v : dat )
    {
        df1.median(v);
        df10.median(v);
    }

    REQUIRE( df1.currentMedian() == 252 );
    REQUIRE( df10.currentMedian() == 252 );
}
// -----------------------------------------------------------------------------
TEST_CASE("[DigitalFilter]: filter1", "[DigitalFilter][filter1]")
{
    DigitalFilter df1;
    DigitalFilter df10;
    // "выброс" за СКО отсекается..
    vector<long> dat = {10, 12, 10, -8, 10, 10, -230, 10, 10};

    for( auto v : dat )
    {
        df1.add(v);
        df10.add(v);
    }

    REQUIRE( df1.current1() == 10 );
    REQUIRE( df10.current1() == 10 );
}
// -----------------------------------------------------------------------------
TEST_CASE("[DigitalFilter]: filterRC", "[DigitalFilter][filterRC]")
{
    double Ti = 0.09; // постоянная времени фильтра
    DigitalFilter df1(5, Ti);
    DigitalFilter df10(10, Ti);
    vector<long> dat = {10, 12, 10, -8, 10, 10, -230, 10, 10, 12, 12, 10, -8, -8, 10, 11, 9, 11, 11, 11, 9, 12, 12, 10, 10, 11, -4560, 12, 10, 10, 11, 10, 10, 10, 10};

    for( auto v : dat )
    {
        df1.add(v);
        df10.add(v);
        msleep(30);
    }

    REQUIRE( df1.currentRC() == 10 );
    REQUIRE( df10.currentRC() == 10 );
}
