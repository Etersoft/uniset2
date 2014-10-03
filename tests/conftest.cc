#include <catch.hpp>
// --------------------------------------------------------------------------
#include <string>
#include "Configuration.h"
#include "ORepHelpers.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
// --------------------------------------------------------------------------
TEST_CASE( "Configuration", "[Configuration]" )
{
	assert( conf != 0 );
	assert( conf->oind != 0 );

	// смотри tests_with_conf.xml
	ObjectId testID = 1;
	const std::string testSensor("Input1_S");
	const std::string testObject("TestProc");
	const std::string testController("SharedMemory");
	const std::string testService("TimeService");

//	CHECK( conf != 0 );
//	CHECK( conf->oind != 0 );

	SECTION( "ObjectIndex tests.." )
	{
		CHECK( conf->getLocalNode()!=DefaultObjectId );
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

        UniXML_iterator it(cnode);

        CHECK( conf->getArgInt("--prop-id2",it.getProp("id2")) != 0 );
        CHECK( conf->getArgInt("--prop-dummy",it.getProp("id2")) == -100 );
        CHECK( conf->getArgPInt("--prop-id2",it.getProp("id2"),0) != 0 );
        CHECK( conf->getArgPInt("--prop-dummy",it.getProp("dummy"),20) == 20 );
        CHECK( conf->getArgPInt("--prop-dummy",it.getProp("dummy"),0) == 0 );
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

	SECTION( "Empty Constructor" )
	{
		int t_argc = 0;
		char t_argv[]={""};
		REQUIRE_THROWS_AS( Configuration(t_argc,(const char* const*)(t_argv),""), UniSetTypes::SystemError );
	}

	// SECTION( "ObjectIndex Constructor" )
	// SECTION( "ObjectsMap Constructor" )

	
	SECTION( "Bad conf: no <ObjectsMap>" )
	{
		int t_argc = 0;
		char t_argv[]={""};
		ulog.level(Debug::NONE);
		REQUIRE_THROWS_AS( Configuration(t_argc,(const char* const*)(t_argv),"tests_no_objectsmap.xml"), UniSetTypes::SystemError );
	}

	SECTION( "Bad conf: no <UniSet>" )
	{
		int t_argc = 0;
		char t_argv[]={""};
		ulog.level(Debug::NONE);
		REQUIRE_THROWS_AS( Configuration(t_argc,(const char* const*)(t_argv),"tests_no_uniset_section.xml"), UniSetTypes::SystemError );
	}

}

#if 0
    try
    {
        int i1 = uni_atoi("-100");
        cout << "**** check uni_atoi: '-100' " << ( ( i1 != -100 ) ? "FAILED" : "OK" ) << endl;

        int i2 = uni_atoi("20");
        cout << "**** check uni_atoi: '20' " << ( ( i2 != 20 ) ? "FAILED" : "OK" ) << endl;
        
        xmlNode* cnode = conf->getNode("testnode");
        if( cnode == NULL )                                                                                                                                                         
        {                                                                                                                                                                           
            cerr << "<testnode name='testnode'> not found" << endl;                                                                                                                                 
            return 1;                                                                                                                                                               
        }
        
        cout << "**** check conf->getNode function [OK] " << endl;
        
        UniXML_iterator it(cnode); 

        int prop2 = conf->getArgInt("--prop-id2",it.getProp("id2"));
        cerr << "**** check conf->getArgInt(arg1,...): " << ( (prop2 == 0) ? "[FAILED]" : "OK" ) << endl;

        int prop3 = conf->getArgInt("--prop-dummy",it.getProp("id2"));
        cerr << "**** check conf->getArgInt(...,arg2): " << ( (prop3 != -100) ? "[FAILED]" : "OK" ) << endl;
        
        int prop1 = conf->getArgPInt("--prop-id2",it.getProp("id2"),0);
        cerr << "**** check conf->getArgPInt(...): " << ( (prop1 == 0) ? "[FAILED]" : "OK" ) << endl;

        int prop4 = conf->getArgPInt("--prop-dummy",it.getProp("dummy"),20);
        cerr << "**** check conf->getArgPInt(...,...,defval): " << ( (prop4 != 20) ? "[FAILED]" : "OK" ) << endl;

        int prop5 = conf->getArgPInt("--prop-dummy",it.getProp("dummy"),0);
        cerr << "**** check conf->getArgPInt(...,...,defval): " << ( (prop5 != 0) ? "[FAILED]" : "OK" ) << endl;
        


        
        return 0;
    }
    catch(SystemError& err)
    {
        cerr << "(conftest): " << err << endl;
    }
    catch(Exception& ex)
    {
        cerr << "(conftest): " << ex << endl;
    }
    catch(...)
    {
        cerr << "(conftest): catch(...)" << endl;
    }
    
    return 1;
}
#endif