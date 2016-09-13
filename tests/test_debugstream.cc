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
