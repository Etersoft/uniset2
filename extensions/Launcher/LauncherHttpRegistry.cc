/*
 * Copyright (c) 2026 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 */
// -------------------------------------------------------------------------
#ifndef DISABLE_REST_API
// -------------------------------------------------------------------------
#include <sstream>
#include <Poco/JSON/Array.h>
#include "LauncherHttpRegistry.h"
#include "Exceptions.h"
// -------------------------------------------------------------------------
namespace uniset
{
    // -------------------------------------------------------------------------
    LauncherHttpRegistry::LauncherHttpRegistry(ProcessManager& pm)
        : pm_(pm)
    {
    }
    // -------------------------------------------------------------------------
    Poco::JSON::Object::Ptr LauncherHttpRegistry::httpRequest(const UHttp::HttpRequestContext& ctx)
    {
        // Only handle requests for "launcher" object
        if (ctx.objectName != "launcher")
            throw uniset::NameNotFound("Object not found: " + ctx.objectName);

        const std::string& method = ctx.request.getMethod();

        // No path = status
        if (ctx.depth() == 0)
            return handleStatus();

        const std::string& cmd = ctx[0];

        if (cmd == "status" && method == "GET")
            return handleStatus();

        if (cmd == "processes" && method == "GET")
            return handleProcesses();

        if (cmd == "health" && method == "GET")
            return handleHealth();

        if (cmd == "groups" && method == "GET")
            return handleGroups();

        // /process/{name}
        if (cmd == "process" && ctx.depth() >= 2)
        {
            const std::string& processName = ctx[1];

            // /process/{name}/restart
            if (ctx.depth() >= 3 && ctx[2] == "restart" && method == "POST")
                return handleRestart(processName);

            // GET /process/{name}
            if (method == "GET")
                return handleProcess(processName);
        }

        throw uniset::NameNotFound("Unknown command: " + ctx.pathString());
    }
    // -------------------------------------------------------------------------
    Poco::JSON::Array::Ptr LauncherHttpRegistry::httpGetObjectsList(const UHttp::HttpRequestContext& ctx)
    {
        auto arr = new Poco::JSON::Array();
        arr->add("launcher");
        return arr;
    }
    // -------------------------------------------------------------------------
    Poco::JSON::Object::Ptr LauncherHttpRegistry::httpHelpRequest(const UHttp::HttpRequestContext& ctx)
    {
        return handleHelp();
    }
    // -------------------------------------------------------------------------
    Poco::JSON::Object::Ptr LauncherHttpRegistry::handleStatus()
    {
        auto root = new Poco::JSON::Object();
        root->set("node", pm_.getNodeName());

        Poco::JSON::Array processes;
        auto allProcs = pm_.getAllProcesses();

        for (const auto& proc : allProcs)
        {
            Poco::JSON::Object obj;
            obj.set("name", proc.name);
            obj.set("state", to_string(proc.state));
            obj.set("pid", proc.pid);
            obj.set("group", proc.group);
            obj.set("critical", proc.critical);
            obj.set("skip", proc.skip);
            obj.set("oneshot", proc.oneshot);
            obj.set("restartCount", proc.restartCount);

            if (!proc.lastError.empty())
                obj.set("lastError", proc.lastError);

            processes.add(obj);
        }

        root->set("processes", processes);
        root->set("allRunning", pm_.allRunning());
        root->set("anyCriticalFailed", pm_.anyCriticalFailed());

        return root;
    }
    // -------------------------------------------------------------------------
    Poco::JSON::Object::Ptr LauncherHttpRegistry::handleProcesses()
    {
        auto root = new Poco::JSON::Object();

        Poco::JSON::Array processes;
        auto allProcs = pm_.getAllProcesses();

        for (const auto& proc : allProcs)
            processes.add(processToJSON(proc));

        root->set("processes", processes);
        root->set("count", static_cast<int>(allProcs.size()));

        return root;
    }
    // -------------------------------------------------------------------------
    Poco::JSON::Object::Ptr LauncherHttpRegistry::handleProcess(const std::string& name)
    {
        auto proc = pm_.getProcessInfo(name);

        if (proc.name.empty())
            throw uniset::NameNotFound("Process not found: " + name);

        return processToJSON(proc);
    }
    // -------------------------------------------------------------------------
    Poco::JSON::Object::Ptr LauncherHttpRegistry::handleRestart(const std::string& name)
    {
        bool ok = pm_.restartProcess(name);

        auto obj = new Poco::JSON::Object();
        obj->set("process", name);
        obj->set("success", ok);

        if (!ok)
            obj->set("error", "Failed to restart process");

        return obj;
    }
    // -------------------------------------------------------------------------
    Poco::JSON::Object::Ptr LauncherHttpRegistry::handleHealth()
    {
        bool healthy = pm_.allRunning() && !pm_.anyCriticalFailed();

        auto obj = new Poco::JSON::Object();
        obj->set("status", healthy ? "healthy" : "unhealthy");
        obj->set("allRunning", pm_.allRunning());
        obj->set("anyCriticalFailed", pm_.anyCriticalFailed());

        return obj;
    }
    // -------------------------------------------------------------------------
    Poco::JSON::Object::Ptr LauncherHttpRegistry::handleGroups()
    {
        auto root = new Poco::JSON::Object();

        Poco::JSON::Array groups;
        auto allGroups = pm_.getAllGroups();

        for (const auto& group : allGroups)
            groups.add(groupToJSON(group));

        root->set("groups", groups);
        root->set("count", static_cast<int>(allGroups.size()));

        return root;
    }
    // -------------------------------------------------------------------------
    Poco::JSON::Object::Ptr LauncherHttpRegistry::handleHelp()
    {
        auto root = new Poco::JSON::Object();
        root->set("object", "launcher");

        Poco::JSON::Array commands;

        // status
        {
            Poco::JSON::Object cmd;
            cmd.set("name", "status");
            cmd.set("desc", "Get overall launcher status with all processes");
            cmd.set("method", "GET");
            commands.add(cmd);
        }

        // processes
        {
            Poco::JSON::Object cmd;
            cmd.set("name", "processes");
            cmd.set("desc", "List all processes");
            cmd.set("method", "GET");
            commands.add(cmd);
        }

        // process/{name}
        {
            Poco::JSON::Object cmd;
            cmd.set("name", "process/{name}");
            cmd.set("desc", "Get specific process details");
            cmd.set("method", "GET");

            Poco::JSON::Array params;
            Poco::JSON::Object param;
            param.set("name", "name");
            param.set("desc", "Process name");
            params.add(param);
            cmd.set("parameters", params);

            commands.add(cmd);
        }

        // process/{name}/restart
        {
            Poco::JSON::Object cmd;
            cmd.set("name", "process/{name}/restart");
            cmd.set("desc", "Restart a process");
            cmd.set("method", "POST");

            Poco::JSON::Array params;
            Poco::JSON::Object param;
            param.set("name", "name");
            param.set("desc", "Process name");
            params.add(param);
            cmd.set("parameters", params);

            commands.add(cmd);
        }

        // health
        {
            Poco::JSON::Object cmd;
            cmd.set("name", "health");
            cmd.set("desc", "Health check for Docker/Kubernetes");
            cmd.set("method", "GET");
            commands.add(cmd);
        }

        // groups
        {
            Poco::JSON::Object cmd;
            cmd.set("name", "groups");
            cmd.set("desc", "Get process groups");
            cmd.set("method", "GET");
            commands.add(cmd);
        }

        root->set("help", commands);

        return root;
    }
    // -------------------------------------------------------------------------
    Poco::JSON::Object::Ptr LauncherHttpRegistry::processToJSON(const ProcessInfo& proc)
    {
        auto obj = new Poco::JSON::Object();
        obj->set("name", proc.name);
        obj->set("command", proc.command);
        obj->set("state", to_string(proc.state));
        obj->set("pid", proc.pid);
        obj->set("group", proc.group);
        obj->set("critical", proc.critical);
        obj->set("skip", proc.skip);
        obj->set("oneshot", proc.oneshot);
        obj->set("restartOnFailure", proc.restartOnFailure);
        obj->set("maxRestarts", proc.maxRestarts);
        obj->set("restartCount", proc.restartCount);
        obj->set("lastExitCode", proc.lastExitCode);

        if (!proc.lastError.empty())
            obj->set("lastError", proc.lastError);

        // Arguments
        Poco::JSON::Array args;

        for (const auto& arg : proc.args)
            args.add(arg);

        obj->set("args", args);

        // Node filter
        if (!proc.nodeFilter.empty())
        {
            Poco::JSON::Array nodes;

            for (const auto& node : proc.nodeFilter)
                nodes.add(node);

            obj->set("nodeFilter", nodes);
        }

        // Ready check
        if (!proc.readyCheck.empty())
        {
            Poco::JSON::Object rc;
            rc.set("type", to_string(proc.readyCheck.type));
            rc.set("target", proc.readyCheck.target);
            rc.set("timeout_msec", proc.readyCheck.timeout_msec);
            obj->set("readyCheck", rc);
        }

        return obj;
    }
    // -------------------------------------------------------------------------
    Poco::JSON::Object::Ptr LauncherHttpRegistry::groupToJSON(const ProcessGroup& group)
    {
        auto obj = new Poco::JSON::Object();
        obj->set("name", group.name);
        obj->set("order", group.order);

        Poco::JSON::Array depends;

        for (const auto& dep : group.depends)
            depends.add(dep);

        obj->set("depends", depends);

        Poco::JSON::Array processes;

        for (const auto& proc : group.processes)
            processes.add(proc);

        obj->set("processes", processes);

        return obj;
    }
    // -------------------------------------------------------------------------
} // end of namespace uniset
// -------------------------------------------------------------------------
#endif // DISABLE_REST_API
// -------------------------------------------------------------------------
