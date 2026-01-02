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
#include <sstream>
#include "UniSetUno.h"
#include "UniXML.h"
// -------------------------------------------------------------------------
using namespace uniset;
// -------------------------------------------------------------------------
// Test configuration XML
static const char* TEST_CONFIG = R"(<?xml version="1.0" encoding="utf-8"?>
<UniSetTest>
  <UniSetUno name="TestApp">
    <extensions>
      <extension type="SharedMemory" name="SharedMemory"/>
      <extension type="UNetExchange" name="UNetExchange1" prefix="unet"/>
      <extension type="MBTCPMaster" name="MBTCPMaster1" prefix="mbtcp"/>
    </extensions>
  </UniSetUno>

  <UniSetUno name="MinimalApp">
    <extensions>
      <extension type="SharedMemory" name="SM"/>
    </extensions>
  </UniSetUno>

  <UniSetUno name="EmptyApp">
    <extensions>
    </extensions>
  </UniSetUno>
</UniSetTest>
)";

// Test configuration with args
static const char* TEST_CONFIG_WITH_ARGS = R"(<?xml version="1.0" encoding="utf-8"?>
<UniSetTest>
  <UniSetUno name="AppWithArgs">
    <extensions>
      <extension type="SharedMemory" name="SharedMemory"/>
      <extension type="MBTCPMaster" name="MBMaster1" prefix="mbtcp"
                 args="--mbtcp-polltime 200 --mbtcp-reopen-timeout 5000"/>
      <extension type="UNetExchange" name="UNet1" prefix="unet"
                 args="--unet-recv-timeout 3000"/>
    </extensions>
  </UniSetUno>

  <UniSetUno name="AppWithQuotedArgs">
    <extensions>
      <extension type="SharedMemory" name="SM"/>
      <extension type="MBTCPMaster" name="MB1" prefix="mbtcp"
                 args="--mbtcp-name 'My Master' --mbtcp-polltime 100"/>
    </extensions>
  </UniSetUno>

  <UniSetUno name="AppNoArgs">
    <extensions>
      <extension type="SharedMemory" name="SM"/>
      <extension type="MBTCPMaster" name="MB1" prefix="mbtcp"/>
    </extensions>
  </UniSetUno>

  <settings>
    <UniSetUno name="AppInSettings">
      <extensions>
        <extension type="SharedMemory" name="SM"/>
        <extension type="MBTCPMaster" name="MB1" prefix="mbtcp"
                   args="--mbtcp-polltime 500"/>
      </extensions>
    </UniSetUno>
  </settings>
</UniSetTest>
)";
// -------------------------------------------------------------------------
class TempConfigFile
{
    public:
        TempConfigFile(const std::string& content)
            : path_("/tmp/unisetapp_test_config.xml")
        {
            std::ofstream f(path_);
            f << content;
            f.close();
        }

        ~TempConfigFile()
        {
            std::remove(path_.c_str());
        }

        const std::string& path() const
        {
            return path_;
        }

