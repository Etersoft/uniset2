#include <catch.hpp>

#include <thread>
#include <atomic>
#include <future>
#include <ostream>

#include "UTCPSocket.h"
#include "TCPCheck.h"
#include "UniSetTypes.h"
using namespace std;
using namespace uniset;

static int port = 2048;
static std::string host = "localhost";
static atomic_bool cancel = {false};
// --------------------------------------------------------
bool run_test_server()
{
    try
    {
        UTCPSocket sock(host, port);

        while( !cancel )
        {
            if( sock.poll(PassiveTimer::millisecToPoco(500), Poco::Net::Socket::SELECT_READ) )
            {

            }
        }

        return true;
    }
    catch(...) {}

    return false;
}
// --------------------------------------------------------
// вспомогательный класс для гарантированного завершения потока..
class TSRunner
{
    public:
        TSRunner()
        {
            cancel = false;
            res = std::async(std::launch::async, run_test_server);
        }

        ~TSRunner()
        {
            cancel = true;
            res.get();
        }

    private:
        std::future<bool> res;
};
// -----------------------------------------------------------------------------
TEST_CASE("TCPCheck::check", "[tcpcheck][tcpcheck_check]" )
{
    TCPCheck t;
    TSRunner tserv;

    ostringstream ia;
    ia << host << ":" << port;

    msleep(200);
    CHECK( t.check(host, port, 300) );
    CHECK( t.check(ia.str(), 300) );

    CHECK_FALSE( t.check("dummy_host_name", port, 300) );
    CHECK_FALSE( t.check("dummy_host_name:2048", 300) );
}
// --------------------------------------------------------
TEST_CASE("TCPCheck::ping", "[tcpcheck][tcpcheck_ping]" )
{
    TCPCheck t;
    TSRunner tserv;

    msleep(200);
    REQUIRE( t.ping(host) );

    // 123.3. - dummy ip
    REQUIRE_FALSE( t.ping("123.3.") );
}
// --------------------------------------------------------
