#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <memory>
#include "UniSetTypes.h"
#include "UInterface.h"
#include "UDPPacket.h"
#include "UDPCore.h"
#include "MulticastTransport.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// -----------------------------------------------------------------------------
static int port = 3000;
static const std::string host("127.255.255.255");
// -----------------------------------------------------------------------------
TEST_CASE("[UNetUDP]: multicast setup", "[unetudp][multicast][config]")
{
    UniXML xml("unetudp-test-configure.xml");
    UniXML::iterator it = xml.findNode(xml.getFirstNode(), "nodes");
    UniXML::iterator root = it;

    REQUIRE( it.getCurrent() );
    REQUIRE( it.goChildren() );
    REQUIRE( it.findName("item", "localhost", false) );

    REQUIRE( it.getName() == "item" );
    REQUIRE( it.getProp("name") == "localhost" );

    REQUIRE_NOTHROW( MulticastReceiveTransport::createFromXml(root, it, 0 ) );
    REQUIRE_NOTHROW( MulticastReceiveTransport::createFromXml(root, it, 2 ) );
    REQUIRE_NOTHROW( MulticastSendTransport::createFromXml(root, it, 0 ) );
    REQUIRE_NOTHROW( MulticastSendTransport::createFromXml(root, it, 2 ) );

    auto t1 = MulticastReceiveTransport::createFromXml(root, it, 2);
    REQUIRE( t1->toString() == "225.0.0.1:3030" );

    auto t2 = MulticastSendTransport::createFromXml(root, it, 2);
    REQUIRE( t2->toString() == "127.0.0.1:3030" );
}
// -----------------------------------------------------------------------------
TEST_CASE("[UNetUDP]: unet2", "[unetudp][multicast][unet2]")
{
    UniXML xml("unetudp-test-configure.xml");
    UniXML::iterator eit = xml.findNode(xml.getFirstNode(), "UNetExchange", "UNetExchange2");
    REQUIRE( eit.getCurrent() );
    REQUIRE( eit.goChildren() );
    UniXML::iterator it = eit;
    REQUIRE( it.find("unet2") );
    REQUIRE( it.getCurrent() );
    UniXML::iterator root = it;

    REQUIRE( it.goChildren() );
    REQUIRE( it.findName("item", "localhost", false) );

    REQUIRE( it.getName() == "item" );
    REQUIRE( it.getProp("name") == "localhost" );

    auto t1 = MulticastReceiveTransport::createFromXml(root, it, 0 );
    REQUIRE( t1->toString() == "224.0.0.1:3000" );
    REQUIRE( t1->createConnection(false, 5000, true) );

    auto t2 = MulticastSendTransport::createFromXml(root, it, 0 );
    REQUIRE( t2->toString() == "127.0.0.1:3000" );
    REQUIRE( t2->getGroupAddress() == Poco::Net::SocketAddress("224.0.0.1", 3000) );

    auto t3 = MulticastReceiveTransport::createFromXml(root, it, 2 );
    REQUIRE( t3->toString() == "225.0.0.1:3000" );
    REQUIRE( t3->createConnection(false, 5000, true) );

    auto t4 = MulticastSendTransport::createFromXml(root, it, 2 );
    REQUIRE( t4->toString() == "127.0.0.1:3000" );
    REQUIRE( t4->getGroupAddress().toString() == Poco::Net::SocketAddress("225.0.0.1", 3000).toString() );
}
// -----------------------------------------------------------------------------

