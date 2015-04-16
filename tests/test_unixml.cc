#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <iostream>
// -----------------------------------------------------------------------------
#include "Exceptions.h"
#include "UniXML.h"
#include "UniSetTypes.h"
// -----------------------------------------------------------------------------
using namespace std;
// -----------------------------------------------------------------------------
TEST_CASE("UniXML", "[unixml][basic]" )
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

    xmlNode* tnode = uxml.findNode(uxml.getFirstNode(),"TestData");
    CHECK( tnode != NULL );
    CHECK( uxml.getProp(tnode,"text") == "text" );
    CHECK( uxml.getIntProp(tnode,"x") == 10 );
    CHECK( uxml.getPIntProp(tnode,"y",-20) == 100 );
    CHECK( uxml.getPIntProp(tnode,"zero",20) == 0 );
    CHECK( uxml.getPIntProp(tnode,"negative",20) == -10 );
    CHECK( uxml.getPIntProp(tnode,"unknown",20) == 20 );

    CHECK( uxml.getProp2(tnode,"unknown","def") == "def" );
    CHECK( uxml.getProp2(tnode,"text","def") == "text" );

    // nextNode
    // create
    // remove
    // copy
    // nextNode
}
// -----------------------------------------------------------------------------
TEST_CASE("UniXML::iterator", "[unixml][iterator][basic]" )
{
    UniXML uxml("tests_unixml.xml");
    CHECK( uxml.isOpen() );

    xmlNode* cnode = uxml.findNode(uxml.getFirstNode(),"UniSet");
    CHECK( cnode != NULL );

    UniXML::iterator it(cnode);
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
    CHECK( it.findName("TestNode","TestNode3") != 0 );
    
    CHECK( it.find("subnode") );
    REQUIRE( it.getProp("name") == "Test4" );

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
    CHECK( it.getProp2("text","def") == "text" );
    CHECK( it.getProp2("unknown","def") == "def" );
    CHECK( it.getIntProp("x") == 10 );
    CHECK( it.getPIntProp("y",-20) == 100 );
    CHECK( it.getPIntProp("zero",20) == 0 );
    CHECK( it.getPIntProp("negative",20) == -10 );
    CHECK( it.getPIntProp("unknown",20) == 20 );
}
