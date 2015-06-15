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
		REQUIRE( !strcmp(str, str2) );
		delete[] str2;
	}
}
// -----------------------------------------------------------------------------
TEST_CASE("UniSetTypes: explode", "[utypes][explode]" )
{
	const std::string str1("id1/wed/wedwed/");

	auto t1 = UniSetTypes::explode_str(str1, '/');
	CHECK( t1.size() == 3 );

	auto t2 = UniSetTypes::explode_str(str1, '.');
	CHECK( t2.size() == 1 );

	const std::string str2("id1/wed/wedwed/f");

	auto t3 = UniSetTypes::explode_str(str2, '/');
	CHECK( t3.size() == 4 );

	const std::string str3("/id1/wed/wedwed/");

	auto t4 = UniSetTypes::explode_str(str3, '/');
	CHECK( t4.size() == 3 );


	const std::string str4("");
	auto t5 = UniSetTypes::explode_str(str4, '/');
	CHECK( t5.size() == 0 );

	const std::string str5("/");
	auto t6 = UniSetTypes::explode_str(str5, '/');
	CHECK( t6.size() == 0 );
}
// -----------------------------------------------------------------------------
TEST_CASE("UniSetTypes: getSInfoList", "[utypes][getsinfo]" )
{
	const std::string str1("Input4_S@node2,Input1_S,5,5@node3,6@1001");

	auto t1 = UniSetTypes::getSInfoList(str1);

	CHECK( t1.size() == 5 );

	vector<UniSetTypes::ParamSInfo> v(t1.begin(),t1.end());

	REQUIRE( v[0].si.id == 141 );
	REQUIRE( v[0].si.node == 1001 );

	REQUIRE( v[1].si.id == 1 );
	REQUIRE( v[1].si.node == DefaultObjectId );

	REQUIRE( v[2].si.id == 5 );
	REQUIRE( v[2].si.node == DefaultObjectId );

	REQUIRE( v[3].si.id == 5 );
	REQUIRE( v[3].si.node == 1002 );

	REQUIRE( v[4].si.id == 6 );
	REQUIRE( v[4].si.node == 1001 );
}
// -----------------------------------------------------------------------------
TEST_CASE("UniSetTypes: getObjectsList", "[utypes][getolist]" )
{
	const std::string str1("TestProc@node2,TestProc,102,102@node3,103@1001");

	auto t1 = UniSetTypes::getObjectsList(str1);

	CHECK( t1.size() == 5 );

	vector<UniSetTypes::ConsumerInfo> v(t1.begin(),t1.end());

	REQUIRE( v[0].id == 100 );
	REQUIRE( v[0].node == 1001 );

	REQUIRE( v[1].id == 100 );
	REQUIRE( v[1].node == DefaultObjectId );

	REQUIRE( v[2].id == 102 );
	REQUIRE( v[2].node == DefaultObjectId );

	REQUIRE( v[3].id == 102 );
	REQUIRE( v[3].node == 1002 );

	REQUIRE( v[4].id == 103 );
	REQUIRE( v[4].node == 1001 );
}
// -----------------------------------------------------------------------------
