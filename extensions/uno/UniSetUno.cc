/*
 * Copyright (c) 2026 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 */
// -------------------------------------------------------------------------
#include <iostream>
#include <sstream>
#include <unordered_set>
#include "UniSetUno.h"
#include "Configuration.h"
#include "UniSetActivator.h"
#include "UniXML.h"

// Include available extensions
#include "SharedMemory.h"
#include "UNetExchange.h"
#include "MBTCPMaster.h"
#include "MBTCPMultiMaster.h"
#include "MBSlave.h"
#include "RTUExchange.h"

#ifdef ENABLE_OPCUA
#include "OPCUAServer.h"
#include "OPCUAExchange.h"
#endif

#ifdef ENABLE_IO
#include "IOControl.h"
#endif

#ifdef ENABLE_LOGICPROC
#include "PassiveLProcessor.h"
#endif

#ifdef ENABLE_MQTT
#include "MQTTPublisher.h"
#endif

#ifdef ENABLE_UWEBSOCKETGATE
#include "UWebSocketGate.h"
#endif

#ifdef ENABLE_CLICKHOUSE
#include "BackendClickHouse.h"
#endif

#ifdef ENABLE_PGSQL
#include "DBServer_PostgreSQL.h"
#endif

// -------------------------------------------------------------------------
namespace uniset
{
    // -------------------------------------------------------------------------
    // Helper class to build extended argv for extension initialization
    // Adds --{prefix}-name {name} and optional extra args
    class ExtArgvBuilder
    {
        public:
            ExtArgvBuilder(int argc, const char* const* argv,
                           const std::string& prefix, const std::string& name,
                           const std::string& extraArgs = "")
                : nameValue_(name)  // Store name as member to keep pointer valid
            {
                // Copy original args
                for (int i = 0; i < argc; ++i)
                    args_.push_back(argv[i]);

                // Add --{prefix}-name {name}
                if (!name.empty() && !prefix.empty())
                {
                    nameArg_ = "--" + prefix + "-name";
                    args_.push_back(nameArg_.c_str());
                    args_.push_back(nameValue_.c_str());
                }

                // Parse and add extra args if provided
                if (!extraArgs.empty())
                {
                    std::istringstream iss(extraArgs);
                    std::string token;

                    while (iss >> token)
                    {
                        extraArgsStorage_.push_back(token);
                    }

                    for (const auto& arg : extraArgsStorage_)
                    {
                        args_.push_back(arg.c_str());
                    }
                }
            }

            int argc() const
            {
                return static_cast<int>(args_.size());
            }
            const char* const* argv() const
            {
                return args_.data();
            }

