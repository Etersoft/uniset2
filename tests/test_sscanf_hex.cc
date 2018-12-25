#include <catch.hpp>

#include <stdio.h>

int check( const char* t )
{
	int r = 0;
	sscanf(t, "%i", &r);
	return r;
}

TEST_CASE("sscanf hex", "[sscanf hex]")
{
	REQUIRE( check("100") == 100 );
	REQUIRE( check("0x100") == 0x100 );
	REQUIRE( check("0xFF") == 0xff );
	REQUIRE( check("010") == 010 ); // -V536
	REQUIRE( check("-10") == -10 );
	REQUIRE( check("-1000") == -1000 );
	REQUIRE( check("") == 0 );
}
