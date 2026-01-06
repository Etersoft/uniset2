/*
 * Copyright (c) 2026 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 */
// -------------------------------------------------------------------------
#include <algorithm>
#include "ProcessTemplate.h"
// -------------------------------------------------------------------------
namespace uniset
{
    // -------------------------------------------------------------------------
    ProcessTemplateRegistry::ProcessTemplateRegistry()
    {
        registerBuiltinTemplates();
    }
    // -------------------------------------------------------------------------
    void ProcessTemplateRegistry::registerBuiltinTemplates()
    {
        // NOTE: Command-line arguments need to be verified against actual extension code

        // SharedMemory - group 1 (depends on naming)
        registerTemplate(
        {
            "SharedMemory",                     // type
            "uniset2-smemory",                  // command
            "--smemory-id ${name}",             // argsPattern
            "corba:${name}",                    // readyCheck
            10000,                              // readyTimeout_msec
            1000,                               // checkTimeout_msec
            1000,                               // checkPause_msec
            1,                                  // group (sharedmemory)
            true,                               // critical
            false,                              // needsSharedMemory (it IS SharedMemory)
            {"SharedMemory", "SM_", "SMemory"}  // prefixes
        });

        // UNetExchange - group 2 (depends on sharedmemory)
        registerTemplate(
        {
            "UNetExchange",
            "uniset2-unetexchange",
            "--unet-name ${name}",
            "corba:${name}",
            10000, 1000, 1000,
            2,
            true,   // critical (default)
            true,
            {"UNet", "UNetExchange"}
        });

        // MBTCPMaster - Modbus TCP Master
        registerTemplate(
        {
            "MBTCPMaster",
            "uniset2-mbtcpmaster",
            "--mbtcp-name ${name}",
            "corba:${name}",
            10000, 1000, 1000,
            2,
            true,   // critical (default)
            true,
            {"MBTCPMaster", "MBTCP", "ModbusTCP"}
        });

        // MBTCPMultiMaster - Modbus TCP Multi-Master (uses same --mbtcp-name as MBTCPMaster)
        registerTemplate(
        {
            "MBTCPMultiMaster",
            "uniset2-mbtcpmultimaster",
            "--mbtcp-name ${name}",
            "corba:${name}",
            10000, 1000, 1000,
            2,
            true,   // critical (default)
            true,
            {"MBMultiMaster", "MBTCPMulti", "MBTCPMultiMaster"}
        });

        // MBSlave - Modbus Slave
        registerTemplate(
        {
            "MBSlave",
            "uniset2-mbslave",
            "--mbs-name ${name}",
            "corba:${name}",
            10000, 1000, 1000,
            2,
            true,   // critical (default)
            true,
            {"MBSlave", "ModbusSlave"}
        });

        // RTUExchange - Modbus RTU Exchange (uses --rs-name, prefix "rs")
        registerTemplate(
        {
            "RTUExchange",
            "uniset2-rtuexchange",
            "--rs-name ${name}",
            "corba:${name}",
            10000, 1000, 1000,
            2,
            true,   // critical (default)
            true,
            {"RTU", "RTUExchange"}
        });

        // OPCUAServer (note: "OPCUA" prefix removed to avoid matching OPCUAExchange)
        registerTemplate(
        {
            "OPCUAServer",
            "uniset2-opcua-server",
            "--opcua-name ${name}",
            "corba:${name}",
            10000, 1000, 1000,
            2,
            true,   // critical (default)
            true,
            {"OPCUAServer", "OPCUASrv"}
        });

        // OPCUAExchange (uses same --opcua-name as OPCUAServer)
        registerTemplate(
        {
            "OPCUAExchange",
            "uniset2-opcua-exchange",
            "--opcua-name ${name}",
            "corba:${name}",
            10000, 1000, 1000,
            2,
            true,   // critical (default)
            true,
            {"OPCUAExchange", "OPCUAClient"}
        });

        // MQTTPublisher
        registerTemplate(
        {
            "MQTTPublisher",
            "uniset2-mqttpublisher",
            "--mqtt-name ${name}",
            "corba:${name}",
            10000, 1000, 1000,
            2,
            true,   // critical (default)
            true,
            {"MQTT", "MQTTPublisher"}
        });

        // LogDB - no CORBA check
        registerTemplate(
        {
            "LogDB",
            "uniset2-logdb",
            "--logdb-name ${name}",
            "",                     // no ready check
            10000, 1000, 1000,
            2,
            true,   // critical (default)
            true,
            {"LogDB"}
        });

        // IOControl
        registerTemplate(
        {
            "IOControl",
            "uniset2-iocontrol",
            "--io-name ${name}",
            "corba:${name}",
            10000, 1000, 1000,
            2,
            true,   // critical (default)
            true,
            {"IOControl", "IO_"}
        });

        // BackendClickHouse
        registerTemplate(
        {
            "BackendClickHouse",
            "uniset2-backend-clickhouse",
            "--clickhouse-name ${name}",
            "corba:${name}",
            10000, 1000, 1000,
            2,
            true,   // critical (default)
            true,
            {"ClickHouse", "BackendClickHouse"}
        });

        // UWebSocketGate
        registerTemplate(
        {
            "UWebSocketGate",
            "uniset2-uwebsocket-gate",
            "--ws-name ${name}",
            "corba:${name}",
            10000, 1000, 1000,
            2,
            true,   // critical (default)
            true,
            {"WebSocket", "UWebSocket", "UWebSocketGate"}
        });
    }
    // -------------------------------------------------------------------------
    const ProcessTemplate* ProcessTemplateRegistry::findByType(const std::string& type) const
    {
        auto it = typeIndex_.find(type);

        if (it != typeIndex_.end())
            return &templates_[it->second];

        return nullptr;
    }
    // -------------------------------------------------------------------------
    const ProcessTemplate* ProcessTemplateRegistry::findByPrefix(const std::string& name) const
    {
        for (const auto& tmpl : templates_)
        {
            for (const auto& prefix : tmpl.prefixes)
            {
                // Exact match or prefix match
                if (name == prefix || name.find(prefix) == 0)
                    return &tmpl;
            }
        }

        return nullptr;
    }
    // -------------------------------------------------------------------------
    std::string ProcessTemplateRegistry::detectType(const std::string& name) const
    {
        const auto* tmpl = findByPrefix(name);

        if (tmpl)
            return tmpl->type;

        return "";
    }
    // -------------------------------------------------------------------------
    void ProcessTemplateRegistry::registerTemplate(const ProcessTemplate& tmpl)
    {
        size_t index = templates_.size();
        templates_.push_back(tmpl);
        typeIndex_[tmpl.type] = index;
    }
    // -------------------------------------------------------------------------
    std::string ProcessTemplateRegistry::expandPattern(const std::string& pattern,
            const std::string& name)
    {
        std::string result = pattern;
        const std::string placeholder = "${name}";

        size_t pos = 0;

        while ((pos = result.find(placeholder, pos)) != std::string::npos)
        {
            result.replace(pos, placeholder.length(), name);
            pos += name.length();
        }

        return result;
    }
    // -------------------------------------------------------------------------
    // Global singleton
    ProcessTemplateRegistry& getProcessTemplateRegistry()
    {
        static ProcessTemplateRegistry registry;
        return registry;
    }
    // -------------------------------------------------------------------------
} // end of namespace uniset
