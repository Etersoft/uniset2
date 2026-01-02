/*
 * Copyright (c) 2026 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 */
// -------------------------------------------------------------------------
#include <catch.hpp>
#include "ProcessTemplate.h"
// -------------------------------------------------------------------------
using namespace uniset;
// -------------------------------------------------------------------------
TEST_CASE("ProcessTemplate: builtin templates registered", "[template]")
{
    auto& registry = getProcessTemplateRegistry();
    const auto& templates = registry.templates();

    // Should have at least 13 builtin templates
    REQUIRE(templates.size() >= 13);

    // Check some known types exist
    REQUIRE(registry.findByType("SharedMemory") != nullptr);
    REQUIRE(registry.findByType("UNetExchange") != nullptr);
    REQUIRE(registry.findByType("MBTCPMaster") != nullptr);
    REQUIRE(registry.findByType("MBSlave") != nullptr);
    REQUIRE(registry.findByType("OPCUAServer") != nullptr);
    REQUIRE(registry.findByType("OPCUAExchange") != nullptr);
    REQUIRE(registry.findByType("MQTTPublisher") != nullptr);
    REQUIRE(registry.findByType("IOControl") != nullptr);
    REQUIRE(registry.findByType("BackendClickHouse") != nullptr);
    REQUIRE(registry.findByType("UWebSocketGate") != nullptr);
}
// -------------------------------------------------------------------------
TEST_CASE("ProcessTemplate: findByType returns correct template", "[template]")
{
    auto& registry = getProcessTemplateRegistry();

    SECTION("SharedMemory template")
    {
        const auto* tmpl = registry.findByType("SharedMemory");
        REQUIRE(tmpl != nullptr);
        REQUIRE(tmpl->type == "SharedMemory");
        REQUIRE(tmpl->command == "uniset2-smemory");
        REQUIRE(tmpl->group == 1);
        REQUIRE(tmpl->critical == true);
        REQUIRE(tmpl->needsSharedMemory == false);
    }

    SECTION("UNetExchange template")
    {
        const auto* tmpl = registry.findByType("UNetExchange");
        REQUIRE(tmpl != nullptr);
        REQUIRE(tmpl->type == "UNetExchange");
        REQUIRE(tmpl->command == "uniset2-unetexchange");
        REQUIRE(tmpl->group == 2);
        REQUIRE(tmpl->critical == false);
        REQUIRE(tmpl->needsSharedMemory == true);
    }

    SECTION("MBTCPMaster template")
    {
        const auto* tmpl = registry.findByType("MBTCPMaster");
        REQUIRE(tmpl != nullptr);
        REQUIRE(tmpl->command == "uniset2-mbtcpmaster");
        REQUIRE(tmpl->argsPattern.find("--mbtcp-name") != std::string::npos);
    }

    SECTION("Unknown type returns nullptr")
    {
        REQUIRE(registry.findByType("UnknownType") == nullptr);
        REQUIRE(registry.findByType("") == nullptr);
    }
}
// -------------------------------------------------------------------------
TEST_CASE("ProcessTemplate: findByPrefix detects type from name", "[template]")
{
    auto& registry = getProcessTemplateRegistry();

    SECTION("Exact match")
    {
        const auto* tmpl = registry.findByPrefix("SharedMemory");
        REQUIRE(tmpl != nullptr);
        REQUIRE(tmpl->type == "SharedMemory");
    }

    SECTION("Prefix match with suffix")
    {
        const auto* tmpl = registry.findByPrefix("SharedMemory1");
        REQUIRE(tmpl != nullptr);
        REQUIRE(tmpl->type == "SharedMemory");

        tmpl = registry.findByPrefix("UNetExchange1");
        REQUIRE(tmpl != nullptr);
        REQUIRE(tmpl->type == "UNetExchange");

        tmpl = registry.findByPrefix("MBTCPMaster1");
        REQUIRE(tmpl != nullptr);
        REQUIRE(tmpl->type == "MBTCPMaster");
    }

    SECTION("Alternative prefixes")
    {
        // SM_ prefix for SharedMemory
        const auto* tmpl = registry.findByPrefix("SM_Main");
        REQUIRE(tmpl != nullptr);
        REQUIRE(tmpl->type == "SharedMemory");

        // MBTCP prefix for MBTCPMaster
        tmpl = registry.findByPrefix("MBTCP1");
        REQUIRE(tmpl != nullptr);
        REQUIRE(tmpl->type == "MBTCPMaster");

        // UNet prefix for UNetExchange
        tmpl = registry.findByPrefix("UNet1");
        REQUIRE(tmpl != nullptr);
        REQUIRE(tmpl->type == "UNetExchange");
    }

    SECTION("No match returns nullptr")
    {
        REQUIRE(registry.findByPrefix("CustomProcess") == nullptr);
        REQUIRE(registry.findByPrefix("MyApp") == nullptr);
        REQUIRE(registry.findByPrefix("") == nullptr);
    }
}
// -------------------------------------------------------------------------
TEST_CASE("ProcessTemplate: detectType convenience method", "[template]")
{
    auto& registry = getProcessTemplateRegistry();

    REQUIRE(registry.detectType("SharedMemory") == "SharedMemory");
    REQUIRE(registry.detectType("SharedMemory1") == "SharedMemory");
    REQUIRE(registry.detectType("SM_Main") == "SharedMemory");

    REQUIRE(registry.detectType("UNetExchange1") == "UNetExchange");
    REQUIRE(registry.detectType("UNet1") == "UNetExchange");

    REQUIRE(registry.detectType("MBTCPMaster1") == "MBTCPMaster");
    REQUIRE(registry.detectType("MBSlave1") == "MBSlave");

    REQUIRE(registry.detectType("OPCUAServer1") == "OPCUAServer");
    REQUIRE(registry.detectType("OPCUAExchange1") == "OPCUAExchange");

    // Unknown names return empty string
    REQUIRE(registry.detectType("CustomProcess") == "");
    REQUIRE(registry.detectType("") == "");
}
// -------------------------------------------------------------------------
TEST_CASE("ProcessTemplate: expandPattern replaces ${name}", "[template]")
{
    SECTION("Single replacement")
    {
        std::string result = ProcessTemplateRegistry::expandPattern(
                                 "--smemory-id ${name}", "SharedMemory1");
        REQUIRE(result == "--smemory-id SharedMemory1");
    }

    SECTION("Multiple replacements")
    {
        std::string result = ProcessTemplateRegistry::expandPattern(
                                 "${name} --id ${name} --log ${name}.log", "MyProcess");
        REQUIRE(result == "MyProcess --id MyProcess --log MyProcess.log");
    }

    SECTION("No placeholder")
    {
        std::string result = ProcessTemplateRegistry::expandPattern(
                                 "--static-arg", "AnyName");
        REQUIRE(result == "--static-arg");
    }

    SECTION("Empty pattern")
    {
        std::string result = ProcessTemplateRegistry::expandPattern("", "Name");
        REQUIRE(result == "");
    }

    SECTION("Empty name")
    {
        std::string result = ProcessTemplateRegistry::expandPattern(
                                 "--id ${name}", "");
        REQUIRE(result == "--id ");
    }
}
// -------------------------------------------------------------------------
TEST_CASE("ProcessTemplate: template properties", "[template]")
{
    auto& registry = getProcessTemplateRegistry();

    SECTION("SharedMemory is in group 1 and critical")
    {
        const auto* tmpl = registry.findByType("SharedMemory");
        REQUIRE(tmpl->group == 1);
        REQUIRE(tmpl->critical == true);
        REQUIRE(tmpl->needsSharedMemory == false);
    }

    SECTION("Exchanges are in group 2 and need SharedMemory")
    {
        const char* exchangeTypes[] = {
            "UNetExchange", "MBTCPMaster", "MBSlave",
            "OPCUAServer", "OPCUAExchange", "MQTTPublisher"
        };

        for (const char* type : exchangeTypes)
        {
            const auto* tmpl = registry.findByType(type);
            REQUIRE(tmpl != nullptr);
            REQUIRE(tmpl->group == 2);
            REQUIRE(tmpl->needsSharedMemory == true);
        }
    }

    SECTION("All templates have readyCheck except LogDB")
    {
        for (const auto& tmpl : registry.templates())
        {
            if (tmpl.type == "LogDB")
                REQUIRE(tmpl.readyCheck.empty());
            else
                REQUIRE_FALSE(tmpl.readyCheck.empty());
        }
    }
}
// -------------------------------------------------------------------------
TEST_CASE("ProcessTemplate: registerTemplate adds custom template", "[template]")
{
    auto& registry = getProcessTemplateRegistry();
    size_t initialCount = registry.templates().size();

    ProcessTemplate custom;
    custom.type = "CustomTestType";
    custom.command = "custom-command";
    custom.argsPattern = "--name ${name}";
    custom.readyCheck = "tcp:8080";
    custom.group = 3;
    custom.critical = true;
    custom.needsSharedMemory = false;
    custom.prefixes = {"Custom", "CT_"};

    registry.registerTemplate(custom);

    REQUIRE(registry.templates().size() == initialCount + 1);

    const auto* tmpl = registry.findByType("CustomTestType");
    REQUIRE(tmpl != nullptr);
    REQUIRE(tmpl->command == "custom-command");

    // Should be findable by prefix
    REQUIRE(registry.findByPrefix("CustomProcess") != nullptr);
    REQUIRE(registry.findByPrefix("CT_App") != nullptr);
}
// -------------------------------------------------------------------------
