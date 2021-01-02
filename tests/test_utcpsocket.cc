#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <sstream>
#include <iostream>
#include <limits>
#include <Poco/Net/NetException.h>
#include "UniSetTypes.h"
#include "UTCPSocket.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// -----------------------------------------------------------------------------
static const std::string host = "127.0.0.1";
static int port = 40000;
// -----------------------------------------------------------------------------
TEST_CASE("UTCPSocket: create", "[utcpsocket][create]" )
{
    {
        UTCPSocket u;
        Poco::Net::SocketAddress sa(host, port);
        REQUIRE_NOTHROW( u.bind(sa) );
    }

    try
    {
        Poco::Net::ServerSocket so(Poco::Net::SocketAddress(host, port));
    }
    catch( Poco::Net::NetException& ex )
    {
        cerr << ex.displayText() << endl;
    }

    REQUIRE_NOTHROW(UTCPSocket(host, port));
}
// -----------------------------------------------------------------------------
