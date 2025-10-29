#include <catch.hpp>
#include "AccessMask.h"
#include "IOController.h"
// --------------------------------------------------------------------------
using namespace uniset;
// --------------------------------------------------------------------------
TEST_CASE("[AccessMask]: default/constructors", "[AccessMask]")
{
    SECTION("default = Unknown")
    {
        AccessMask a;
        CHECK(a.empty());
        CHECK(a.raw() == 0);
        CHECK_FALSE(a.canRead());
        CHECK_FALSE(a.canWrite());
        CHECK(a.toString() == "Unknown");
    }

    SECTION("RO constructor")
    {
        AccessMask a(AccessMask::READ);
        CHECK_FALSE(a.empty());
        CHECK(a.canRead());
        CHECK_FALSE(a.canWrite());
        CHECK(a.toString() == "RO");

        // has self; does not include WRITE
        CHECK(a.has(AccessMask(AccessMask::READ)));
        CHECK_FALSE(a.has(AccessMask(AccessMask::WRITE)));
    }

    SECTION("RW via initializer_list")
    {
        AccessMask a({AccessMask::READ, AccessMask::WRITE});
        CHECK_FALSE(a.empty());
        CHECK(a.canRead());
        CHECK(a.canWrite());
        CHECK(a.toString() == "RW");

        CHECK(a.has(AccessMask(AccessMask::READ)));
        CHECK(a.has(AccessMask(AccessMask::WRITE)));
        CHECK(a.has(AccessMask({AccessMask::READ, AccessMask::WRITE})));
    }

    SECTION("Named constants")
    {
        CHECK_FALSE(AccessUnknown.canRead());
        CHECK_FALSE(AccessUnknown.canWrite());
        CHECK_FALSE(AccessNone.canRead());
        CHECK_FALSE(AccessNone.canWrite());
        CHECK(AccessRO.canRead());
        CHECK_FALSE(AccessRO.canWrite());
        CHECK(AccessRW.canRead());
        CHECK(AccessRW.canWrite());
    }
}
// --------------------------------------------------------------------------
TEST_CASE("[AccessMask]: mutation (add/remove/clear)", "[AccessMask]")
{
    AccessMask a; // None
    CHECK(a.empty());

    SECTION("add READ -> RO")
    {
        a.add(AccessMask(AccessMask::READ));
        CHECK(a.canRead());
        CHECK_FALSE(a.canWrite());
        CHECK(a.toString() == "RO");
    }

    SECTION("add READ then WRITE -> RW")
    {
        a.add(AccessMask(AccessMask::READ));
        a.add(AccessMask(AccessMask::WRITE));
        CHECK(a.canRead());
        CHECK(a.canWrite());
        CHECK(a.toString() == "RW");

        // remove READ -> only WRITE
        a.remove(AccessMask(AccessMask::READ));
        CHECK_FALSE(a.canRead());
        CHECK(a.canWrite());
        CHECK(a.raw() == static_cast<uint8_t>(AccessMask::WRITE));

        // clear -> Unknown
        a.clear();
        CHECK(a.empty());
        CHECK_FALSE(a.canRead());
        CHECK_FALSE(a.canWrite());
        CHECK(a.toString() == "Unknown");
    }
}
// --------------------------------------------------------------------------
TEST_CASE("[AccessMask]: bitwise ops", "[AccessMask]")
{
    const AccessMask none = AccessNone;
    const AccessMask ro   = AccessRO;                 // READ
    const AccessMask w    = AccessMask(AccessMask::WRITE);
    const AccessMask rw   = AccessRW;                 // READ|WRITE

    SECTION("OR")
    {
        CHECK((ro | w).canRead());
        CHECK((ro | w).canWrite());
        CHECK((ro | w).has(rw));
        CHECK((none | ro).toString() == "None");
        CHECK((none | w).raw() == static_cast<uint8_t>(AccessMask::NONE));
    }

    SECTION("AND")
    {
        CHECK(((ro & w).raw() == 0));  // RO & W = None
        CHECK((rw & ro).toString() == "RO");
        CHECK((rw & w).raw() == static_cast<uint8_t>(AccessMask::WRITE));
    }

    SECTION("has() semantics")
    {
        CHECK(rw.has(ro));
        CHECK_FALSE(ro.has(rw));
        CHECK_FALSE(none.has(ro));
        CHECK(none.has(none));
    }
}
// --------------------------------------------------------------------------
TEST_CASE("[AccessMask]: fromString / toString", "[AccessMask]")
{
    SECTION("Basic strings")
    {
        CHECK(AccessMask::fromString("") == AccessUnknown);
        CHECK(AccessMask::fromString("unk") == AccessUnknown);

        CHECK(AccessMask::fromString("None") == AccessNone);
        CHECK(AccessMask::fromString("none") == AccessNone);

        CHECK(AccessMask::fromString("RO") == AccessRO);
        CHECK(AccessMask::fromString("ro") == AccessRO);
        CHECK(AccessMask::fromString("read") == AccessRO);
        CHECK(AccessMask::fromString("r") == AccessRO);
        CHECK(AccessMask::fromString("rd") == AccessRO);

        CHECK(AccessMask::fromString("wr") == AccessMask::WRITE);
        CHECK(AccessMask::fromString("W") == AccessMask::WRITE);
        CHECK(AccessMask::fromString("write") == AccessMask::WRITE);

        CHECK(AccessMask::fromString("RW") == AccessRW);
        CHECK(AccessMask::fromString("rw") == AccessRW);
    }

    SECTION("Invalid or empty string returns Unknown")
    {
        CHECK(AccessMask::fromString("") == AccessUnknown);
        CHECK(AccessMask::fromString("unknown") == AccessUnknown);
        CHECK(AccessMask::fromString("42") == AccessUnknown);
    }

    SECTION("Round-trip conversion (toString -> fromString)")
    {
        const AccessMask variants[] = { AccessNone, AccessRO, AccessRW };

        for (const auto& v : variants)
        {
            auto s = v.toString();
            auto back = AccessMask::fromString(s);
            INFO("toString=" << s);
            CHECK(back == v);
        }
    }

    SECTION("Unknown string")
    {
        auto u = AccessMask::fromString("Unknown");
        CHECK(u != AccessNone);
        CHECK_FALSE(u.canRead());
        CHECK_FALSE(u.canWrite());
        const auto s = u.toString();
        CHECK( (s == "Unknown" || s.rfind("Unknown(", 0) == 0) );

        if (s == "Unknown")
        {
            CHECK(AccessMask::fromString(s) == u);
        }
    }


}
// --------------------------------------------------------------------------
TEST_CASE("[USensorInfo]: checkMask permissions hierarchy", "[AccessMask][USensorInfo]")
{
    auto sensorInfo = std::make_shared<IOController::USensorInfo>();
    sensorInfo->acl = std::make_shared<uniset::ACL>();
    auto& perms = sensorInfo->acl->permissions;
    AccessMask defaultMask = AccessRW;

    SECTION("No specific permission -> use defaultPermissions")
    {
        // нет записи в perms
        sensorInfo->acl->defaultPermissions = AccessRO;

        auto result = sensorInfo->checkMask(100, defaultMask);
        CHECK(result == AccessRO);
        CHECK(result.canRead());
        CHECK_FALSE(result.canWrite());
    }

    SECTION("Specific permission has highest priority")
    {
        ObjectId processId = 400;

        // Явное правило сильнее defaultPermissions
        perms.emplace(processId, AccessRO);
        sensorInfo->acl->defaultPermissions = AccessRW;

        auto result = sensorInfo->checkMask(processId, defaultMask);
        CHECK(result == AccessRO);
    }

    SECTION("WRITE permission (explicit)")
    {
        ObjectId processId = 600;
        perms.emplace(processId, AccessMask::WRITE);

        auto result = sensorInfo->checkMask(processId, defaultMask);
        CHECK_FALSE(result.canRead());
        CHECK(result.canWrite());
        CHECK(result.raw() == static_cast<uint8_t>(AccessMask::WRITE));
    }

    SECTION("READ | WRITE permission (explicit)")
    {
        ObjectId processId = 700;
        perms.emplace(processId, AccessRW);

        auto result = sensorInfo->checkMask(processId, defaultMask);
        CHECK(result.canRead());
        CHECK(result.canWrite());
        CHECK(result.toString() == "RW");
    }

    SECTION("NONE permission (deny)")
    {
        ObjectId processId = 800;
        perms.emplace(processId, AccessNone);

        auto result = sensorInfo->checkMask(processId, defaultMask);
        CHECK_FALSE(result.canRead());
        CHECK_FALSE(result.canWrite());
        CHECK(result == AccessNone);
    }

    SECTION("Default permissions override defaultMask (when defaultPermissions is set)")
    {
        perms.clear();
        sensorInfo->acl->defaultPermissions = AccessRO;
        AccessMask testDefaultMask = AccessMask::WRITE;

        auto result = sensorInfo->checkMask(100, testDefaultMask);
        CHECK(result == AccessRO); // defaultPermissions имеет приоритет над defaultMask
        CHECK_FALSE(result == testDefaultMask);
    }
}

