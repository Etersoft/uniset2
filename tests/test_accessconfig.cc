#include <catch.hpp>
#include <memory>
#include <string>
#include "AccessConfig.h"
#include "AccessMask.h"
#include "Configuration.h"
#include "UniXML.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// --------------------------------------------------------------------------
TEST_CASE("AccessConfig: read ACLConfig from global tests_with_conf.xml", "[AccessConfig]")
{
    REQUIRE(uniset_conf() != nullptr);
    auto conf = uniset_conf();

    SECTION("ACLConfig1 contains acl1 and acl2 with expected defaults and entries")
    {
        auto map1 = AccessConfig::read(conf, conf->getConfXML(), "ACLConfig", "ACLConfig");
        REQUIRE(map1.count("acl1") == 1);
        REQUIRE(map1.count("acl2") == 1);

        // acl1
        {
            auto acl1 = map1["acl1"];
            REQUIRE(acl1 != nullptr);
            CHECK(acl1->defaultPermissions == AccessRW);

            ObjectId p1 = conf->getObjectID("Process1");
            ObjectId p2 = conf->getObjectID("Process2");
            REQUIRE(p1 != DefaultObjectId);
            REQUIRE(p2 != DefaultObjectId);

            REQUIRE(acl1->permissions.count(p1) == 1);
            REQUIRE(acl1->permissions.count(p2) == 1);
            CHECK(acl1->permissions.at(p1) == AccessRO);
            CHECK(acl1->permissions.at(p2) == AccessMask(AccessMask::WRITE));
        }

        // acl2
        {
            auto acl2 = map1["acl2"];
            REQUIRE(acl2 != nullptr);
            CHECK(acl2->defaultPermissions == AccessRW);

            ObjectId p1 = conf->getObjectID("Process1");
            ObjectId p3 = conf->getObjectID("Process3");
            REQUIRE(p1 != DefaultObjectId);
            REQUIRE(p3 != DefaultObjectId);

            REQUIRE(acl2->permissions.count(p1) == 1);
            REQUIRE(acl2->permissions.count(p3) == 1);
            CHECK(acl2->permissions.at(p1) == AccessRO);
            CHECK(acl2->permissions.at(p3) == AccessMask(AccessMask::WRITE));
        }
    }

    SECTION("ACLConfig2 contains acl_no_default with Unknown default")
    {
        auto map2 = AccessConfig::read(conf, conf->getConfXML(), "ACLConfig2", "ACLConfig");
        REQUIRE(map2.count("acl_no_default") == 1);
        auto acl = map2["acl_no_default"];
        REQUIRE(acl != nullptr);
        CHECK(acl->defaultPermissions == AccessUnknown);

        ObjectId p1 = conf->getObjectID("Process1");
        REQUIRE(p1 != DefaultObjectId);
        REQUIRE(acl->permissions.count(p1) == 1);
        CHECK(acl->permissions.at(p1) == AccessRO);
    }
}
// --------------------------------------------------------------------------
