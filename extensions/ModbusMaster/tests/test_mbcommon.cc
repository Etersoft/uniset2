#include <catch.hpp>
// -----------------------------------------------------------------------------
#include "MBExchange.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: first bit", "[modbus][firstbit]")
{
    REQUIRE( MBExchange::firstBit(4) == 2 );
    REQUIRE( MBExchange::firstBit(1) == 0 );
    REQUIRE( MBExchange::firstBit(64) == 6 );
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: get bits", "[modbus][getbits]")
{
    REQUIRE( MBExchange::getBits(1, 1, 0) == 1 );
    REQUIRE( MBExchange::getBits(12, 12, 2) == 3 );
    REQUIRE( MBExchange::getBits(9, 12, 2) == 2 );
    REQUIRE( MBExchange::getBits(9, 3, 0) == 1 );
    REQUIRE( MBExchange::getBits(15, 12, 2) == 3 );
    REQUIRE( MBExchange::getBits(15, 3, 0) == 3 );
    REQUIRE( MBExchange::getBits(15, 0, 0) == 15 );
    REQUIRE( MBExchange::getBits(15, 0, 3) == 15 );
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: set bits", "[modbus][setbits]")
{
    REQUIRE( MBExchange::setBits(1, 1, 1, 0) == 1 );
    REQUIRE( MBExchange::setBits(0, 3, 12, 2) == 12 );
    REQUIRE( MBExchange::setBits(3, 3, 12, 2) == 15 );
    REQUIRE( MBExchange::setBits(16, 3, 12, 2) == 28 );
    REQUIRE( MBExchange::setBits(28, 1, 3, 0) == 29 );
    REQUIRE( MBExchange::setBits(28, 10, 0, 0) == 28 );
    REQUIRE( MBExchange::setBits(28, 10, 0, 4) == 28 );
}
// -----------------------------------------------------------------------------
TEST_CASE("MBTCPMaster: force set bits", "[modbus][forcesetbits]")
{
    REQUIRE( MBExchange::forceSetBits(1, 1, 1, 0) == 1 );
    REQUIRE( MBExchange::forceSetBits(0, 3, 12, 2) == 12 );
    REQUIRE( MBExchange::forceSetBits(28, 10, 0, 0) == 10 );
    REQUIRE( MBExchange::forceSetBits(28, 10, 0, 4) == 10 );

    REQUIRE( MBExchange::forceSetBits(0, 2, 3, 0) == 2 );
    REQUIRE( MBExchange::forceSetBits(2, 2, 12, 2) == 10 );

    REQUIRE( MBExchange::forceSetBits(2, 0, 3, 0) == 0 );
    REQUIRE( MBExchange::forceSetBits(12, 0, 12, 2) == 0 );

    REQUIRE( MBExchange::forceSetBits(3, 5, 3, 0) == 1 );
}
// -----------------------------------------------------------------------------