// --------------------------------------------------------------------------
TEST_CASE("[USensorInfo]: checkMask permission combinations", "[AccessMask][USensorInfo]")
{
    auto sensorInfo = std::make_shared<IOController::USensorInfo>();
    sensorInfo->acl = std::make_shared<ACL>();
    AccessMask defaultMask = AccessRW;

    SECTION("READ permission")
    {
        ObjectId processId = 500;
        sensorInfo->acl->permissions.emplace(processId, AccessRO);

        auto result = sensorInfo->checkMask(processId, defaultMask);
        CHECK(result.canRead());
        CHECK_FALSE(result.canWrite());
        CHECK(result.toString() == "RO");
    }

    SECTION("WRITE permission")
    {
        ObjectId processId = 600;
        sensorInfo->acl->permissions.emplace(processId, AccessMask::WRITE);

        auto result = sensorInfo->checkMask(processId, defaultMask);
        CHECK_FALSE(result.canRead());
        CHECK(result.canWrite());
        CHECK(result.raw() == static_cast<uint8_t>(AccessMask::WRITE));
    }

    SECTION("READ | WRITE permission")
    {
        ObjectId processId = 700;
        sensorInfo->acl->permissions.emplace(processId, AccessRW);

        auto result = sensorInfo->checkMask(processId, defaultMask);
        CHECK(result.canRead());
        CHECK(result.canWrite());
        CHECK(result.toString() == "RW");
    }

    SECTION("Unknown permission (use default)")
    {
        ObjectId processId = 800;
        sensorInfo->acl->permissions.emplace(processId, AccessUnknown);

        auto result = sensorInfo->checkMask(processId, defaultMask);
        CHECK(result.canRead());
        CHECK(result.canWrite());
        CHECK(result.toString() == "RW");
    }
}
// --------------------------------------------------------------------------
TEST_CASE("[USensorInfo]: checkMask with different defaultMasks", "[AccessMask][USensorInfo]")
{
    auto sensorInfo = std::make_shared<IOController::USensorInfo>();
    sensorInfo->acl = std::make_shared<uniset::ACL>();
    auto& perms = sensorInfo->acl->permissions;

    SECTION("RO defaultMask (when defaultPermissions == Unknown)")
    {
        ObjectId processId = 900;
        sensorInfo->acl->defaultPermissions = AccessUnknown; // «дефолт не задан»
        AccessMask roMask = AccessRO;

        auto result = sensorInfo->checkMask(processId, roMask);
        CHECK(result.canRead());
        CHECK_FALSE(result.canWrite());
        CHECK(result.toString() == "RO");
    }

    SECTION("RW defaultMask (when defaultPermissions == Unknown)")
    {
        ObjectId processId = 901;
        sensorInfo->acl->defaultPermissions = AccessUnknown;
        AccessMask rwMask = AccessRW;

        auto result = sensorInfo->checkMask(processId, rwMask);
        CHECK(result.canRead());
        CHECK(result.canWrite());
        CHECK(result.toString() == "RW");
    }

    SECTION("Unknown defaultPermissions -> use provided defaultMask")
    {
        sensorInfo->acl->defaultPermissions = AccessUnknown;
        AccessMask defaultMask = AccessMask::WRITE;

        auto result = sensorInfo->checkMask(100, defaultMask);
        CHECK(result == defaultMask); // Unknown => берём defaultMask
        CHECK_FALSE(result.canRead());
        CHECK(result.canWrite());
    }

    SECTION("DefaultPermissions == None -> deny even if defaultMask allows")
    {
        // Пустые perms, но defaultPermissions=None
        perms.clear();
        sensorInfo->acl->defaultPermissions = AccessNone;
        AccessMask defaultMask = AccessRW;

        auto result = sensorInfo->checkMask(100, defaultMask);
        CHECK(result == AccessNone);
        CHECK_FALSE(result.canRead());
        CHECK_FALSE(result.canWrite());
    }

    SECTION("Multiple processes with different explicit permissions")
    {
        perms.clear();

        ObjectId process1 = 100;
        ObjectId process2 = 101;

        perms.emplace(process1, AccessRO);
        perms.emplace(process2, AccessMask::WRITE);

        auto r1 = sensorInfo->checkMask(process1, AccessRW);
        auto r2 = sensorInfo->checkMask(process2, AccessRW);

        CHECK(r1 == AccessRO);
        CHECK_FALSE(r1.canWrite());
        CHECK(r2 == AccessMask(AccessMask::WRITE));
        CHECK_FALSE(r2.canRead());
    }
}

