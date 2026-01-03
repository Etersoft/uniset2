/*
 * Copyright (c) 2026 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 */
// -------------------------------------------------------------------------
#include <catch.hpp>
#include <fstream>
#include "ConfigLoader.h"
#include "HealthChecker.h"
// -------------------------------------------------------------------------
using namespace uniset;
// -------------------------------------------------------------------------
static const char* TEST_CONFIG = R"(<?xml version="1.0" encoding="utf-8"?>
<TestConfig>
  <Launcher name="TestLauncher"
            healthCheckInterval="3000"
            restartDelay="2000"
            restartWindow="30000"
            maxRestarts="3"
            httpPort="9090">

    <ProcessGroups>
      <group name="naming" order="0">
        <process name="omniNames"
                 command="omniNames"
                 args="-start -logdir /tmp"
                 readyCheck="tcp:2809"
                 readyTimeout="5000"
                 critical="true"/>
      </group>

      <group name="sm" order="1" depends="naming">
        <process name="SharedMemory"
                 command="uniset2-smemory"
                 args="--confile ${CONFFILE} --localNode ${NODE_NAME}"
                 readyCheck="corba:SharedMemory"
                 maxRestarts="5"/>
      </group>

      <group name="exchanges" order="2" depends="sm">
        <process name="UNetExchange"
                 command="uniset2-unetexchange"
                 args="--unet-name UNet1"
                 nodeFilter="Node1,Node2"
                 maxRestarts="-1"/>
        <process name="ModbusMaster"
                 command="uniset2-mbtcpmaster"
                 nodeFilter="Node3"/>
      </group>
    </ProcessGroups>

    <Environment>
      <var name="CONFFILE" value="/app/configure.xml"/>
      <var name="NODE_NAME" value="TestNode"/>
    </Environment>
  </Launcher>
</TestConfig>
)";
// -------------------------------------------------------------------------
class TempConfigFile
{
    public:
        TempConfigFile(const std::string& content)
            : path_("/tmp/launcher_test_config.xml")
        {
            std::ofstream f(path_);
            f << content;
            f.close();
        }

        ~TempConfigFile()
        {
            std::remove(path_.c_str());
        }

        const std::string& path() const { return path_; }