        private:
            std::vector<const char*> args_;
            std::string nameArg_;
            std::string nameValue_;
            std::vector<std::string> extraArgsStorage_;
    };
    // -------------------------------------------------------------------------
    UniSetUno::UniSetUno()
        : applog(std::make_shared<DebugStream>())
    {
        applog->setLogName("UniSetUno");
        applog->level(Debug::ANY);
    }
    // -------------------------------------------------------------------------
    UniSetUno::~UniSetUno()
    {
        stop();
    }
    // -------------------------------------------------------------------------
    bool UniSetUno::init(int argc, const char* const* argv, const std::string& appName)
    {
        try
        {
            // Load configuration
            if (!loadConfig(appName))
            {
                applog->crit() << "Failed to load configuration" << std::endl;
                return false;
            }

            // Create extensions
            if (!createExtensions(argc, argv))
            {
                applog->crit() << "Failed to create extensions" << std::endl;
                return false;
            }

            return true;
        }
        catch (const std::exception& ex)
        {
            applog->crit() << "Init failed: " << ex.what() << std::endl;
            return false;
        }
    }
    // -------------------------------------------------------------------------
    bool UniSetUno::loadConfig(const std::string& appName)
    {
        auto conf = uniset_conf();

        if (!conf)
        {
            applog->crit() << "UniSet configuration not initialized" << std::endl;
            return false;
        }

        // Initialize log from command line and config (--uno-log-add-levels, etc.)
        conf->initLogStream(applog, "uno-log");

        // Get UniXML from configuration
        auto xml = conf->getConfXML();

        if (!xml)
        {
            applog->crit() << "Failed to get UniXML from configuration" << std::endl;
            return false;
        }

        xmlNode* root = xml->getFirstNode();

        if (!root)
        {
            applog->crit() << "Empty XML configuration" << std::endl;
            return false;
        }

        // Search for UniSetUno node
        xmlNode* appNode = nullptr;

        // First look in settings section
        xmlNode* settings = xml->findNode(root, "settings");

        if (settings)
        {
            if (appName.empty())
            {
                appNode = xml->findNode(settings, "UniSetUno");
            }
            else
            {
                UniXML::iterator sit(settings);

                if (sit.goChildren())
                {
                    do
                    {
                        if (sit.getName() == "UniSetUno" && sit.getProp("name") == appName)
                        {
                            appNode = sit.getCurrent();
                            break;
                        }
                    }
                    while (sit.goNext());
                }
            }
        }

        // Then try root level
        if (!appNode)
        {
            if (appName.empty())
            {
                appNode = xml->findNode(root, "UniSetUno");
            }
            else
            {
                UniXML::iterator it(root);

                if (it.goChildren())
                {
                    do
                    {
                        if (it.getName() == "UniSetUno" && it.getProp("name") == appName)
                        {
                            appNode = it.getCurrent();
                            break;
                        }
                    }
                    while (it.goNext());
                }
            }
        }

        if (!appNode)
        {
            applog->crit() << "UniSetUno section not found in configuration" << std::endl;
            return false;
        }

        UniXML::iterator appIt(appNode);

        // Load app-level settings
        logfile_ = appIt.getProp("logfile");

        if (!logfile_.empty())
        {
            applog->logFile(logfile_);
        }

        // Load extensions list
        if (!appIt.goChildren())
        {
            applog->crit() << "No children in UniSetUno section" << std::endl;
            return false;
        }

        do
        {
            if (appIt.getName() == "extensions")
            {
                UniXML::iterator extIt(appIt.getCurrent());

                if (!extIt.goChildren())
                    continue;

                do
                {
                    if (extIt.getName() == "extension")
                    {
                        ExtensionInfo info;
                        info.type = extIt.getProp("type");
                        info.name = extIt.getProp("name");
                        info.prefix = extIt.getProp("prefix");
                        info.args = extIt.getProp("args");

                        if (info.type.empty())
                        {
                            applog->warn() << "Extension without type, skipping" << std::endl;
                            continue;
                        }

                        if (info.name.empty())
                            info.name = info.type;

                        applog->info() << "Found extension: type=" << info.type
                                       << " name=" << info.name
                                       << " prefix=" << info.prefix
                                       << (info.args.empty() ? "" : " args=" + info.args)
                                       << std::endl;

                        extensions_.push_back(info);
                    }
                }
                while (extIt.goNext());
            }
        }
        while (appIt.goNext());

        if (extensions_.empty())
        {
            applog->crit() << "No extensions defined in configuration" << std::endl;
            return false;
        }

        applog->info() << "Loaded " << extensions_.size() << " extension definitions" << std::endl;

        // Validate configuration: check for duplicate names and prefixes
        if (!validateExtensions())
            return false;

        return true;
    }
    // -------------------------------------------------------------------------
    bool UniSetUno::validateExtensions()
    {
        bool valid = true;
        auto conf = uniset_conf();

        bool ignorePrefixDuplicates = (findArgParam("--uno-ignore-prefix-duplicates",
                                       conf->getArgc(), conf->getArgv()) != -1);

        // Group extensions by type
        std::unordered_map<std::string, std::vector<const ExtensionInfo*>> byType;

        for (const auto& ext : extensions_)
            byType[ext.type].push_back(&ext);

        // Check each type group
        for (const auto& [type, exts] : byType)
        {
            if (exts.size() < 2)
                continue;

            // Check for duplicate names within same type
            std::unordered_set<std::string> names;

            for (const auto* ext : exts)
            {
                if (names.count(ext->name))
                {
                    applog->crit() << "Configuration error: duplicate name '" << ext->name
                                   << "' for type '" << type << "'" << std::endl;
                    valid = false;
                }

                names.insert(ext->name);
            }

            // Check for duplicate prefixes within same type
            std::unordered_map<std::string, std::string> prefixToName;  // prefix -> first name using it

            for (const auto* ext : exts)
            {
                std::string effectivePrefix = ext->prefix;

                // Use default prefix if empty
                if (effectivePrefix.empty())
                    effectivePrefix = getDefaultPrefix(ext->type);

                auto it = prefixToName.find(effectivePrefix);

                if (it != prefixToName.end())
                {
                    if (ignorePrefixDuplicates)
                    {
                        applog->warn() << "Warning: same prefix '" << effectivePrefix
                                       << "' used by '" << it->second << "' and '" << ext->name
                                       << "' (type: " << type << "). "
                                       << "This may cause configuration conflicts!" << std::endl;
                    }
                    else
                    {
                        applog->crit() << "Configuration error: same prefix '" << effectivePrefix
                                       << "' used by '" << it->second << "' and '" << ext->name
                                       << "' (type: " << type << "). "
                                       << "Use different prefixes or --uno-ignore-prefix-duplicates" << std::endl;
                        valid = false;
                    }
                }
                else
                {
                    prefixToName[effectivePrefix] = ext->name;
                }
            }
        }

        return valid;
    }
    // -------------------------------------------------------------------------
    std::string UniSetUno::getDefaultPrefix(const std::string& type)
    {
        if (type == EXT_SHARED_MEMORY) return "smemory";

        if (type == EXT_UNET_EXCHANGE) return "unet";

        if (type == EXT_MBTCP_MASTER) return "mbtcp";

        if (type == EXT_MBTCP_MULTIMASTER) return "mbtcp";

        if (type == EXT_MB_SLAVE) return "mbs";

        if (type == EXT_RTU_EXCHANGE) return "rs";

        if (type == EXT_OPCUA_SERVER) return "opcua";

        if (type == EXT_OPCUA_EXCHANGE) return "opcua";

        if (type == EXT_IO_CONTROL) return "io";

        if (type == EXT_LOGIC_PROCESSOR) return "lproc";

        if (type == EXT_MQTT_PUBLISHER) return "mqtt";

        if (type == EXT_WEBSOCKET_GATE) return "ws";

        if (type == EXT_BACKEND_CLICKHOUSE) return "clickhouse";

        if (type == EXT_DBSERVER_POSTGRESQL) return "pgsql";

        return "";
    }
    // -------------------------------------------------------------------------
    bool UniSetUno::createExtensions(int argc, const char* const* argv)
    {
        auto conf = uniset_conf();

        // First pass: find and create SharedMemory
        for (auto& ext : extensions_)
        {
            if (ext.type == EXT_SHARED_MEMORY)
            {
                applog->info() << "Creating SharedMemory: " << ext.name << std::endl;

                try
                {
                    shm_ = SharedMemory::init_smemory(argc, argv);

                    if (!shm_)
                    {
                        applog->crit() << "Failed to create SharedMemory" << std::endl;
                        return false;
                    }

                    ext.object = shm_;
                    extByName_[ext.name] = shm_;
                    applog->info() << "SharedMemory created successfully" << std::endl;
                }
                catch (const std::exception& ex)
                {
                    applog->crit() << "SharedMemory creation failed: " << ex.what() << std::endl;
                    return false;
                }

                break;
            }
        }

        // SharedMemory is required
        if (!shm_)
        {
            applog->crit() << "SharedMemory extension not found in configuration" << std::endl;
            return false;
        }

        ObjectId shmID = shm_->getId();

        // Second pass: create other extensions
        for (auto& ext : extensions_)
        {
            if (ext.type == EXT_SHARED_MEMORY)
                continue;  // Already created

            applog->info() << "Creating extension: " << ext.type << " (" << ext.name << ")" << std::endl;

            try
            {
                ext.object = createExtension(argc, argv, ext);

                if (!ext.object)
                {
                    applog->crit() << "Failed to create extension: " << ext.type << std::endl;
                    return false;
                }

                extByName_[ext.name] = ext.object;
                applog->info() << "Extension " << ext.name << " created successfully" << std::endl;
            }
            catch (const std::exception& ex)
            {
                applog->crit() << "Extension " << ext.name << " creation failed: " << ex.what() << std::endl;
                return false;
            }
        }

        return true;
    }
    // -------------------------------------------------------------------------
    std::shared_ptr<UniSetObject> UniSetUno::createExtension(
        int argc, const char* const* argv,
        const ExtensionInfo& info)
    {
        ObjectId shmID = shm_->getId();

        // Determine actual prefix (user-specified or default for the type)
        std::string prefix = info.prefix;

        if (prefix.empty())
            prefix = getDefaultPrefix(info.type);

        // Build extended argv with --{prefix}-name and optional args
        ExtArgvBuilder extArgs(argc, argv, prefix, info.name, info.args);

        if (info.type == EXT_UNET_EXCHANGE)
        {
            return UNetExchange::init_unetexchange(extArgs.argc(), extArgs.argv(),
                                                   shmID, shm_, prefix);
        }

        if (info.type == EXT_MBTCP_MASTER)
        {
            return MBTCPMaster::init_mbmaster(extArgs.argc(), extArgs.argv(),
                                              shmID, shm_, prefix);
        }

        if (info.type == EXT_MBTCP_MULTIMASTER)
        {
            return MBTCPMultiMaster::init_mbmaster(extArgs.argc(), extArgs.argv(),
                                                   shmID, shm_, prefix);
        }

        if (info.type == EXT_MB_SLAVE)
        {
            return MBSlave::init_mbslave(extArgs.argc(), extArgs.argv(),
                                         shmID, shm_, prefix);
        }

        if (info.type == EXT_RTU_EXCHANGE)
        {
            return RTUExchange::init_rtuexchange(extArgs.argc(), extArgs.argv(),
                                                 shmID, shm_, prefix);
        }

#ifdef ENABLE_OPCUA

        if (info.type == EXT_OPCUA_SERVER)
        {
            return OPCUAServer::init_opcua_server(extArgs.argc(), extArgs.argv(),
                                                  shmID, shm_, prefix);
        }

        if (info.type == EXT_OPCUA_EXCHANGE)
        {
            return OPCUAExchange::init_opcuaexchange(extArgs.argc(), extArgs.argv(),
                    shmID, shm_, prefix);
        }

#endif

#ifdef ENABLE_IO

        if (info.type == EXT_IO_CONTROL)
        {
            return IOControl::init_iocontrol(extArgs.argc(), extArgs.argv(),
                                             shmID, shm_, prefix);
        }

#endif

#ifdef ENABLE_LOGICPROC

        if (info.type == EXT_LOGIC_PROCESSOR)
        {
            return PassiveLProcessor::init_plproc(extArgs.argc(), extArgs.argv(),
                                                  shmID, shm_, prefix);
        }

#endif

#ifdef ENABLE_MQTT

        if (info.type == EXT_MQTT_PUBLISHER)
        {
            return MQTTPublisher::init_mqttpublisher(extArgs.argc(), extArgs.argv(),
                    shmID, shm_, prefix);
        }

#endif

#ifdef ENABLE_UWEBSOCKETGATE

        if (info.type == EXT_WEBSOCKET_GATE)
        {
            return UWebSocketGate::init_wsgate(extArgs.argc(), extArgs.argv(),
                                               shmID, shm_, prefix);
        }

#endif

#ifdef ENABLE_CLICKHOUSE

        if (info.type == EXT_BACKEND_CLICKHOUSE)
        {
            return BackendClickHouse::init_clickhouse(extArgs.argc(), extArgs.argv(),
                    shmID, shm_, prefix);
        }

#endif

#ifdef ENABLE_PGSQL

        if (info.type == EXT_DBSERVER_POSTGRESQL)
        {
            return DBServer_PostgreSQL::init_dbserver(extArgs.argc(), extArgs.argv(),
                    shm_, prefix);
        }

#endif

        applog->crit() << "Unknown extension type: " << info.type << std::endl;
        return nullptr;
    }
    // -------------------------------------------------------------------------
    void UniSetUno::run()
    {
        if (extensions_.empty())
        {
            applog->crit() << "No extensions to run" << std::endl;
            return;
        }

        auto act = UniSetActivator::Instance();

        // Add all extensions to activator
        for (const auto& ext : extensions_)
        {
            if (ext.object)
            {
                applog->info() << "Adding to activator: " << ext.name << std::endl;
                act->add(ext.object);
            }
        }

        // Broadcast StartUp message
        SystemMessage sm(SystemMessage::StartUp);
        act->broadcast(sm.transport_msg());

        running_ = true;
        applog->info() << "UniSetUno running with " << extensions_.size() << " extensions" << std::endl;

        // Run ORB (blocking)
        act->run(false);

        running_ = false;
    }
    // -------------------------------------------------------------------------
    void UniSetUno::stop()
    {
        if (!running_)
            return;

        applog->info() << "Stopping UniSetUno..." << std::endl;

        auto act = UniSetActivator::Instance();
        act->shutdown();

        running_ = false;
        applog->info() << "UniSetUno stopped" << std::endl;
    }
    // -------------------------------------------------------------------------
    std::shared_ptr<SharedMemory> UniSetUno::getSharedMemory() const
    {
        return shm_;
    }
    // -------------------------------------------------------------------------
    std::shared_ptr<UniSetObject> UniSetUno::getExtension(const std::string& name) const
    {
        auto it = extByName_.find(name);

        if (it != extByName_.end())
            return it->second;

        return nullptr;
    }
    // -------------------------------------------------------------------------
    std::vector<std::string> UniSetUno::getExtensionNames() const
    {
        std::vector<std::string> names;
        names.reserve(extensions_.size());

        for (const auto& ext : extensions_)
            names.push_back(ext.name);

        return names;
    }
    // -------------------------------------------------------------------------
    bool UniSetUno::isRunning() const
    {
        return running_;
    }
    // -------------------------------------------------------------------------
    std::shared_ptr<DebugStream> UniSetUno::log()
    {
        return applog;
    }
    // -------------------------------------------------------------------------
    bool UniSetUno::help_extension(int argc, const char* const* argv, const std::string& extType)
    {
        if (extType == EXT_SHARED_MEMORY)
        {
            SharedMemory::help_print(argc, argv);
            return true;
        }

        if (extType == EXT_UNET_EXCHANGE)
        {
            UNetExchange::help_print(argc, const_cast<const char**>(argv));
            return true;
        }

        if (extType == EXT_MBTCP_MASTER)
        {
            MBTCPMaster::help_print(argc, argv);
            return true;
        }

        if (extType == EXT_MBTCP_MULTIMASTER)
        {
            MBTCPMultiMaster::help_print(argc, argv);
            return true;
        }

        if (extType == EXT_MB_SLAVE)
        {
            MBSlave::help_print(argc, argv);
            return true;
        }

        if (extType == EXT_RTU_EXCHANGE)
        {
            RTUExchange::help_print(argc, argv);
            return true;
        }

#ifdef ENABLE_OPCUA

        if (extType == EXT_OPCUA_SERVER)
        {
            OPCUAServer::help_print();
            return true;
        }

        if (extType == EXT_OPCUA_EXCHANGE)
        {
            OPCUAExchange::help_print(argc, argv);
            return true;
        }

#endif

#ifdef ENABLE_IO

        if (extType == EXT_IO_CONTROL)
        {
            IOControl::help_print(argc, argv);
            return true;
        }

#endif

#ifdef ENABLE_LOGICPROC

        if (extType == EXT_LOGIC_PROCESSOR)
        {
            PassiveLProcessor::help_print(argc, argv);
            return true;
        }

#endif

#ifdef ENABLE_MQTT

        if (extType == EXT_MQTT_PUBLISHER)
        {
            MQTTPublisher::help_print(argc, argv);
            return true;
        }

#endif

#ifdef ENABLE_UWEBSOCKETGATE

        if (extType == EXT_WEBSOCKET_GATE)
        {
            UWebSocketGate::help_print();
            return true;
        }

#endif

#ifdef ENABLE_CLICKHOUSE

        if (extType == EXT_BACKEND_CLICKHOUSE)
        {
            BackendClickHouse::help_print(argc, argv);
            return true;
        }

#endif

#ifdef ENABLE_PGSQL

        if (extType == EXT_DBSERVER_POSTGRESQL)
        {
            DBServer_PostgreSQL::help_print(argc, argv);
            return true;
        }

#endif

        return false;
    }
    // -------------------------------------------------------------------------
    void UniSetUno::help_print(int argc, const char* const* argv, const std::string& extName)
    {
        // If specific extension requested
        if (!extName.empty())
        {
            std::cout << "=== Help for extension: " << extName << " ===" << std::endl << std::endl;

            if (!help_extension(argc, argv, extName))
            {
                std::cerr << "Unknown extension type: " << extName << std::endl;
                std::cerr << "Use --help to see list of supported extensions" << std::endl;
            }

            return;
        }

        // Print main help
        std::cout << "UniSetUno - Combined application for multiple UniSet extensions\n"
                  << "\n"
                  << "Usage: uniset2-uno --confile configure.xml [options]\n"
                  << "\n"
                  << "Options:\n"
                  << "  --confile FILE          Configuration file (required)\n"
                  << "  --uno-name NAME         UniSetUno section name in config\n"
                  << "  --uno-logfile FILE      Log file for uno messages\n"
                  << "  --uno-ignore-prefix-duplicates  Allow same prefix for multiple extensions of same type\n"
                  << "  --help                  Show this help\n"
                  << "  --help EXT_TYPE         Show help for specific extension\n"
                  << "\n"
                  << "Supported extension types:\n"
                  << "  SharedMemory            Shared memory for sensor data\n"
                  << "  UNetExchange            UDP network exchange (broadcast/multicast)\n"
                  << "  MBTCPMaster             Modbus TCP Master\n"
                  << "  MBTCPMultiMaster        Modbus TCP Multi-Master\n"
                  << "  MBSlave                 Modbus Slave\n"
                  << "  RTUExchange             Modbus RTU Exchange\n"
#ifdef ENABLE_OPCUA
                  << "  OPCUAServer             OPC UA Server\n"
                  << "  OPCUAExchange           OPC UA Exchange\n"
#endif
#ifdef ENABLE_IO
                  << "  IOControl               COMEDI I/O Control\n"
#endif
#ifdef ENABLE_LOGICPROC
                  << "  LogicProcessor          Logic element processor\n"
#endif
#ifdef ENABLE_MQTT
                  << "  MQTTPublisher           MQTT Publisher\n"
#endif
#ifdef ENABLE_UWEBSOCKETGATE
                  << "  UWebSocketGate          WebSocket Gateway\n"
#endif
#ifdef ENABLE_CLICKHOUSE
                  << "  BackendClickHouse       ClickHouse backend\n"
#endif
#ifdef ENABLE_PGSQL
                  << "  DBServerPostgreSQL      PostgreSQL database server\n"
#endif
                  << "\n"
                  << "To see help for specific extension:\n"
                  << "  uniset2-uno --help SharedMemory\n"
                  << "  uniset2-uno --help MBTCPMaster\n"
                  << "  etc.\n"
                  << std::endl;

        // Print UniSetActivator options
        UniSetActivator::help_print();

        std::cout << "\n"
                  << "XML Configuration example:\n"
                  << "<UniSetUno name=\"MyApp\">\n"
                  << "  <extensions>\n"
                  << "    <extension type=\"SharedMemory\" name=\"SharedMemory\"/>\n"
                  << "    <extension type=\"UNetExchange\" name=\"UNet1\" prefix=\"unet\"/>\n"
                  << "    <extension type=\"MBTCPMaster\" name=\"MBMaster1\" prefix=\"mbtcp\"\n"
                  << "               args=\"--mbtcp-polltime 200 --mbtcp-reopen-timeout 5000\"/>\n"
                  << "  </extensions>\n"
                  << "</UniSetUno>\n"
                  << "\n"
                  << "NOTE: args must use full argument names with prefix (e.g. --mbtcp-polltime,\n"
                  << "      not --polltime). Command-line arguments override args from config.\n"
                  << std::endl;
    }
    // -------------------------------------------------------------------------
    std::vector<std::string> UniSetUno::parseArgsString(const std::string& argsStr)
    {
        std::vector<std::string> args;
        std::string current;
        bool inQuotes = false;
        char quoteChar = 0;

        for (size_t i = 0; i < argsStr.length(); i++)
        {
            char c = argsStr[i];

            if (!inQuotes && (c == '"' || c == '\''))
            {
                inQuotes = true;
                quoteChar = c;
            }
            else if (inQuotes && c == quoteChar)
            {
                inQuotes = false;
                quoteChar = 0;
            }
            else if (!inQuotes && (c == ' ' || c == '\t'))
            {
                if (!current.empty())
                {
                    args.push_back(current);
                    current.clear();
                }
            }
            else
            {
                current += c;
            }
        }

        if (!current.empty())
            args.push_back(current);

        return args;
    }
    // -------------------------------------------------------------------------
    std::vector<std::string> UniSetUno::collectExtensionArgs(
        const std::shared_ptr<UniXML>& xml,
        const std::string& appName)
    {
        std::vector<std::string> args;

        if (!xml)
            return args;

        xmlNode* root = xml->getFirstNode();

        if (!root)
            return args;

        xmlNode* unoNode = nullptr;

        // Search in settings section first
        xmlNode* settings = xml->findNode(root, "settings");

        if (settings)
        {
            if (appName.empty())
            {
                unoNode = xml->findNode(settings, "UniSetUno");
            }
            else
            {
                UniXML::iterator sit(settings);

                if (sit.goChildren())
                {
                    do
                    {
                        if (sit.getName() == "UniSetUno" && sit.getProp("name") == appName)
                        {
                            unoNode = sit.getCurrent();
                            break;
                        }
                    }
                    while (sit.goNext());
                }
            }
        }

        // Try root level
        if (!unoNode)
        {
            if (appName.empty())
            {
                unoNode = xml->findNode(root, "UniSetUno");
            }
            else
            {
                UniXML::iterator it(root);

                if (it.goChildren())
                {
                    do
                    {
                        if (it.getName() == "UniSetUno" && it.getProp("name") == appName)
                        {
                            unoNode = it.getCurrent();
                            break;
                        }
                    }
                    while (it.goNext());
                }
            }
        }

        if (!unoNode)
            return args;

        // Find extensions section and collect args
        UniXML::iterator appIt(unoNode);

        if (!appIt.goChildren())
            return args;

        do
        {
            if (appIt.getName() == "extensions")
            {
                UniXML::iterator extIt(appIt.getCurrent());

                if (!extIt.goChildren())
                    continue;

                do
                {
                    if (extIt.getName() == "extension")
                    {
                        std::string extArgs = extIt.getProp("args");

                        if (!extArgs.empty())
                        {
                            auto parsed = parseArgsString(extArgs);

                            for (const auto& arg : parsed)
                                args.push_back(arg);
                        }
                    }
                }
                while (extIt.goNext());
            }
        }
        while (appIt.goNext());

        return args;
    }
    // -------------------------------------------------------------------------
} // end of namespace uniset
// -------------------------------------------------------------------------
