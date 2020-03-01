#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <memory>
#include "UniXML.h"
#include "Configuration.h"
#include "ClickHouseTagsConfig.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// -----------------------------------------------------------------------------
TEST_CASE("ClickHouseTagsConfig", "[tagsconfig]" )
{
	int argc = 1;
	std::string prog = "test";
	char* argv[] = { (char*)prog.c_str() };
	auto conf = uniset_init(argc, argv, "test-clickhouse-config.xml");

	uniset::ClickHouseTagsConfig tconf;

	REQUIRE_NOTHROW( tconf.load(conf) );

	// check no tags
	REQUIRE( tconf.getTags(100500).size() == 0 );
	REQUIRE( tconf.getTags(4).size() == 0 );

	// tags for sensor id="2"

	// update S3 values
	REQUIRE( tconf.updateTags(3,1) );
	auto tags = tconf.getTags(2);
	REQUIRE( tags.size() == 1 );
	REQUIRE( tags[0].key == "tag11" );
	REQUIRE( tags[0].value == "key11" );

	REQUIRE( tconf.updateTags(3,2) );
	tags = tconf.getTags(2);
	REQUIRE( tags.size() == 1 );
	REQUIRE( tags[0].key == "tag12" );
	REQUIRE( tags[0].value == "key12" );

	REQUIRE( tconf.updateTags(3,3) );
	tags = tconf.getTags(2);
	REQUIRE( tags.size() == 1 );
	REQUIRE( tags[0].key == "tag13" );
	REQUIRE( tags[0].value == "key13" );

	REQUIRE( tconf.updateTags(3,4) );
	tags = tconf.getTags(2);
	REQUIRE( tags.size() == 1 );
	REQUIRE( tags[0].key == "tag13" );
	REQUIRE( tags[0].value == "key13" );

	REQUIRE( tconf.updateTags(3,100) );
	tags = tconf.getTags(2);
	REQUIRE( tags.size() == 1 );
	REQUIRE( tags[0].key == "tag13" );
	REQUIRE( tags[0].value == "key13" );

	REQUIRE( tconf.updateTags(3,101) );
	tags = tconf.getTags(2);
	REQUIRE( tags.size() == 0 );

	REQUIRE( tconf.updateTags(3,102) );
	tags = tconf.getTags(2);
	REQUIRE( tags.size() == 1 );
	REQUIRE( tags[0].key == "tag14" );
	REQUIRE( tags[0].value == "key14" );

	REQUIRE( tconf.updateTags(3,10000) );
	tags = tconf.getTags(2);
	REQUIRE( tags.size() == 1 );
	REQUIRE( tags[0].key == "tag14" );
	REQUIRE( tags[0].value == "key14" );

	// пересекающий диапазон
	REQUIRE( tconf.updateTags(3,103) );
	tags = tconf.getTags(2);
	REQUIRE( tags.size() == 2 );
	REQUIRE( tags[0].key == "tag14" );
	REQUIRE( tags[0].value == "key14" );
	REQUIRE( tags[1].key == "tag16" );
	REQUIRE( tags[1].value == "key16" );

	// минимальный
	REQUIRE( tconf.updateTags(3,-5) );
	tags = tconf.getTags(2);
	REQUIRE( tags.size() == 1 );
	REQUIRE( tags[0].key == "tag15" );
	REQUIRE( tags[0].value == "key15" );


	// tags for sensor id="1"
	REQUIRE( tconf.updateTags(4,0) );
	tags = tconf.getTags(1);
	REQUIRE( tags.size() == 0 );

	REQUIRE( tconf.updateTags(4,1) );
	tags = tconf.getTags(1);
	REQUIRE( tags.size() == 2 );
	REQUIRE( tags[0].key == "tag1" );
	REQUIRE( tags[0].value == "key1" );
	REQUIRE( tags[1].key == "tag2" );
	REQUIRE( tags[1].value == "key2" );

	REQUIRE( tconf.updateTags(4,10) );
	tags = tconf.getTags(1);
	REQUIRE( tags.size() == 2 );
	REQUIRE( tags[0].key == "tag1" );
	REQUIRE( tags[0].value == "key1" );
	REQUIRE( tags[1].key == "tag2" );
	REQUIRE( tags[1].value == "key2" );

	REQUIRE( tconf.updateTags(4,0) );
	tags = tconf.getTags(1);
	REQUIRE( tags.size() == 0 );

	// tags for S2 from S3, S4
	REQUIRE( tconf.updateTags(3,3) );
	REQUIRE( tconf.updateTags(4,1) );
	tags = tconf.getTags(2);
	REQUIRE( tags.size() == 2 );
	REQUIRE( tags[0].key == "tag13" );
	REQUIRE( tags[0].value == "key13" );
	REQUIRE( tags[1].key == "tag21" );
	REQUIRE( tags[1].value == "key21" );


	// special test
	REQUIRE( tconf.updateTags(8,1) );
	tags = tconf.getTags(5);
	REQUIRE( tags.size() == 3 );
	REQUIRE( tags[0].key == "mode1" );
	REQUIRE( tags[0].value == "manual" );
	REQUIRE( tags[1].key == "mode2" );
	REQUIRE( tags[1].value == "manual" );
	REQUIRE( tags[2].key == "post" );
	REQUIRE( tags[2].value == "1" );

	REQUIRE( tconf.updateTags(8,2) );
	tags = tconf.getTags(5);
	REQUIRE( tags.size() == 3 );
	REQUIRE( tags[0].key == "mode1" );
	REQUIRE( tags[0].value == "manual" );
	REQUIRE( tags[1].key == "mode2" );
	REQUIRE( tags[1].value == "manual" );
	REQUIRE( tags[2].key == "post" );
	REQUIRE( tags[2].value == "2" );

	REQUIRE( tconf.updateTags(6,1) );
	tags = tconf.getTags(5);
	REQUIRE( tags.size() == 3 );
	REQUIRE( tags[0].key == "mode1" );
	REQUIRE( tags[0].value == "auto" );
	REQUIRE( tags[1].key == "mode2" );
	REQUIRE( tags[1].value == "manual" );
	REQUIRE( tags[2].key == "post" );
	REQUIRE( tags[2].value == "2" );

	// dynamic value
	REQUIRE( tconf.updateTags(9,0) );
	tags = tconf.getTags(10);
	REQUIRE( tags.size() == 1 );
	REQUIRE( tags[0].key == "dyntag" );
	REQUIRE( tags[0].value == "0" );


	REQUIRE( tconf.updateTags(9,1) );
	tags = tconf.getTags(10);
	REQUIRE( tags.size() == 1 );
	REQUIRE( tags[0].key == "dyntag" );
	REQUIRE( tags[0].value == "1" );

	REQUIRE( tconf.updateTags(9,12) );
	tags = tconf.getTags(10);
	REQUIRE( tags.size() == 1 );
	REQUIRE( tags[0].key == "dyntag" );
	REQUIRE( tags[0].value == "11_12_15" );

	REQUIRE( tconf.updateTags(9,16) );
	tags = tconf.getTags(10);
	REQUIRE( tags.size() == 1 );
	REQUIRE( tags[0].key == "dyntag16" );
	REQUIRE( tags[0].value == "myval" );
}
// ----------------------------------------------------------------------------
