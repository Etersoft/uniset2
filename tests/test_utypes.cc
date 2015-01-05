#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <sstream>
#include <limits>
#include "UniSetTypes.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
// -----------------------------------------------------------------------------
TEST_CASE("UniSetTypes: uni_atoi", "[utypes][uni_atoi]" ) 
{
    SECTION("int")
    {
        REQUIRE( uni_atoi("100") == 100 );
        REQUIRE( uni_atoi("-100") == -100 );
        REQUIRE( uni_atoi("0") == 0 );
        REQUIRE( uni_atoi("-0") == 0 );

        ostringstream imax;
        imax << std::numeric_limits<int>::max();
        REQUIRE( uni_atoi(imax.str()) == std::numeric_limits<int>::max() );

        ostringstream imin;
        imin << std::numeric_limits<int>::min();
        REQUIRE( uni_atoi(imin.str()) == std::numeric_limits<int>::min() );
    }

    SECTION("unsigned int")
    {
        ostringstream imax;
        imax << std::numeric_limits<unsigned int>::max();

        unsigned int uint_max_val = (unsigned int)uni_atoi(imax.str());
    
        REQUIRE(  uint_max_val == std::numeric_limits<unsigned int>::max() );

        ostringstream imin;
        imax << std::numeric_limits<int>::min();

        unsigned int uint_min_val = (unsigned int)uni_atoi(imin.str()); // "0"?

        REQUIRE( uint_min_val == std::numeric_limits<unsigned int>::min() );
    }

    SECTION("hex")
    {
        REQUIRE( uni_atoi("0xff") == 0xff );
        REQUIRE( uni_atoi("0xffff") == 0xffff );
        REQUIRE( uni_atoi("0x0") == 0 );
        REQUIRE( (unsigned int)uni_atoi("0xffffffff") == 0xffffffff );
        REQUIRE( uni_atoi("0xfffffff8") == 0xfffffff8 );
    }

    WARN("Tests for 'UniSetTypes' incomplete...");
}
// -----------------------------------------------------------------------------
TEST_CASE("UniSetTypes: uni_strdup", "[utypes][uni_strdup]" )
{
    SECTION("uni_strdup string")
    {
        string str("Test string");
        const char* cp = uni_strdup(str);
        string str2(cp);
        REQUIRE( str == str2 );
        delete[] cp;
    }

    SECTION("uni_strdup char*")
    {
        const char* str = "Test string";
        char* str2 = uni_strdup(str);
        REQUIRE( !strcmp(str,str2) );
        delete[] str2;
    }
}
// -----------------------------------------------------------------------------