    private:
        std::string path_;
};
// -------------------------------------------------------------------------
TEST_CASE("ConfigLoader: load basic config", "[config]")
{
    TempConfigFile cfg(TEST_CONFIG);

    ConfigLoader loader;
    auto config = loader.load(cfg.path(), "TestLauncher");

    REQUIRE(config.name == "TestLauncher");
    REQUIRE(config.healthCheckInterval_msec == 3000);
    REQUIRE(config.restartDelay_msec == 2000);
    REQUIRE(config.restartWindow_msec == 30000);
    REQUIRE(config.maxRestarts == 3);
    REQUIRE(config.httpPort == 9090);
}
// -------------------------------------------------------------------------
TEST_CASE("ConfigLoader: load groups", "[config]")
{
    TempConfigFile cfg(TEST_CONFIG);

    ConfigLoader loader;
    auto config = loader.load(cfg.path(), "TestLauncher");

    REQUIRE(config.groups.size() == 3);

    // Groups should be sorted by order
    REQUIRE(config.groups[0].name == "naming");
    REQUIRE(config.groups[0].order == 0);
    REQUIRE(config.groups[0].depends.empty());

    REQUIRE(config.groups[1].name == "sm");
    REQUIRE(config.groups[1].order == 1);
    REQUIRE(config.groups[1].depends.count("naming") == 1);

    REQUIRE(config.groups[2].name == "exchanges");
    REQUIRE(config.groups[2].order == 2);
    REQUIRE(config.groups[2].depends.count("sm") == 1);
}
// -------------------------------------------------------------------------
TEST_CASE("ConfigLoader: load processes", "[config]")
{
    TempConfigFile cfg(TEST_CONFIG);

    ConfigLoader loader;
    auto config = loader.load(cfg.path(), "TestLauncher");

    REQUIRE(config.processes.size() == 4);

    // omniNames
    auto& omni = config.processes["omniNames"];
    REQUIRE(omni.name == "omniNames");
    REQUIRE(omni.command == "omniNames");
    REQUIRE(omni.args.size() == 3);
    REQUIRE(omni.args[0] == "-start");
    REQUIRE(omni.args[1] == "-logdir");
    REQUIRE(omni.args[2] == "/tmp");
    REQUIRE(omni.critical == true);
    REQUIRE(omni.readyCheck.type == ReadyCheckType::TCP);
    REQUIRE(omni.readyCheck.target == "localhost:2809");
    REQUIRE(omni.readyCheck.timeout_msec == 5000);
    REQUIRE(omni.group == "naming");

    // SharedMemory
    auto& sm = config.processes["SharedMemory"];
    REQUIRE(sm.name == "SharedMemory");
    REQUIRE(sm.command == "uniset2-smemory");
    REQUIRE(sm.readyCheck.type == ReadyCheckType::CORBA);
    REQUIRE(sm.readyCheck.target == "SharedMemory");
    REQUIRE(sm.maxRestarts == 5);
    REQUIRE(sm.critical == true);  // SharedMemory template default
    REQUIRE(sm.group == "sm");

    // UNetExchange
    auto& unet = config.processes["UNetExchange"];
    REQUIRE(unet.maxRestarts == -1);  // explicitly disabled restart
    REQUIRE(unet.nodeFilter.size() == 2);
    REQUIRE(unet.nodeFilter.count("Node1") == 1);
    REQUIRE(unet.nodeFilter.count("Node2") == 1);
    REQUIRE(unet.group == "exchanges");
}
// -------------------------------------------------------------------------
TEST_CASE("ConfigLoader: load environment", "[config]")
{
    TempConfigFile cfg(TEST_CONFIG);

    ConfigLoader loader;
    auto config = loader.load(cfg.path(), "TestLauncher");

    REQUIRE(config.environment.size() == 2);
    REQUIRE(config.environment["CONFFILE"] == "/app/configure.xml");
    REQUIRE(config.environment["NODE_NAME"] == "TestNode");
}
// -------------------------------------------------------------------------
TEST_CASE("ConfigLoader: process group assignment", "[config]")
{
    TempConfigFile cfg(TEST_CONFIG);

    ConfigLoader loader;
    auto config = loader.load(cfg.path(), "TestLauncher");

    // Check that processes are assigned to correct groups
    REQUIRE(config.groups[0].processes.size() == 1);
    REQUIRE(config.groups[0].processes[0] == "omniNames");

    REQUIRE(config.groups[1].processes.size() == 1);
    REQUIRE(config.groups[1].processes[0] == "SharedMemory");

    REQUIRE(config.groups[2].processes.size() == 2);
}
// -------------------------------------------------------------------------
TEST_CASE("ConfigLoader: missing launcher section throws", "[config]")
{
    const char* minimalConfig = R"(<?xml version="1.0"?><Config></Config>)";
TempConfigFile cfg(minimalConfig);

ConfigLoader loader;
REQUIRE_THROWS(loader.load(cfg.path(), "NonExistent"));
}
// -------------------------------------------------------------------------
TEST_CASE("HealthChecker: parseReadyCheck TCP", "[healthcheck]")
{
    auto check = HealthChecker::parseReadyCheck("tcp:2809");
    REQUIRE(check.type == ReadyCheckType::TCP);
    REQUIRE(check.target == "localhost:2809");

    auto check2 = HealthChecker::parseReadyCheck("tcp:192.168.1.1:4840");
    REQUIRE(check2.type == ReadyCheckType::TCP);
    REQUIRE(check2.target == "192.168.1.1:4840");
}
// -------------------------------------------------------------------------
TEST_CASE("HealthChecker: parseReadyCheck CORBA", "[healthcheck]")
{
    auto check = HealthChecker::parseReadyCheck("corba:SharedMemory");
    REQUIRE(check.type == ReadyCheckType::CORBA);
    REQUIRE(check.target == "SharedMemory");
}
// -------------------------------------------------------------------------
TEST_CASE("HealthChecker: parseReadyCheck HTTP", "[healthcheck]")
{
    auto check = HealthChecker::parseReadyCheck("http://localhost:8080/health");
    REQUIRE(check.type == ReadyCheckType::HTTP);
    REQUIRE(check.target == "//localhost:8080/health");
}
// -------------------------------------------------------------------------
TEST_CASE("HealthChecker: parseReadyCheck File", "[healthcheck]")
{
    auto check = HealthChecker::parseReadyCheck("file:/var/run/service.pid");
    REQUIRE(check.type == ReadyCheckType::File);
    REQUIRE(check.target == "/var/run/service.pid");
}
// -------------------------------------------------------------------------
TEST_CASE("ConfigLoader: skip attribute", "[config]")
{
    const char* configWithSkip = R"(<?xml version="1.0"?>
<Config>
  <Launcher name="Test">
    <ProcessGroups>
      <group name="test" order="0">
        <process name="proc1" command="/bin/true" skip="1"/>
        <process name="proc2" command="/bin/true" skip="true"/>
        <process name="proc3" command="/bin/true"/>
      </group>
    </ProcessGroups>
  </Launcher>
</Config>
)";
    TempConfigFile cfg(configWithSkip);

    ConfigLoader loader;
    auto config = loader.load(cfg.path(), "Test");

    REQUIRE(config.processes["proc1"].skip == true);
    REQUIRE(config.processes["proc2"].skip == true);
    REQUIRE(config.processes["proc3"].skip == false);
}
// -------------------------------------------------------------------------
TEST_CASE("HealthChecker: parseReadyCheck empty", "[healthcheck]")
{
    auto check = HealthChecker::parseReadyCheck("");
    REQUIRE(check.type == ReadyCheckType::None);
    REQUIRE(check.empty());
}
// -------------------------------------------------------------------------
TEST_CASE("HealthChecker: checkFile", "[healthcheck]")
{
    // Create temp file
    const char* tmpFile = "/tmp/launcher_test_file";
    std::ofstream f(tmpFile);
    f << "test";
    f.close();

    HealthChecker checker;

    ReadyCheck check;
    check.type = ReadyCheckType::File;
    check.target = tmpFile;
    REQUIRE(checker.checkOnce(check) == true);

    check.target = "/nonexistent/file/path";
    REQUIRE(checker.checkOnce(check) == false);

    std::remove(tmpFile);
}
// -------------------------------------------------------------------------
TEST_CASE("HealthChecker: isProcessAlive", "[healthcheck]")
{
    // Current process should be alive
    REQUIRE(HealthChecker::isProcessAlive(getpid()) == true);

    // Invalid PID should not be alive
    REQUIRE(HealthChecker::isProcessAlive(0) == false);
    REQUIRE(HealthChecker::isProcessAlive(-1) == false);

    // Very large PID unlikely to exist
    REQUIRE(HealthChecker::isProcessAlive(999999999) == false);
}
// -------------------------------------------------------------------------
TEST_CASE("ConfigLoader: commonArgs attribute", "[config]")
{
    const char* configWithCommonArgs = R"(<?xml version="1.0"?>
<Config>
  <Launcher name="Test" commonArgs="--confile ${CONFFILE} --localNode ${NODE_NAME}">
    <ProcessGroups>
      <group name="test" order="0">
        <process name="proc1" command="/bin/true" args="--extra"/>
      </group>
    </ProcessGroups>
  </Launcher>
</Config>
)";
    TempConfigFile cfg(configWithCommonArgs);

    ConfigLoader loader;
    auto config = loader.load(cfg.path(), "Test");

    REQUIRE(config.commonArgs.size() == 4);
    REQUIRE(config.commonArgs[0] == "--confile");
    REQUIRE(config.commonArgs[1] == "${CONFFILE}");
    REQUIRE(config.commonArgs[2] == "--localNode");
    REQUIRE(config.commonArgs[3] == "${NODE_NAME}");
}
// -------------------------------------------------------------------------
TEST_CASE("ConfigLoader: commonArgs empty by default", "[config]")
{
    const char* configNoCommonArgs = R"(<?xml version="1.0"?>
<Config>
  <Launcher name="Test">
    <ProcessGroups>
      <group name="test" order="0">
        <process name="proc1" command="/bin/true"/>
      </group>
    </ProcessGroups>
  </Launcher>
</Config>
)";
    TempConfigFile cfg(configNoCommonArgs);

    ConfigLoader loader;
    auto config = loader.load(cfg.path(), "Test");

    REQUIRE(config.commonArgs.empty());
}
// -------------------------------------------------------------------------
