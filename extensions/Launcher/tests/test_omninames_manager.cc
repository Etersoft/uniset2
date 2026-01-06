// Tests for OmniNamesManager
#include <catch.hpp>
#include <unistd.h>
#include <cstdlib>
#include "OmniNamesManager.h"

using namespace uniset;

TEST_CASE("OmniNamesManager: calcPortFromUID", "[omninames]")
{
    // Formula: UID + 52809
    int port = OmniNamesManager::calcPortFromUID();
    int expectedPort = static_cast<int>(getuid()) + 52809;
    REQUIRE(port == expectedPort);
}

TEST_CASE("OmniNamesManager: calcPortFromUID for root", "[omninames]")
{
    // For root (UID=0): port = 52809
    // We can't change UID in test, but we can verify formula
    int basePort = 52809;
    REQUIRE(0 + basePort == 52809);
    REQUIRE(1000 + basePort == 53809);
    REQUIRE(1001 + basePort == 53810);
}

TEST_CASE("OmniNamesManager: getDefaultLogDir", "[omninames]")
{
    std::string logDir = OmniNamesManager::getDefaultLogDir();

    // Should not be empty
    REQUIRE(!logDir.empty());

    // Should contain "omniORB"
    REQUIRE(logDir.find("omniORB") != std::string::npos);
}

TEST_CASE("OmniNamesManager: getDefaultLogDir with TMPDIR", "[omninames]")
{
    // Save original
    const char* origTmpdir = std::getenv("TMPDIR");

    // Set TMPDIR
    setenv("TMPDIR", "/custom/tmp", 1);

    std::string logDir = OmniNamesManager::getDefaultLogDir();
    REQUIRE(logDir == "/custom/tmp/omniORB");

    // Restore
    if (origTmpdir)
        setenv("TMPDIR", origTmpdir, 1);
    else
        unsetenv("TMPDIR");
}

TEST_CASE("OmniNamesManager: getDefaultLogDir with HOME fallback", "[omninames]")
{
    // Save originals
    const char* origTmpdir = std::getenv("TMPDIR");
    const char* origHome = std::getenv("HOME");

    // Clear TMPDIR, set HOME
    unsetenv("TMPDIR");
    setenv("HOME", "/home/testuser", 1);

    std::string logDir = OmniNamesManager::getDefaultLogDir();
    REQUIRE(logDir == "/home/testuser/tmp/omniORB");

    // Restore
    if (origTmpdir)
        setenv("TMPDIR", origTmpdir, 1);

    if (origHome)
        setenv("HOME", origHome, 1);
}

TEST_CASE("OmniNamesManager: initial state", "[omninames]")
{
    OmniNamesManager mgr;

    REQUIRE(mgr.getPort() == 0);
    REQUIRE(!mgr.wasStartedByUs());
    REQUIRE(!mgr.isRunning());
}

TEST_CASE("OmniNamesManager: setPort/getPort", "[omninames]")
{
    OmniNamesManager mgr;

    mgr.setPort(12345);
    REQUIRE(mgr.getPort() == 12345);

    mgr.setPort(52809);
    REQUIRE(mgr.getPort() == 52809);
}

TEST_CASE("OmniNamesManager: setLogDir/getLogDir", "[omninames]")
{
    OmniNamesManager mgr;

    // Default
    std::string defaultDir = mgr.getLogDir();
    REQUIRE(!defaultDir.empty());

    // Custom
    mgr.setLogDir("/custom/omni/logs");
    REQUIRE(mgr.getLogDir() == "/custom/omni/logs");
}

TEST_CASE("OmniNamesManager: start without port fails", "[omninames]")
{
    OmniNamesManager mgr;
    // Port is 0 by default

    REQUIRE(!mgr.start(false));
    REQUIRE(!mgr.wasStartedByUs());
}

TEST_CASE("OmniNamesManager: stop without start is safe", "[omninames]")
{
    OmniNamesManager mgr;
    mgr.setPort(52809);

    // Should not crash
    mgr.stop(100);
    REQUIRE(!mgr.wasStartedByUs());
}

TEST_CASE("OmniNamesManager: log stream", "[omninames]")
{
    OmniNamesManager mgr;

    auto log = mgr.log();
    REQUIRE(log != nullptr);
}
