#include <catch.hpp>

#include <thread>
#include <atomic>
#include <future>
#include <ostream>

#include "TCPCheck.h"
#include "UniSetTypes.h"
using namespace std;

static int port = 2048;
static std::string host = "localhost";
static atomic_bool cancel = {false};
// --------------------------------------------------------
bool run_test_server()
{
	ost::InetAddress addr = host.c_str();
	ost::TCPSocket sock(addr, port);

	while( !cancel )
	{
		if( sock.isPendingConnection(500) ) {}
	}

	return true;
}
// --------------------------------------------------------
TEST_CASE("TCPCheck::check", "[tcpcheck][tcpcheck_check]" )
{
	TCPCheck t;

	auto res = std::async(std::launch::async, run_test_server);

	ostringstream ia;
	ia << host << ":" << port;

	CHECK( t.check(host, port, 300) );
	CHECK( t.check(ia.str(), 300) );

	CHECK_FALSE( t.check("dummy_host_name", port, 300) );
	CHECK_FALSE( t.check("dummy_host_name:2048", 300) );

	cancel = true;
	CHECK( res.get() );
}
// --------------------------------------------------------
TEST_CASE("TCPCheck::ping", "[tcpcheck][tcpcheck_ping]" )
{
	TCPCheck t;

	auto res = std::async(std::launch::async, run_test_server);

	CHECK( t.ping(host) );
	CHECK_FALSE( t.ping("dummy_host_name") );

	cancel = true;
	CHECK( res.get() );
}
// --------------------------------------------------------
