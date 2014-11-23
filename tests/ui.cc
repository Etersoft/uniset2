#include <catch.hpp>

#include <time.h>
#include "UInterface.h"
#include "UniSetTypes.h"

using namespace std;
using namespace UniSetTypes;

TEST_CASE("UInterface","[UInterface]")
{
	CHECK( conf!=0 );

	std::string sidName("Input1_S");

	ObjectId testOID = conf->getObjectID("TestProc");
	CHECK( testOID != DefaultObjectId );

	ObjectId sid = conf->getSensorID(sidName);
	CHECK( sid != DefaultObjectId );

	UInterface ui;

	CHECK( ui.getObjectIndex() != nullptr );
	CHECK( ui.getConf() == UniSetTypes::conf );

	CHECK( ui.getConfIOType(sid) != UniversalIO::UnknownIOType );

	REQUIRE_THROWS_AS( ui.getValue(DefaultObjectId), UniSetTypes::ORepFailed );
	REQUIRE_THROWS_AS( ui.getValue(sid), UniSetTypes::Exception );
	REQUIRE_THROWS_AS( ui.getValue(sid, DefaultObjectId), UniSetTypes::Exception );
	REQUIRE_THROWS_AS( ui.getValue(sid, 100), UniSetTypes::Exception );

	REQUIRE_THROWS_AS( ui.resolve(sid), UniSetTypes::ORepFailed );
	REQUIRE_THROWS_AS( ui.resolve(sid,10), UniSetTypes::ORepFailed );
	REQUIRE_THROWS_AS( ui.resolve(sid,DefaultObjectId), UniSetTypes::ORepFailed );

	TransportMessage tm( SensorMessage(sid,10).transport_msg() );
	
	REQUIRE_THROWS_AS( ui.send(testOID,tm), UniSetTypes::Exception );
	REQUIRE_THROWS_AS( ui.send(testOID,tm, -20), UniSetTypes::Exception );
	REQUIRE_THROWS_AS( ui.send(testOID,tm, DefaultObjectId), UniSetTypes::Exception );
	REQUIRE_THROWS_AS( ui.getChangedTime(sid,-20), UniSetTypes::Exception );
	REQUIRE_THROWS_AS( ui.getChangedTime(sid,DefaultObjectId), UniSetTypes::Exception );
	REQUIRE_THROWS_AS( ui.getChangedTime(sid,conf->getLocalNode()), UniSetTypes::Exception );

	CHECK_FALSE( ui.isExist(sid) );
	CHECK_FALSE( ui.isExist(sid,DefaultObjectId) );
	CHECK_FALSE( ui.isExist(sid,100) );

	CHECK_FALSE( ui.waitReady(sid,100,50) );
	CHECK_FALSE( ui.waitReady(sid,300,50,DefaultObjectId) );
	CHECK_FALSE( ui.waitReady(sid,300,50,-20) );
	CHECK_FALSE( ui.waitReady(sid,-1,50) );
	CHECK_FALSE( ui.waitReady(sid,300,-1) );
	
	CHECK_FALSE( ui.waitWorking(sid,100,50) );
	CHECK_FALSE( ui.waitWorking(sid,100,50,DefaultObjectId) );
	CHECK_FALSE( ui.waitWorking(sid,100,50,-20) );
	CHECK_FALSE( ui.waitWorking(sid,-1,50) );
	CHECK_FALSE( ui.waitWorking(sid,100,-1) );

	std::string longName("UNISET_PLC/Sensors/" + sidName);
	CHECK( ui.getIdByName(longName) == sid );
	CHECK( ui.getNameById(sid) == longName );
	CHECK( ui.getTextName(sid) == "Команда 1" );

	CHECK( ui.getNodeId("localhost") == 1000 );
}
