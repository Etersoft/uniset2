/*
 * Copyright (c) 2026 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 */
// -------------------------------------------------------------------------
#include <sstream>
#include <algorithm>
#include "ConfigLoader.h"
#include "HealthChecker.h"
#include "ProcessTemplate.h"
// -------------------------------------------------------------------------
namespace uniset
{
    // -------------------------------------------------------------------------
    ConfigLoader::LauncherConfig ConfigLoader::load(const std::string& xmlFile,
            const std::string& launcherName)
    {
        auto xml = std::make_shared<UniXML>(xmlFile);
        return load(xml, launcherName);
    }
    // -------------------------------------------------------------------------
    ConfigLoader::LauncherConfig ConfigLoader::load(const std::shared_ptr<UniXML>& xml,
            const std::string& launcherName)
    {
        LauncherConfig config;

        // Find Launcher node
        xmlNode* root = xml->getFirstNode();

        if (!root)
            throw std::runtime_error("Empty XML configuration");

        xmlNode* launcherNode = nullptr;

        // Search in settings section first
        xmlNode* settings = xml->findNode(root, "settings");

        if (settings)
        {
            if (launcherName.empty())
            {
                launcherNode = xml->findNode(settings, "Launcher");
            }
            else
            {
                // Find by name attribute
                UniXML::iterator it(settings);

                if (it.goChildren())
                {
                    do
                    {
                        if (it.getName() == "Launcher" &&
                                it.getProp("name") == launcherName)
                        {
                            launcherNode = it.getCurrent();
                            break;
                        }
                    }
                    while (it.goNext());
                }
            }
        }

        // Try root level
        if (!launcherNode)
        {
            if (launcherName.empty())
                launcherNode = xml->findNode(root, "Launcher");
            else
            {
                UniXML::iterator it(root);

                if (it.goChildren())
                {
                    do
                    {
                        if (it.getName() == "Launcher" &&
                                it.getProp("name") == launcherName)
                        {
                            launcherNode = it.getCurrent();
                            break;
                        }
                    }
                    while (it.goNext());
                }
            }
        }

        if (!launcherNode)
            throw std::runtime_error("Launcher section not found in configuration");

        // Load launcher attributes
        UniXML::iterator lit(launcherNode);
        config.name = lit.getProp("name");

        if (lit.getProp("healthCheckInterval").length() > 0)
            config.healthCheckInterval_msec = lit.getIntProp("healthCheckInterval");

        if (lit.getProp("restartDelay").length() > 0)
            config.restartDelay_msec = lit.getIntProp("restartDelay");

        if (lit.getProp("restartWindow").length() > 0)
            config.restartWindow_msec = lit.getIntProp("restartWindow");

        if (lit.getProp("maxRestarts").length() > 0)
            config.maxRestarts = lit.getIntProp("maxRestarts");

        if (lit.getProp("maxRestartDelay").length() > 0)
            config.maxRestartDelay_msec = lit.getIntProp("maxRestartDelay");

        if (lit.getProp("httpPort").length() > 0)
            config.httpPort = lit.getIntProp("httpPort");

        // Default ready check for processes without explicit one
        config.defaultReadyCheck = lit.getProp("defaultReadyCheck");

        // Common arguments prepended to all processes
        std::string commonArgsStr = lit.getProp("commonArgs");

        if (!commonArgsStr.empty())
            config.commonArgs = parseArgs(commonArgsStr);

        // Load environment first (for variable expansion)
        loadEnvironment(launcherNode, config);

        // Load process groups
        loadGroups(launcherNode, config);

        return config;
    }
    // -------------------------------------------------------------------------
    void ConfigLoader::loadGroups(xmlNode* launcherNode, LauncherConfig& config)
    {
        UniXML::iterator it(launcherNode);

        if (!it.goChildren())
            return;

        do
        {
            if (it.getName() == "ProcessGroups")
            {
                UniXML::iterator git(it.getCurrent());

                if (!git.goChildren())
                    continue;

                do
                {
                    if (git.getName() != "group")
                        continue;

                    ProcessGroup group;
                    group.name = git.getProp("name");
                    group.order = git.getIntProp("order");

                    // Parse depends
                    std::string dependsStr = git.getProp("depends");

                    if (!dependsStr.empty())
                    {
                        std::istringstream iss(dependsStr);
                        std::string dep;

                        while (std::getline(iss, dep, ','))
                        {
                            // Trim whitespace
                            dep.erase(0, dep.find_first_not_of(" \t"));
                            dep.erase(dep.find_last_not_of(" \t") + 1);

                            if (!dep.empty())
                                group.depends.insert(dep);
                        }
                    }

                    // Load processes in this group
                    UniXML::iterator pit(git.getCurrent());

                    if (pit.goChildren())
                    {
                        do
                        {
                            if (pit.getName() == "process")
                            {
                                loadProcess(pit.getCurrent(), group.name, config);
                                group.processes.push_back(pit.getProp("name"));
                            }
                        }
                        while (pit.goNext());
                    }

                    config.groups.push_back(group);
                }
                while (git.goNext());
            }
        }
        while (it.goNext());

        // Sort groups by order
        std::sort(config.groups.begin(), config.groups.end(),
                  [](const ProcessGroup & a, const ProcessGroup & b)
        {
            return a.order < b.order;
        });
    }
    // -------------------------------------------------------------------------
    void ConfigLoader::loadProcess(xmlNode* processNode, const std::string& groupName,
                                   LauncherConfig& config)
    {
        UniXML::iterator it(processNode);

        ProcessInfo proc;
        proc.name = it.getProp("name");
        proc.group = groupName;

        // Check for explicit type or detect by prefix
        std::string typeStr = it.getProp("type");
        const ProcessTemplate* tmpl = nullptr;
        auto& registry = getProcessTemplateRegistry();

        if (!typeStr.empty())
        {
            // Explicit type specified
            proc.type = typeStr;
            tmpl = registry.findByType(typeStr);
        }
        else
        {
            // Try to detect by prefix
            proc.type = registry.detectType(proc.name);

            if (!proc.type.empty())
                tmpl = registry.findByType(proc.type);
        }

        // Apply template defaults if found
        if (tmpl)
        {
            // Command: use template if not explicitly specified
            std::string cmdStr = it.getProp("command");

            if (cmdStr.empty())
                proc.command = tmpl->command;
            else
                proc.command = cmdStr;

            // Args: use template pattern if not explicitly specified
            std::string argsStr = it.getProp("args");

            if (argsStr.empty() && !tmpl->argsPattern.empty())
            {
                std::string expanded = ProcessTemplateRegistry::expandPattern(tmpl->argsPattern, proc.name);
                proc.args = parseArgs(expanded);
            }
            else if (!argsStr.empty())
            {
                proc.args = parseArgs(argsStr);
            }

            // ReadyCheck: use template if not explicitly specified
            std::string readyCheckStr = it.getProp("readyCheck");

            if (readyCheckStr.empty() && !tmpl->readyCheck.empty())
            {
                // Use template readyCheck
                std::string expanded = ProcessTemplateRegistry::expandPattern(tmpl->readyCheck, proc.name);
                proc.readyCheck = HealthChecker::parseReadyCheck(expanded);
                proc.readyCheck.timeout_msec = tmpl->readyTimeout_msec;
                proc.readyCheck.checkTimeout_msec = tmpl->checkTimeout_msec;
                proc.readyCheck.pause_msec = tmpl->checkPause_msec;
            }
            else if (!readyCheckStr.empty() && readyCheckStr != "none")
            {
                // Use explicitly specified readyCheck
                proc.readyCheck = HealthChecker::parseReadyCheck(readyCheckStr);
            }
            else if (readyCheckStr.empty() && !config.defaultReadyCheck.empty())
            {
                // Use global default readyCheck
                proc.readyCheck = HealthChecker::parseReadyCheck(config.defaultReadyCheck);
            }

            // Apply timeout override if specified
            if (it.getProp("readyTimeout").length() > 0)
                proc.readyCheck.timeout_msec = it.getIntProp("readyTimeout");

            if (it.getProp("checkPause").length() > 0)
                proc.readyCheck.pause_msec = it.getIntProp("checkPause");

            if (it.getProp("checkTimeout").length() > 0)
                proc.readyCheck.checkTimeout_msec = it.getIntProp("checkTimeout");

            // Liveness check (watchdog): restart process if it stops responding
            std::string healthCheckStr = it.getProp("healthCheck");

            if (!healthCheckStr.empty() && healthCheckStr != "none")
            {
                proc.healthCheck = HealthChecker::parseReadyCheck(healthCheckStr);

                // Use readyCheck timeouts as defaults for liveness
                proc.healthCheck.checkTimeout_msec = proc.readyCheck.checkTimeout_msec;
                proc.healthCheck.pause_msec = proc.readyCheck.pause_msec;
            }

            if (it.getProp("healthFailThreshold").length() > 0)
                proc.healthFailThreshold = it.getIntProp("healthFailThreshold");

            // ignoreFail: if true, process failure won't stop launcher
            // Default: ignoreFail=false (process is critical)
            std::string ignoreFailStr = it.getProp("ignoreFail");
            proc.critical = !(ignoreFailStr == "true" || ignoreFailStr == "1");

            // Track SharedMemory process name
            if (proc.type == "SharedMemory")
                config.sharedMemoryName = proc.name;

            // Auto-inject --smemory-id for processes that need it
            if (tmpl->needsSharedMemory && !config.sharedMemoryName.empty())
            {
                // Check if --smemory-id is not already in args
                bool hasSmemoryId = false;

                for (const auto& arg : proc.args)
                {
                    if (arg == "--smemory-id")
                    {
                        hasSmemoryId = true;
                        break;
                    }
                }

                if (!hasSmemoryId)
                {
                    proc.args.push_back("--smemory-id");
                    proc.args.push_back(config.sharedMemoryName);
                }
            }
        }
        else
        {
            // No template - use explicit values (legacy mode)
            proc.command = it.getProp("command");

            // Parse arguments
            std::string argsStr = it.getProp("args");

            if (!argsStr.empty())
                proc.args = parseArgs(argsStr);

            // Ready check
            std::string readyCheckStr = it.getProp("readyCheck");

            if (!readyCheckStr.empty() && readyCheckStr != "none")
            {
                // Use explicitly specified readyCheck
                proc.readyCheck = HealthChecker::parseReadyCheck(readyCheckStr);
                proc.readyCheck.timeout_msec = it.getPIntProp("readyTimeout", proc.readyCheck.timeout_msec);
                proc.readyCheck.pause_msec = it.getPIntProp("checkPause", proc.readyCheck.pause_msec);
                proc.readyCheck.checkTimeout_msec = it.getPIntProp("checkTimeout", proc.readyCheck.checkTimeout_msec);
            }
            else if (readyCheckStr.empty() && !config.defaultReadyCheck.empty())
            {
                // Use global default readyCheck
                proc.readyCheck = HealthChecker::parseReadyCheck(config.defaultReadyCheck);
            }

            // Liveness check (watchdog): restart process if it stops responding
            std::string healthCheckStr = it.getProp("healthCheck");

            if (!healthCheckStr.empty() && healthCheckStr != "none")
            {
                proc.healthCheck = HealthChecker::parseReadyCheck(healthCheckStr);
                proc.healthCheck.checkTimeout_msec = it.getPIntProp("checkTimeout", proc.healthCheck.checkTimeout_msec);
                proc.healthCheck.pause_msec = it.getPIntProp("checkPause", proc.healthCheck.pause_msec);
            }

            if (it.getProp("healthFailThreshold").length() > 0)
                proc.healthFailThreshold = it.getIntProp("healthFailThreshold");

            // ignoreFail: if true, process failure won't stop launcher
            // Default: ignoreFail=false (process is critical)
            std::string ignoreFailStr = it.getProp("ignoreFail");
            proc.critical = !(ignoreFailStr == "true" || ignoreFailStr == "1");

            // Detect SharedMemory in legacy mode (by command name)
            if (proc.command == "uniset2-smemory" && config.sharedMemoryName.empty())
            {
                // Try to extract name from args (--smemory-id NAME)
                for (size_t i = 0; i + 1 < proc.args.size(); i++)
                {
                    if (proc.args[i] == "--smemory-id")
                    {
                        config.sharedMemoryName = proc.args[i + 1];
                        break;
                    }
                }
            }
        }

        // Parse rawArgs (standalone args without commonArgs) - always explicit
        std::string rawArgsStr = it.getProp("rawArgs");

        if (!rawArgsStr.empty())
            proc.rawArgs = parseArgs(rawArgsStr);

        proc.workDir = it.getProp("workDir");

        // afterRun hook
        proc.afterRun = it.getProp("afterRun");

        // Parse maxRestarts: -1 = no restart, 0 = infinite (default), >0 = limited
        if (it.getProp("maxRestarts").length() > 0)
            proc.maxRestarts = it.getIntProp("maxRestarts");
        else
            proc.maxRestarts = config.maxRestarts;

        if (it.getProp("restartDelay").length() > 0)
            proc.restartDelay_msec = it.getIntProp("restartDelay");
        else
            proc.restartDelay_msec = config.restartDelay_msec;

        if (it.getProp("maxRestartDelay").length() > 0)
            proc.maxRestartDelay_msec = it.getIntProp("maxRestartDelay");
        else
            proc.maxRestartDelay_msec = config.maxRestartDelay_msec;

        // Node filter
        std::string nodeFilterStr = it.getProp("nodeFilter");

        if (!nodeFilterStr.empty())
            proc.nodeFilter = parseNodeFilter(nodeFilterStr);

        // Skip flag
        proc.skip = it.getProp("skip") == "true" ||
                    it.getProp("skip") == "1";

        // Manual flag (start only via REST API)
        proc.manual = it.getProp("manual") == "true" ||
                      it.getProp("manual") == "1";

        // Oneshot flag (process runs once and exits)
        proc.oneshot = it.getProp("oneshot") == "true" ||
                       it.getProp("oneshot") == "1";

        if (it.getProp("oneshotTimeout").length() > 0)
            proc.oneshotTimeout_msec = it.getIntProp("oneshotTimeout");

        // Environment variables for this process
        UniXML::iterator eit(processNode);

        if (eit.goChildren())
        {
            do
            {
                if (eit.getName() == "env")
                {
                    std::string varName = eit.getProp("name");
                    std::string varValue = eit.getProp("value");

                    if (!varName.empty())
                        proc.env[varName] = varValue;
                }
            }
            while (eit.goNext());
        }

        config.processes[proc.name] = proc;
    }
    // -------------------------------------------------------------------------
    void ConfigLoader::loadEnvironment(xmlNode* launcherNode, LauncherConfig& config)
    {
        UniXML::iterator it(launcherNode);

        if (!it.goChildren())
            return;

        do
        {
            if (it.getName() == "Environment")
            {
                UniXML::iterator eit(it.getCurrent());

                if (!eit.goChildren())
                    continue;

                do
                {
                    if (eit.getName() == "var")
                    {
                        std::string varName = eit.getProp("name");
                        std::string varValue = eit.getProp("value");

                        if (!varName.empty())
                            config.environment[varName] = varValue;
                    }
                }
                while (eit.goNext());
            }
        }
        while (it.goNext());
    }
    // -------------------------------------------------------------------------
    std::vector<std::string> ConfigLoader::parseArgs(const std::string& argsStr)
    {
        std::vector<std::string> args;

        // Simple parsing: split by spaces, respect quotes
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
    std::set<std::string> ConfigLoader::parseNodeFilter(const std::string& filterStr)
    {
        std::set<std::string> nodes;
        std::istringstream iss(filterStr);
        std::string node;

        while (std::getline(iss, node, ','))
        {
            // Trim whitespace
            node.erase(0, node.find_first_not_of(" \t"));
            node.erase(node.find_last_not_of(" \t") + 1);

            if (!node.empty())
                nodes.insert(node);
        }

        return nodes;
    }
    // -------------------------------------------------------------------------
    bool ConfigLoader::needsNamingService(const std::shared_ptr<Configuration>& conf)
    {
        if (!conf)
            return true;  // Default: assume Naming Service is needed

        // Check localIOR parameter:
        // - localIOR=1 means use local IOR files, Naming Service NOT needed
        // - localIOR=0 or not set means use Naming Service
        int localIOR = conf->getArgPInt("--localIOR", conf->getPIntField("localIOR", 0));
        return (localIOR == 0);
    }
    // -------------------------------------------------------------------------
} // end of namespace uniset
