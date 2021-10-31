#include <catch.hpp>
// --------------------------------------------------------------------------
#include <string>
#include "Configuration.h"
#include "ORepHelpers.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// --------------------------------------------------------------------------
TEST_CASE( "Configuration", "[Configuration]" )
{
    auto conf = uniset_conf();

    assert( conf != nullptr );
    assert( conf->oind != nullptr );

    // смотри tests_with_conf.xml
    ObjectId testID = 1;
    const std::string testSensor("Input1_S");
    const std::string testObject("TestProc");
    const std::string testController("SharedMemory");
    const std::string testService("TimeService");

    //    CHECK( conf != 0 );
    //    CHECK( conf->oind != 0 );

    SECTION( "ObjectIndex tests.." )
    {
        CHECK( conf->getLocalNode() != DefaultObjectId );
        CHECK( conf->oind->getTextName(testID) != "" );

        string ln("/Projects/Sensors/VeryVeryLongNameSensor_ForTest_AS");
        string ln_short("VeryVeryLongNameSensor_ForTest_AS");
        REQUIRE( ORepHelpers::getShortName(ln) == ln_short );

        CHECK( conf->oind->getNameById(testID) != "" );

        string mn(conf->oind->getMapName(testID));
        CHECK( mn != "" );
        CHECK( conf->oind->getIdByName(mn) != DefaultObjectId );

        CHECK( conf->getIOType(testID) != UniversalIO::UnknownIOType );
        CHECK( conf->getIOType(mn) != UniversalIO::UnknownIOType );
        CHECK( conf->getIOType(testSensor) != UniversalIO::UnknownIOType );
    }

    SECTION( "Arguments" )
    {
        xmlNode* cnode = conf->getNode("testnode");
        CHECK( cnode != NULL );

        UniXML::iterator it(cnode);

        CHECK( conf->getArgInt("--prop-id2", it.getProp("id2")) != 0 );
        CHECK( conf->getArgInt("--prop-dummy", it.getProp("id2")) == -100 );
        CHECK( conf->getArgPInt("--prop-id2", it.getProp("id2"), 0) != 0 );
        CHECK( conf->getArgPInt("--prop-dummy", it.getProp("dummy"), 20) == 20 );
        CHECK( conf->getArgPInt("--prop-dummy", it.getProp("dummy"), 0) == 0 );
    }

    SECTION( "XML sections" )
    {
        CHECK( conf->getXMLSensorsSection() != 0 );
        CHECK( conf->getXMLObjectsSection() != 0 );
        CHECK( conf->getXMLControllersSection() != 0 );
        CHECK( conf->getXMLServicesSection() != 0 );
        CHECK( conf->getXMLNodesSection() != 0 );

        CHECK( conf->getSensorID(testSensor) != DefaultObjectId );
        CHECK( conf->getObjectID(testObject) != DefaultObjectId );
        CHECK( conf->getControllerID(testController) != DefaultObjectId );
        CHECK( conf->getServiceID(testService) != DefaultObjectId );
    }

    SECTION( "Default parameters" )
    {
        REQUIRE( conf->getNCReadyTimeout() == 60000 );
        REQUIRE( conf->getHeartBeatTime() == 2000 );
        REQUIRE( conf->getCountOfNet() == 1 );
        REQUIRE( conf->getRepeatCount() == 3 );
        REQUIRE( conf->getRepeatTimeout() == 50 );
        REQUIRE( conf->getStartupIgnoreTimeout() == 6000 );
    }

    SECTION( "Empty Constructor" )
    {
        int t_argc = 0;
        char t_argv[] = {""};
        REQUIRE_THROWS_AS( Configuration(t_argc, (const char* const*)(t_argv), ""), uniset::SystemError );
    }

    // SECTION( "ObjectIndex Constructor" )
    // SECTION( "ObjectsMap Constructor" )


    SECTION( "Bad conf: no <ObjectsMap>" )
    {
        int t_argc = 0;
        char t_argv[] = {""};
        ulog()->level(Debug::NONE);
        REQUIRE_THROWS_AS( Configuration(t_argc, (const char* const*)(t_argv), "tests_no_objectsmap.xml"), uniset::SystemError );
    }

    SECTION( "Bad conf: no <UniSet>" )
    {
        int t_argc = 0;
        char t_argv[] = {""};
        ulog()->level(Debug::NONE);
        REQUIRE_THROWS_AS( Configuration(t_argc, (const char* const*)(t_argv), "tests_no_uniset_section.xml"), uniset::SystemError );
    }

}
