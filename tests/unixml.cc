#include <catch.hpp>

#include <iostream>

using namespace std;

#include "Exceptions.h"
#include "UniXML.h"
#include "UniSetTypes.h"

TEST_CASE("UniXML", "[UniXML]" )
{
    SECTION( "Default constructor" ) 
    {
		UniXML xml;
		CHECK_FALSE( xml.isOpen() );
	}

    SECTION( "Bad file" )
    {
		REQUIRE_THROWS_AS( UniXML("unknown.xml"), UniSetTypes::NameNotFound );
		REQUIRE_THROWS_AS( UniXML("tests_unixml_badfile.xml"), UniSetTypes::Exception );
	}
	
	UniXML uxml("tests_unixml.xml");
	CHECK( uxml.isOpen() );

    xmlNode* cnode = uxml.findNode(uxml.getFirstNode(),"UniSet");
    CHECK( cnode != NULL );
	// проверка поиска "вглубь"
	CHECK( uxml.findNode(uxml.getFirstNode(),"Services") != NULL );

	// getProp
	// getIntProp
	// nextNode
	// getPIntProp
	// create
	// remove
	// copy
	// nextNode

	SECTION( "Iterator" );
	{
    	UniXML_iterator it(cnode);
		CHECK( it.getCurrent() != 0 );
		it = uxml.begin();
		CHECK( it.find("UniSet") != 0 );

		// поиск в глубину..
		it = uxml.begin();
		CHECK( it.find("TestProc") != 0 );
		
		it = uxml.begin();
		CHECK( it.getName() == "UNISETPLC" );
		it.goChildren();
		CHECK( it.getName() == "UserData" );

		it += 4;
		CHECK( it.getName() == "settings" );

		it -= 4;
		CHECK( it.getName() == "UserData" );

		it = it + 4;
		CHECK( it.getName() == "settings" );

		it = it - 4;
		CHECK( it.getName() == "UserData" );

		it++;
		CHECK( it.getName() == "UniSet" );

		it--;
		CHECK( it.getName() == "UserData" );

		++it;
		CHECK( it.getName() == "UniSet" );

		--it;
		CHECK( it.getName() == "UserData" );

		it = uxml.begin();
		CHECK( it.findName("TestNode","TestNode1") != 0 );
		it = uxml.begin();
		CHECK( it.findName("TestNode","TestNode2") != 0 );

		it = uxml.begin();
		it.goChildren();
		CHECK( it.getName() == "UserData" );
		it.goParent();
		CHECK( it.getName() == "UNISETPLC" );

		it = uxml.begin();
		it.goChildren();
		it.goEnd();
		CHECK( it.getName() == "EndSection" );
		it.goBegin();
		CHECK( it.getName() == "UserData" );

		it = uxml.begin();
		CHECK( it.find("TestData") != 0 );

		CHECK( it.getProp("text") == "text" );
		CHECK( it.getIntProp("x") == 10 );
		CHECK( it.getPIntProp("y",-20) == 100 );
		CHECK( it.getPIntProp("unknown",20) == 20 );
	}
}
