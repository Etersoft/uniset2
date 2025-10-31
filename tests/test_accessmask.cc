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
    sensorInfo->permissions = std::make_shared<IOController::USensorInfo::PermissionsMap>();
    AccessMask defaultMask = AccessRW;

    SECTION("No permissions - use defaultPermissions")
    {
        sensorInfo->permissions.reset(); // Нет permissions
        sensorInfo->defaultPermissions = AccessRO;

        auto result = sensorInfo->checkMask(100, defaultMask);
        CHECK(result == AccessRO);
        CHECK(result.canRead());
        CHECK_FALSE(result.canWrite());
    }

    SECTION("Specific process permission")
    {
        ObjectId processId = 100;
        sensorInfo->permissions->emplace(processId, AccessRO);

        auto result = sensorInfo->checkMask(processId, defaultMask);
        CHECK(result == AccessRO);
        CHECK(result.canRead());
        CHECK_FALSE(result.canWrite());
    }

    SECTION("Different process uses defaultPermissions")
    {
        ObjectId process1 = 100;
        ObjectId process2 = 200;

        sensorInfo->permissions->emplace(process1, AccessRO);
        sensorInfo->defaultPermissions = AccessRW;

        auto result1 = sensorInfo->checkMask(process1, defaultMask);
        auto result2 = sensorInfo->checkMask(process2, defaultMask);

        CHECK(result1 == AccessRO);
        CHECK(result2 == AccessRW);
        CHECK(result2.canRead());
        CHECK(result2.canWrite());
    }

    SECTION("Unknown defaultPermissions uses defaultMask")
    {
        ObjectId processId = 300;
        sensorInfo->defaultPermissions = AccessUnknown;

        auto result = sensorInfo->checkMask(processId, defaultMask);
        CHECK(result == defaultMask);
        CHECK(result.toString() == "RW");
    }

    SECTION("Permission hierarchy priority")
    {
        ObjectId processId = 400;

        // 1. Specific permission has highest priority
        sensorInfo->permissions->emplace(processId, AccessRO);
        sensorInfo->defaultPermissions = AccessRW;
        AccessMask testDefaultMask = AccessNone;

        auto result = sensorInfo->checkMask(processId, testDefaultMask);
        CHECK(result == AccessRO);

        // 2. No specific permission -> use defaultPermissions
        sensorInfo->permissions->clear();
        result = sensorInfo->checkMask(processId, testDefaultMask);
        CHECK(result == AccessRW);

        // 3. No defaultPermissions -> use defaultMask
        sensorInfo->defaultPermissions = AccessUnknown;
        result = sensorInfo->checkMask(processId, testDefaultMask);
        CHECK(result == testDefaultMask);
        CHECK_FALSE(result.empty());
    }
}
// --------------------------------------------------------------------------
TEST_CASE("[USensorInfo]: checkMask permission combinations", "[AccessMask][USensorInfo]")
{
    auto sensorInfo = std::make_shared<IOController::USensorInfo>();
    sensorInfo->permissions = std::make_shared<IOController::USensorInfo::PermissionsMap>();
    AccessMask defaultMask = AccessRW;

    SECTION("READ permission")
    {
        ObjectId processId = 500;
        sensorInfo->permissions->emplace(processId, AccessRO);

        auto result = sensorInfo->checkMask(processId, defaultMask);
        CHECK(result.canRead());
        CHECK_FALSE(result.canWrite());
        CHECK(result.toString() == "RO");
    }

    SECTION("WRITE permission")
    {
        ObjectId processId = 600;
        sensorInfo->permissions->emplace(processId, AccessMask::WRITE);

        auto result = sensorInfo->checkMask(processId, defaultMask);
        CHECK_FALSE(result.canRead());
        CHECK(result.canWrite());
        CHECK(result.raw() == static_cast<uint8_t>(AccessMask::WRITE));
    }

    SECTION("READ | WRITE permission")
    {
        ObjectId processId = 700;
        sensorInfo->permissions->emplace(processId, AccessRW);

        auto result = sensorInfo->checkMask(processId, defaultMask);
        CHECK(result.canRead());
        CHECK(result.canWrite());
        CHECK(result.toString() == "RW");
    }

    SECTION("Unknown permission (use default)")
    {
        ObjectId processId = 800;
        sensorInfo->permissions->emplace(processId, AccessUnknown);

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
    sensorInfo->permissions = std::make_shared<IOController::USensorInfo::PermissionsMap>();

    SECTION("RO defaultMask")
    {
        ObjectId processId = 900;
        AccessMask roMask = AccessRO;

        auto result = sensorInfo->checkMask(processId, roMask);
        CHECK(result.canRead());
        CHECK_FALSE(result.canWrite());
        CHECK(result.toString() == "RO");
    }

    SECTION("None defaultMask")
    {
        ObjectId processId = 1000;
        AccessMask noneMask = AccessNone;

        auto result = sensorInfo->checkMask(processId, noneMask);
        CHECK_FALSE(result.canRead());
        CHECK_FALSE(result.canWrite());
        CHECK(result.toString() == "None");
    }

    SECTION("Unknown defaultMask")
    {
        ObjectId processId = 1000;
        AccessMask unkMask = AccessUnknown;

        auto result = sensorInfo->checkMask(processId, unkMask);
        CHECK_FALSE(result.canRead());
        CHECK_FALSE(result.canWrite());
        CHECK(result.empty());
        CHECK(result.toString() == "Unknown");
    }

    SECTION("WRITE defaultMask")
    {
        ObjectId processId = 1100;
        AccessMask writeMask = AccessMask::WRITE;

        auto result = sensorInfo->checkMask(processId, writeMask);
        CHECK_FALSE(result.canRead());
        CHECK(result.canWrite());
        CHECK(result.raw() == static_cast<uint8_t>(AccessMask::WRITE));
    }
}
// --------------------------------------------------------------------------
TEST_CASE("[USensorInfo]: checkMask edge cases", "[AccessMask][USensorInfo]")
{
    auto sensorInfo = std::make_shared<IOController::USensorInfo>();
    AccessMask defaultMask = AccessRW;

    SECTION("No permissions map created")
    {
        // permissions не создан (nullptr)
        sensorInfo->defaultPermissions = AccessRO;

        auto result = sensorInfo->checkMask(100, defaultMask);
        CHECK(result == AccessRO);
        CHECK(result.canRead());
    }

    SECTION("Multiple processes with different permissions")
    {
        sensorInfo->permissions = std::make_shared<IOController::USensorInfo::PermissionsMap>();

        ObjectId process1 = 100;
        ObjectId process2 = 200;
        ObjectId process3 = 300;

        sensorInfo->permissions->emplace(process1, AccessRO);
        sensorInfo->permissions->emplace(process2, AccessRW);
        sensorInfo->permissions->emplace(process3, AccessUnknown);
        sensorInfo->defaultPermissions = AccessMask::WRITE;

        CHECK(sensorInfo->checkMask(process1, defaultMask) == AccessRO);
        CHECK(sensorInfo->checkMask(process2, defaultMask) == AccessRW);
        CHECK(sensorInfo->checkMask(process3, defaultMask) == sensorInfo->defaultPermissions);

        // Процесс без специфичных прав использует defaultPermissions
        CHECK(sensorInfo->checkMask(400, defaultMask) == AccessMask::WRITE);
    }

    SECTION("Default permissions override defaultMask")
    {
        sensorInfo->permissions = std::make_shared<IOController::USensorInfo::PermissionsMap>();
        sensorInfo->defaultPermissions = AccessRO;
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