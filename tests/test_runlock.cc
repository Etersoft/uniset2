#include <catch.hpp>

#include "Configuration.h"
#include "UniSetTypes.h"
#include "RunLock.h"

using namespace std;
using namespace uniset;

TEST_CASE("RunLock", "[runlock][basic]" )
{
    REQUIRE( uniset_conf() != nullptr );

    const std::string lname = uniset_conf()->getLockDir() + "testLock.lock";
    RunLock rlock(lname);

    REQUIRE_FALSE( rlock.isLocked() );
    REQUIRE( rlock.lock() );
    REQUIRE( rlock.isLocked() );
    REQUIRE( rlock.isLockOwner() );

    // double check
    REQUIRE( rlock.isLocked() );
    REQUIRE( rlock.isLockOwner() );

    REQUIRE( rlock.unlock() );
    REQUIRE_FALSE( rlock.isLocked() );
    REQUIRE_FALSE( rlock.isLockOwner() );
}