// --------------------------------------------------------------------------
TEST_CASE("[USensorInfo]: checkMask edge cases", "[AccessMask][USensorInfo]")
{
    auto sensorInfo = std::make_shared<IOController::USensorInfo>();
    sensorInfo->acl = std::make_shared<ACL>();
    AccessMask defaultMask = AccessRW;

    SECTION("No permissions map created")
    {
        // permissions не создан (nullptr)
        sensorInfo->acl->defaultPermissions = AccessRO;

        auto result = sensorInfo->checkMask(100, defaultMask);
        CHECK(result == AccessRO);
        CHECK(result.canRead());
    }

    SECTION("Multiple processes with different permissions")
    {
        ObjectId process1 = 100;
        ObjectId process2 = 200;
        ObjectId process3 = 300;

        sensorInfo->acl->permissions.emplace(process1, AccessRO);
        sensorInfo->acl->permissions.emplace(process2, AccessRW);
        sensorInfo->acl->permissions.emplace(process3, AccessUnknown);
        sensorInfo->acl->defaultPermissions = AccessMask::WRITE;

        CHECK(sensorInfo->checkMask(process1, defaultMask) == AccessRO);
        CHECK(sensorInfo->checkMask(process2, defaultMask) == AccessRW);
        CHECK(sensorInfo->checkMask(process3, defaultMask) == sensorInfo->acl->defaultPermissions);

        // Процесс без специфичных прав использует defaultPermissions
        CHECK(sensorInfo->checkMask(400, defaultMask) == AccessMask::WRITE);
    }

    SECTION("Default permissions override defaultMask")
    {
        sensorInfo->acl->defaultPermissions = AccessRO;
        AccessMask testDefaultMask = AccessMask::WRITE;

        auto result = sensorInfo->checkMask(100, testDefaultMask);
        CHECK(result == AccessRO); // defaultPermissions имеет приоритет над defaultMask
        CHECK_FALSE(result == testDefaultMask);
    }
}
// --------------------------------------------------------------------------
// --- Unknown: базовые свойства ---
TEST_CASE("[AccessMask]: Unknown basics", "[AccessMask]")
{
    // Предполагается, что в enum есть флаг UNKNOWN
    AccessMask u = AccessMask(AccessMask::UNKNOWN);

    CHECK_FALSE(u.canRead());
    CHECK_FALSE(u.canWrite());

    // Строковые конверсии
    CHECK(AccessMask::fromString("Unknown") == u);
    CHECK(AccessMask::fromString("unknown") == u);
    CHECK(u.toString() == "Unknown");

    // NONE и UNKNOWN — разные состояния
    CHECK(u != AccessNone);
}
// --------------------------------------------------------------------------
// --- Симметрия toString/fromString с Unknown ---
TEST_CASE("[AccessMask]: fromString/toString with Unknown", "[AccessMask]")
{
    const AccessMask variants[] =
    {
        AccessNone,
        AccessRO,
        AccessRW,
        AccessMask(AccessMask::UNKNOWN)
    };

    for (const auto& v : variants)
    {
        auto s = v.toString();
        INFO("toString=" << s);
        CHECK(AccessMask::fromString(s) == v);
    }
}
// --------------------------------------------------------------------------