#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <sstream>
#include "DebugStream.h"
// -----------------------------------------------------------------------------
using namespace std;
// -----------------------------------------------------------------------------
//! \todo Дописать тесты по DebugStream
// -----------------------------------------------------------------------------
TEST_CASE("Debugstream: set levels", "[debugstream][set]" )
{
	DebugStream d;

	d.level(Debug::value("level1"));
	REQUIRE(d.is_level1());

	d.level( Debug::value("level2"));
	REQUIRE(d.is_level2());
	REQUIRE_FALSE(d.is_level1());

	d.level( Debug::type(Debug::LEVEL1 | Debug::LEVEL2) );
	REQUIRE(d.is_level2());
	REQUIRE(d.is_level1());

	d.level(Debug::value("level1,level2"));
	REQUIRE(d.is_level2());
	REQUIRE(d.is_level1());

	d.level(Debug::value("level1,-level2"));
	REQUIRE(d.is_level1());
	REQUIRE_FALSE(d.is_level2());

	d.level(Debug::value("any,-level2"));
	REQUIRE(d.is_level1());
	REQUIRE(d.is_level9());
	REQUIRE_FALSE(d.is_level2());
}
// -----------------------------------------------------------------------------
TEST_CASE("Debugstream: add levels", "[debugstream][add]" )
{
	DebugStream d;

	d.level(Debug::LEVEL1);
	REQUIRE(d.is_level1());

	d.addLevel(Debug::LEVEL2);
	REQUIRE(d.is_level1());
	REQUIRE(d.is_level2());

	d.addLevel(Debug::value("level3"));
	REQUIRE(d.is_level1());
	REQUIRE(d.is_level2());
	REQUIRE(d.is_level3());
}
// -----------------------------------------------------------------------------
TEST_CASE("Debugstream: del levels", "[debugstream][del]" )
{
	DebugStream d;

	d.level(Debug::ANY);
	REQUIRE(d.is_level1());
	REQUIRE(d.is_level2());

	d.delLevel(Debug::LEVEL1);
	REQUIRE_FALSE(d.is_level1());
	REQUIRE(d.is_level2());

	d.delLevel(Debug::value("level2"));
	REQUIRE_FALSE(d.is_level1());
	REQUIRE_FALSE(d.is_level2());
	REQUIRE(d.is_level3());
}
// -----------------------------------------------------------------------------
static ostringstream test_log_str;

void test_log_buffer(const std::string& txt )
{
	test_log_str << txt;
}

TEST_CASE("Debugstream: verbose", "[debugstream][verbose]" )
{
	DebugStream d(Debug::INFO);
	d.verbose(0);
	REQUIRE(d.verbose() == 0);

	d.signal_stream_event().connect( &test_log_buffer );

	d.V(1)[Debug::INFO] << "text" << endl;
	REQUIRE(test_log_str.str() == "" );

	test_log_str.str(""); // clean
	d.verbose(1);
	d.V(1)(Debug::INFO) << "text";
	d.V(2)(Debug::INFO) << "text2";
	REQUIRE(test_log_str.str() == "text" );

	test_log_str.str(""); // clean
	d.verbose(2);
	d.V(2)(Debug::INFO) << "text";
	d.V(100)(Debug::INFO) << "text100";
	REQUIRE( test_log_str.str() == "text" );

	test_log_str.str(""); // clean
	d.verbose(0);
	d.V(1).info() << "text";
	d.V(2).info() << "text2";
	d.V(0).warn() << "text warning";
	REQUIRE(test_log_str.str() == "" );
}
// -----------------------------------------------------------------------------