    private:
        std::string path_;
};
// -------------------------------------------------------------------------
// Extension type constants tests
// -------------------------------------------------------------------------
TEST_CASE("UniSetUno: extension type constants", "[app]")
{
    REQUIRE(std::string(UniSetUno::EXT_SHARED_MEMORY) == "SharedMemory");
    REQUIRE(std::string(UniSetUno::EXT_UNET_EXCHANGE) == "UNetExchange");
    REQUIRE(std::string(UniSetUno::EXT_MBTCP_MASTER) == "MBTCPMaster");
    REQUIRE(std::string(UniSetUno::EXT_MBTCP_MULTIMASTER) == "MBTCPMultiMaster");
    REQUIRE(std::string(UniSetUno::EXT_MB_SLAVE) == "MBSlave");
    REQUIRE(std::string(UniSetUno::EXT_RTU_EXCHANGE) == "RTUExchange");
    REQUIRE(std::string(UniSetUno::EXT_OPCUA_SERVER) == "OPCUAServer");
    REQUIRE(std::string(UniSetUno::EXT_OPCUA_EXCHANGE) == "OPCUAExchange");
    REQUIRE(std::string(UniSetUno::EXT_IO_CONTROL) == "IOControl");
    REQUIRE(std::string(UniSetUno::EXT_LOGIC_PROCESSOR) == "LogicProcessor");
    REQUIRE(std::string(UniSetUno::EXT_MQTT_PUBLISHER) == "MQTTPublisher");
    REQUIRE(std::string(UniSetUno::EXT_WEBSOCKET_GATE) == "UWebSocketGate");
    REQUIRE(std::string(UniSetUno::EXT_BACKEND_CLICKHOUSE) == "BackendClickHouse");
}
// -------------------------------------------------------------------------
// UniSetUno initial state tests
// -------------------------------------------------------------------------
TEST_CASE("UniSetUno: initial state", "[app]")
{
    UniSetUno app;

    REQUIRE(app.isRunning() == false);
    REQUIRE(app.getSharedMemory() == nullptr);
    REQUIRE(app.getExtensionNames().empty());
    REQUIRE(app.getExtension("Unknown") == nullptr);
    REQUIRE(app.log() != nullptr);
}
// -------------------------------------------------------------------------
// Help output test
// -------------------------------------------------------------------------
TEST_CASE("UniSetUno: help_print outputs to stdout", "[app]")
{
    // Capture stdout
    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());

    const char* argv[] = {"uniset2-app"};
    UniSetUno::help_print(1, argv);

    std::cout.rdbuf(old);

    std::string output = buffer.str();

    // Check that help contains expected sections
    REQUIRE(output.find("UniSetUno") != std::string::npos);
    REQUIRE(output.find("--confile") != std::string::npos);
    REQUIRE(output.find("--uno-name") != std::string::npos);
    REQUIRE(output.find("SharedMemory") != std::string::npos);
    REQUIRE(output.find("UNetExchange") != std::string::npos);
    REQUIRE(output.find("MBTCPMaster") != std::string::npos);
    REQUIRE(output.find("extension") != std::string::npos);
}
// -------------------------------------------------------------------------
// parseArgsString tests
// -------------------------------------------------------------------------
TEST_CASE("UniSetUno: parseArgsString basic", "[args]")
{
    SECTION("empty string")
    {
        auto args = UniSetUno::parseArgsString("");
        REQUIRE(args.empty());
    }

    SECTION("single argument")
    {
        auto args = UniSetUno::parseArgsString("--test");
        REQUIRE(args.size() == 1);
        REQUIRE(args[0] == "--test");
    }

    SECTION("multiple arguments")
    {
        auto args = UniSetUno::parseArgsString("--mbtcp-polltime 200 --mbtcp-reopen-timeout 5000");
        REQUIRE(args.size() == 4);
        REQUIRE(args[0] == "--mbtcp-polltime");
        REQUIRE(args[1] == "200");
        REQUIRE(args[2] == "--mbtcp-reopen-timeout");
        REQUIRE(args[3] == "5000");
    }

    SECTION("with tabs")
    {
        auto args = UniSetUno::parseArgsString("--arg1\t100\t--arg2\t200");
        REQUIRE(args.size() == 4);
        REQUIRE(args[0] == "--arg1");
        REQUIRE(args[1] == "100");
    }

    SECTION("multiple spaces")
    {
        auto args = UniSetUno::parseArgsString("--arg1   100    --arg2   200");
        REQUIRE(args.size() == 4);
    }

    SECTION("leading and trailing spaces")
    {
        auto args = UniSetUno::parseArgsString("   --arg1 100   ");
        REQUIRE(args.size() == 2);
        REQUIRE(args[0] == "--arg1");
        REQUIRE(args[1] == "100");
    }
}
// -------------------------------------------------------------------------
TEST_CASE("UniSetUno: parseArgsString with quotes", "[args]")
{
    SECTION("double quotes")
    {
        auto args = UniSetUno::parseArgsString("--name \"My Value\" --other 123");
        REQUIRE(args.size() == 4);
        REQUIRE(args[0] == "--name");
        REQUIRE(args[1] == "My Value");
        REQUIRE(args[2] == "--other");
        REQUIRE(args[3] == "123");
    }

    SECTION("single quotes")
    {
        auto args = UniSetUno::parseArgsString("--name 'My Value' --other 123");
        REQUIRE(args.size() == 4);
        REQUIRE(args[0] == "--name");
        REQUIRE(args[1] == "My Value");
        REQUIRE(args[2] == "--other");
        REQUIRE(args[3] == "123");
    }

    SECTION("mixed quotes")
    {
        auto args = UniSetUno::parseArgsString("--name \"Don't stop\" --other 'Say \"hello\"'");
        REQUIRE(args.size() == 4);
        REQUIRE(args[0] == "--name");
        REQUIRE(args[1] == "Don't stop");
        REQUIRE(args[2] == "--other");
        REQUIRE(args[3] == "Say \"hello\"");
    }

    SECTION("empty quoted string")
    {
        auto args = UniSetUno::parseArgsString("--name \"\" --other ''");
        REQUIRE(args.size() == 2);
        REQUIRE(args[0] == "--name");
        REQUIRE(args[1] == "--other");
    }
}
// -------------------------------------------------------------------------
// collectExtensionArgs tests
// -------------------------------------------------------------------------
TEST_CASE("UniSetUno: collectExtensionArgs", "[args]")
{
    TempConfigFile configFile(TEST_CONFIG_WITH_ARGS);
    auto xml = std::make_shared<UniXML>(configFile.path());

    SECTION("collect args from AppWithArgs")
    {
        auto args = UniSetUno::collectExtensionArgs(xml, "AppWithArgs");
        // Should have args from MBTCPMaster and UNetExchange
        // --mbtcp-polltime 200 --mbtcp-reopen-timeout 5000 --unet-recv-timeout 3000
        REQUIRE(args.size() == 6);

        // Check MBTCPMaster args
        bool hasPolltime = false;
        bool hasReopenTimeout = false;
        bool hasRecvTimeout = false;

        for (size_t i = 0; i + 1 < args.size(); i++)
        {
            if (args[i] == "--mbtcp-polltime" && args[i + 1] == "200")
                hasPolltime = true;

            if (args[i] == "--mbtcp-reopen-timeout" && args[i + 1] == "5000")
                hasReopenTimeout = true;

            if (args[i] == "--unet-recv-timeout" && args[i + 1] == "3000")
                hasRecvTimeout = true;
        }

        REQUIRE(hasPolltime);
        REQUIRE(hasReopenTimeout);
        REQUIRE(hasRecvTimeout);
    }

    SECTION("collect args with quoted values")
    {
        auto args = UniSetUno::collectExtensionArgs(xml, "AppWithQuotedArgs");
        // --mbtcp-name 'My Master' --mbtcp-polltime 100
        REQUIRE(args.size() == 4);

        bool hasName = false;

        for (size_t i = 0; i + 1 < args.size(); i++)
        {
            if (args[i] == "--mbtcp-name" && args[i + 1] == "My Master")
                hasName = true;
        }

        REQUIRE(hasName);
    }

    SECTION("no args defined")
    {
        auto args = UniSetUno::collectExtensionArgs(xml, "AppNoArgs");
        REQUIRE(args.empty());
    }

    SECTION("app in settings section")
    {
        auto args = UniSetUno::collectExtensionArgs(xml, "AppInSettings");
        // --mbtcp-polltime 500
        REQUIRE(args.size() == 2);
        REQUIRE(args[0] == "--mbtcp-polltime");
        REQUIRE(args[1] == "500");
    }

    SECTION("non-existent app")
    {
        auto args = UniSetUno::collectExtensionArgs(xml, "NonExistent");
        REQUIRE(args.empty());
    }

    SECTION("null xml")
    {
        auto args = UniSetUno::collectExtensionArgs(nullptr, "AppWithArgs");
        REQUIRE(args.empty());
    }
}
// -------------------------------------------------------------------------
