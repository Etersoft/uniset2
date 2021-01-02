#include <catch.hpp>

#include <time.h>
#include "UInterface.h"
#include "UniSetTypes.h"

using namespace std;
using namespace uniset;

TEST_CASE("UInterface", "[UInterface]")
{
    auto conf = uniset_conf();
    CHECK( conf != nullptr );

    std::string sidName("Input1_S");

    ObjectId testOID = conf->getObjectID("TestProc");
    CHECK( testOID != DefaultObjectId );

    ObjectId sid = conf->getSensorID(sidName);
    CHECK( sid != DefaultObjectId );

    UInterface ui;

    CHECK( ui.getObjectIndex() != nullptr );
    CHECK( ui.getConf() == conf );

    CHECK( ui.getConfIOType(sid) != UniversalIO::UnknownIOType );

    REQUIRE_THROWS_AS( ui.getValue(DefaultObjectId), uniset::ORepFailed& );
    REQUIRE_THROWS_AS( ui.getValue(sid), uniset::Exception& );
    REQUIRE_THROWS_AS( ui.getValue(sid, DefaultObjectId), uniset::Exception& );
    REQUIRE_THROWS_AS( ui.getValue(sid, 100), uniset::Exception& );

    REQUIRE_THROWS_AS( ui.resolve(sid), uniset::ORepFailed& );
    REQUIRE_THROWS_AS( ui.resolve(sid, 10), uniset::ResolveNameError& );
    REQUIRE_THROWS_AS( ui.resolve(sid, DefaultObjectId), ResolveNameError& );

    TransportMessage tm( SensorMessage(sid, 10).transport_msg() );

    REQUIRE_THROWS_AS( ui.send(testOID, tm), uniset::Exception& );
    REQUIRE_THROWS_AS( ui.send(testOID, tm, -20), uniset::Exception& );
    REQUIRE_THROWS_AS( ui.send(testOID, tm, DefaultObjectId), uniset::Exception& );
    REQUIRE_THROWS_AS( ui.getTimeChange(sid, -20), uniset::Exception& );
    REQUIRE_THROWS_AS( ui.getTimeChange(sid, DefaultObjectId), uniset::Exception& );
    REQUIRE_THROWS_AS( ui.getTimeChange(sid, conf->getLocalNode()), uniset::Exception& );
    REQUIRE_THROWS_AS( ui.sendText(testOID, "hello", 1), uniset::Exception& );
    REQUIRE_THROWS_AS( ui.sendText(testOID, "hello", 1, -20), uniset::Exception& );

    CHECK_FALSE( ui.isExist(sid) );
    CHECK_FALSE( ui.isExist(sid, DefaultObjectId) );
    CHECK_FALSE( ui.isExist(sid, 100) );

    CHECK_FALSE( ui.waitReady(sid, 100, 50) );
    CHECK_FALSE( ui.waitReady(sid, 300, 50, DefaultObjectId) );
    CHECK_FALSE( ui.waitReady(sid, 300, 50, -20) );
    CHECK_FALSE( ui.waitReady(sid, -1, 50) );
    CHECK_FALSE( ui.waitReady(sid, 300, -1) );

    CHECK_FALSE( ui.waitWorking(sid, 100, 50) );
    CHECK_FALSE( ui.waitWorking(sid, 100, 50, DefaultObjectId) );
    CHECK_FALSE( ui.waitWorking(sid, 100, 50, -20) );
    CHECK_FALSE( ui.waitWorking(sid, -1, 50) );
    CHECK_FALSE( ui.waitWorking(sid, 100, -1) );

    std::string longName("UNISET_PLC/Sensors/" + sidName);
    CHECK( ui.getIdByName(longName) == sid );
    CHECK( ui.getNameById(sid) == longName );
    CHECK( ui.getTextName(sid) == "Команда 1" );

    CHECK( ui.getNodeId("localhost") == 1000 );
}
