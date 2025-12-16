#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <sstream>
#include <limits>
#include "Configuration.h"
#include "UniSetTypes.h"
#include "UHttpRequestHandler.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
using namespace uniset::UHttp;
// -----------------------------------------------------------------------------
TEST_CASE("UHttp", "[uhttp]" )
{
    NetworkRule wlRule;
    wlRule.address = Poco::Net::IPAddress("192.168.1.0");
    wlRule.prefixLength = 24;

    NetworkRule wlRule2;
    wlRule2.address = Poco::Net::IPAddress("10.0.0.0");
    wlRule2.prefixLength = 8;

    NetworkRule blRule;
    blRule.address = Poco::Net::IPAddress("192.168.1.100");
    blRule.prefixLength = 32;
    blRule.isRange = false;

    SECTION("match checks ip in single rule")
    {
        REQUIRE(UHttpRequestHandler::match(Poco::Net::IPAddress("192.168.1.5"), wlRule));
        REQUIRE_FALSE(UHttpRequestHandler::match(Poco::Net::IPAddress("192.168.2.1"), wlRule));
    }

    SECTION("inRules finds any matching rule")
    {
        NetworkRules wl { wlRule, wlRule2 };
        REQUIRE(UHttpRequestHandler::inRules(Poco::Net::IPAddress("10.1.2.3"), wl));
        REQUIRE_FALSE(UHttpRequestHandler::inRules(Poco::Net::IPAddress("172.16.0.1"), wl));
    }

    SECTION("isDenied respects blacklist")
    {
        NetworkRules wl;
        NetworkRules bl { blRule };
        REQUIRE(UHttpRequestHandler::isDenied(Poco::Net::IPAddress("192.168.1.100"), wl, bl));
        REQUIRE_FALSE(UHttpRequestHandler::isDenied(Poco::Net::IPAddress("8.8.8.8"), wl, bl));
    }

    SECTION("isDenied respects whitelist")
    {
        NetworkRules wl { wlRule };
        NetworkRules bl;
        REQUIRE_FALSE(UHttpRequestHandler::isDenied(Poco::Net::IPAddress("192.168.1.50"), wl, bl));
        REQUIRE(UHttpRequestHandler::isDenied(Poco::Net::IPAddress("10.0.0.1"), wl, bl));
    }

    SECTION("blacklist overrides whitelist")
    {
        NetworkRules wl { wlRule };
        NetworkRules bl { blRule };
        REQUIRE(UHttpRequestHandler::isDenied(Poco::Net::IPAddress("192.168.1.100"), wl, bl));
        REQUIRE_FALSE(UHttpRequestHandler::isDenied(Poco::Net::IPAddress("192.168.1.1"), wl, bl));
    }

    SECTION("range rules are supported")
    {
        NetworkRule rangeRule;
        rangeRule.address = Poco::Net::IPAddress("172.16.0.10");
        rangeRule.rangeEnd = Poco::Net::IPAddress("172.16.0.20");
        rangeRule.isRange = true;

        NetworkRules wl;
        NetworkRules bl { rangeRule };

        REQUIRE(UHttpRequestHandler::match(Poco::Net::IPAddress("172.16.0.15"), rangeRule));
        REQUIRE_FALSE(UHttpRequestHandler::match(Poco::Net::IPAddress("172.16.0.25"), rangeRule));

        REQUIRE(UHttpRequestHandler::isDenied(Poco::Net::IPAddress("172.16.0.10"), wl, bl));
        REQUIRE(UHttpRequestHandler::isDenied(Poco::Net::IPAddress("172.16.0.20"), wl, bl));
        REQUIRE_FALSE(UHttpRequestHandler::isDenied(Poco::Net::IPAddress("172.16.0.9"), wl, bl));
    }
}
// -----------------------------------------------------------------------------
