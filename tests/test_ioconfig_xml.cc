#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <memory>
#include "IOConfig_XML.h"
#include "AccessMask.h"
#include "IOController.h"
#include "Configuration.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// --------------------------------------------------------------------------
TEST_CASE("[IOConfig_XML]: basic initialization with global config", "[IOConfig_XML]")
{
    SECTION("Basic initialization")
    {
        REQUIRE(uniset_conf() != nullptr);

        IOConfig_XML config(uniset_conf()->getConfFileName(), uniset_conf());
        auto ioList = config.read();

        CHECK(ioList.size() >= 5); // Как минимум 5 датчиков из tests_with_conf.xml
    }

    SECTION("Sensor parameters from config file")
    {
        IOConfig_XML config(uniset_conf()->getConfFileName(), uniset_conf());
        auto ioList = config.read();

        // Проверяем датчики из tests_with_conf.xml
        auto it = ioList.find(1); // Input1_S
        REQUIRE(it != ioList.end());

        auto& sensor = it->second;
        REQUIRE(sensor->si.id == 1);
        REQUIRE(sensor->value == 1); // default="1"
        REQUIRE(sensor->type == UniversalIO::DI);
        REQUIRE(sensor->priority == uniset::Message::Medium);
        REQUIRE(sensor->acl == nullptr);

        auto it2 = ioList.find(142); // SensorRW
        REQUIRE(it2 != ioList.end());

        auto& sensor2 = it2->second;
        REQUIRE(sensor2->si.id == 142);
        REQUIRE( sensor2->acl != nullptr);
        REQUIRE(sensor2->acl->defaultPermissions == AccessRW);
        auto pit2 = sensor2->acl->permissions.find(500); // Process1
        REQUIRE( pit2 != sensor2->acl->permissions.end() );
        auto& p2 = pit2->second;
        REQUIRE(p2 == AccessRO);
    }
}
// --------------------------------------------------------------------------
