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
    REQUIRE( it.getCurrent() );
    REQUIRE( it.goChildren() );
    REQUIRE( it.findName("item", "localhost", false) );

    REQUIRE( it.getName() == "item" );
    REQUIRE( it.getProp("name") == "localhost" );

    REQUIRE_NOTHROW( MulticastReceiveTransport::createFromXml(it, "192.168.0.1", 0 ) );
    REQUIRE_NOTHROW( MulticastReceiveTransport::createFromXml(it, "192.168.1.1", 2 ) );
    REQUIRE_NOTHROW( MulticastSendTransport::createFromXml(it, "192.168.0.1", 0 ) );
    REQUIRE_NOTHROW( MulticastSendTransport::createFromXml(it, "192.168.1.1", 2 ) );

    auto t1 = MulticastReceiveTransport::createFromXml(it, "192.168.1.1", 2 );
    REQUIRE( t1->toString() == "127.0.1.1:2999" );

    auto t2 = MulticastSendTransport::createFromXml(it, "192.168.1.1", 2 );
    REQUIRE( t2->toString() == "127.0.1.1:2999" );
}
// -----------------------------------------------------------------------------
TEST_CASE("[UNetUDP]: multicast transport", "[unetudp][multicast][transport]")
{
    UniXML xml("unetudp-test-configure.xml");
    UniXML::iterator it = xml.findNode(xml.getFirstNode(), "nodes");
    REQUIRE( it.getCurrent() );
    REQUIRE( it.goChildren() );
    REQUIRE( it.findName("item", "localhost", false) );

    REQUIRE( it.getName() == "item" );
    REQUIRE( it.getProp("name") == "localhost" );

    auto t1 = MulticastReceiveTransport::createFromXml(it, "0.0.0.0", 0 );
    REQUIRE( t1->toString() == "0.0.0.0:3000" );
    REQUIRE( t1->createConnection(false, 5000, true) );

    auto t2 = MulticastSendTransport::createFromXml(it, "127.0.0.1", 0 );
    REQUIRE( t2->toString() == "127.0.0.1:3000" );
    REQUIRE( t2->getGroupAddress() == Poco::Net::SocketAddress("224.0.0.1", 3000) );
    REQUIRE( t2->createConnection(false, 5000) );

    string msg = "hello world";
    REQUIRE( t2->send(msg.data(), msg.size()) == msg.size() );

    unsigned char buf[64];
    REQUIRE( t1->receive(&buf, sizeof(buf)) == msg.size() );
    REQUIRE( string((const char*)buf) == msg );

    REQUIRE( t1->receive(&buf, sizeof(buf)) == -1 );

    msg = "hello world, again";
    REQUIRE( t2->send(msg.data(), msg.size()) == msg.size() );

    memset(buf, 0, sizeof(buf));
    REQUIRE( t1->receive(&buf, sizeof(buf)) == msg.size() );
    REQUIRE( string((const char*)buf) == msg );
}
// -----------------------------------------------------------------------------
