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
#include <fstream>
#include <thread>
#include <Poco/JSON/Array.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/File.h>
#include "LauncherHttpRegistry.h"
#include "Exceptions.h"
#include "Configuration.h"

#ifndef UNISET_DATADIR
#define UNISET_DATADIR "/usr/share/uniset2/"
#endif
// -------------------------------------------------------------------------
namespace uniset
{
    // -------------------------------------------------------------------------
    LauncherHttpRegistry::LauncherHttpRegistry(ProcessManager& pm)
        : pm_(pm)
    {
    }
    // -------------------------------------------------------------------------
    void LauncherHttpRegistry::setReadToken(const std::string& token)
    {
        readToken_ = token;
    }
    // -------------------------------------------------------------------------
    void LauncherHttpRegistry::setControlToken(const std::string& token)
    {
        controlToken_ = token;
    }
    // -------------------------------------------------------------------------
    void LauncherHttpRegistry::setHtmlTemplate(const std::string& path)
    {
        htmlTemplatePath_ = path;
    }
    // -------------------------------------------------------------------------
    Poco::JSON::Object::Ptr LauncherHttpRegistry::httpRequest(const UHttp::HttpRequestContext& ctx)
    {
        // Only handle requests for "launcher" object
        if (ctx.objectName != "launcher")
            throw uniset::NameNotFound("Object not found: " + ctx.objectName);

        const std::string& method = ctx.request.getMethod();

        // Check read authorization for GET requests (if read token is set)
        if (method == "GET" && !checkReadAuth(ctx.request))
        {
            ctx.response.setStatus(Poco::Net::HTTPResponse::HTTP_UNAUTHORIZED);
            auto obj = new Poco::JSON::Object();
            obj->set("error", "unauthorized");
            obj->set("message", "missing or invalid read token");
            return obj;
        }

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

        // /restart-all - restart all running processes
        if (cmd == "restart-all" && method == "POST")
        {
            // Check if control is enabled
            if (controlToken_.empty())
            {
                ctx.response.setStatus(Poco::Net::HTTPResponse::HTTP_FORBIDDEN);
                auto obj = new Poco::JSON::Object();
                obj->set("error", "forbidden");
                obj->set("message", "control operations disabled (--control-token not set)");
                return obj;
            }

            // Check control authorization
            if (!checkControlAuth(ctx.request))
            {
                ctx.response.setStatus(Poco::Net::HTTPResponse::HTTP_UNAUTHORIZED);
                auto obj = new Poco::JSON::Object();
                obj->set("error", "unauthorized");
                obj->set("message", "missing or invalid control token");
                return obj;
            }

            return handleRestartAll();
        }

        // /reload-all - stop all, then start all (except skip, manual)
        if (cmd == "reload-all" && method == "POST")
        {
            // Check if control is enabled
            if (controlToken_.empty())
            {
                ctx.response.setStatus(Poco::Net::HTTPResponse::HTTP_FORBIDDEN);
                auto obj = new Poco::JSON::Object();
                obj->set("error", "forbidden");
                obj->set("message", "control operations disabled (--control-token not set)");
                return obj;
            }

            // Check control authorization
            if (!checkControlAuth(ctx.request))
            {
                ctx.response.setStatus(Poco::Net::HTTPResponse::HTTP_UNAUTHORIZED);
                auto obj = new Poco::JSON::Object();
                obj->set("error", "unauthorized");
                obj->set("message", "missing or invalid control token");
                return obj;
            }

            return handleReloadAll();
        }

        // /auth - validate control token
        if (cmd == "auth" && method == "GET")
        {
            // Check if control is enabled
            if (controlToken_.empty())
            {
                ctx.response.setStatus(Poco::Net::HTTPResponse::HTTP_FORBIDDEN);
                auto obj = new Poco::JSON::Object();
                obj->set("error", "forbidden");
                obj->set("message", "control operations disabled");
                return obj;
            }

            // Check control authorization
            if (!checkControlAuth(ctx.request))
            {
                ctx.response.setStatus(Poco::Net::HTTPResponse::HTTP_UNAUTHORIZED);
                auto obj = new Poco::JSON::Object();
                obj->set("error", "unauthorized");
                obj->set("message", "invalid control token");
                return obj;
            }

            auto obj = new Poco::JSON::Object();
            obj->set("success", true);
            obj->set("message", "control token valid");
            return obj;
        }

        // /process/{name}
        if (cmd == "process" && ctx.depth() >= 2)
        {
            const std::string& processName = ctx[1];

            // Control operations (POST)
            if (ctx.depth() >= 3 && method == "POST")
            {
                const std::string& action = ctx[2];

                // Check if control is enabled
                if (controlToken_.empty())
                {
                    ctx.response.setStatus(Poco::Net::HTTPResponse::HTTP_FORBIDDEN);
                    auto obj = new Poco::JSON::Object();
                    obj->set("error", "forbidden");
                    obj->set("message", "control operations disabled (--control-token not set)");
                    return obj;
                }

                // Check control authorization
                if (!checkControlAuth(ctx.request))
                {
                    ctx.response.setStatus(Poco::Net::HTTPResponse::HTTP_UNAUTHORIZED);
                    auto obj = new Poco::JSON::Object();
                    obj->set("error", "unauthorized");
                    obj->set("message", "missing or invalid control token");
                    return obj;
                }

                if (action == "restart")
                    return handleRestart(processName);

                if (action == "stop")
                    return handleStop(processName);

                if (action == "start")
                    return handleStart(processName);
            }

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
#ifdef PACKAGE_VERSION
        root->set("version", PACKAGE_VERSION);
#else
        root->set("version", "unknown");
#endif

        Poco::JSON::Array processes;
        auto allProcs = pm_.getAllProcesses();

        for (const auto& proc : allProcs)
        {
            Poco::JSON::Object obj;
            obj.set("name", proc.name);
            obj.set("command", proc.command);
            obj.set("state", to_string(proc.state));
            obj.set("pid", proc.pid);
            obj.set("group", proc.group);
            obj.set("critical", proc.critical);
            obj.set("skip", proc.skip);
            obj.set("manual", proc.manual);
            obj.set("oneshot", proc.oneshot);
            obj.set("restartCount", proc.restartCount);
            obj.set("maxRestarts", proc.maxRestarts);

            // Full arguments (commonArgs + args + forwardArgs)
            Poco::JSON::Array args;
            for (const auto& arg : pm_.getFullArgs(proc.name))
                args.add(arg);
            obj.set("args", args);

            // Calculate uptime for running processes
            if (proc.state == ProcessState::Running || proc.state == ProcessState::Completed)
            {
                auto now = std::chrono::steady_clock::now();
                auto uptime = std::chrono::duration_cast<std::chrono::seconds>(
                                  now - proc.lastStartTime).count();
                obj.set("uptime", static_cast<int>(uptime));
            }

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
    Poco::JSON::Object::Ptr LauncherHttpRegistry::handleStop(const std::string& name)
    {
        bool ok = pm_.stopProcess(name);

        auto obj = new Poco::JSON::Object();
        obj->set("process", name);
        obj->set("success", ok);

        if (!ok)
            obj->set("error", "Failed to stop process");

        return obj;
    }
    // -------------------------------------------------------------------------
    Poco::JSON::Object::Ptr LauncherHttpRegistry::handleStart(const std::string& name)
    {
        bool ok = pm_.startProcess(name);

        auto obj = new Poco::JSON::Object();
        obj->set("process", name);
        obj->set("success", ok);

        if (!ok)
            obj->set("error", "Failed to start process");

        return obj;
    }
    // -------------------------------------------------------------------------
    Poco::JSON::Object::Ptr LauncherHttpRegistry::handleRestartAll()
    {
        // Run restartAll asynchronously so HTTP response returns immediately
        // This allows client to poll for "restarting" state during the operation
        std::thread([this]()
        {
            pm_.restartAll();
        }).detach();

        auto obj = new Poco::JSON::Object();
        obj->set("success", true);
        obj->set("message", "Restart all initiated");

        return obj;
    }
    // -------------------------------------------------------------------------
    Poco::JSON::Object::Ptr LauncherHttpRegistry::handleReloadAll()
    {
        // Run reloadAll asynchronously so HTTP response returns immediately
        // This allows client to poll for "restarting" state during the operation
        std::thread([this]()
        {
            pm_.reloadAll();
        }).detach();

        auto obj = new Poco::JSON::Object();
        obj->set("success", true);
        obj->set("message", "Reload all initiated");

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
            cmd.set("desc", "Restart a process (requires control token)");
            cmd.set("method", "POST");

            Poco::JSON::Array params;
            Poco::JSON::Object param;
            param.set("name", "name");
            param.set("desc", "Process name");
            params.add(param);
            cmd.set("parameters", params);

            commands.add(cmd);
        }

        // process/{name}/stop
        {
            Poco::JSON::Object cmd;
            cmd.set("name", "process/{name}/stop");
            cmd.set("desc", "Stop a process (requires control token)");
            cmd.set("method", "POST");

            Poco::JSON::Array params;
            Poco::JSON::Object param;
            param.set("name", "name");
            param.set("desc", "Process name");
            params.add(param);
            cmd.set("parameters", params);

            commands.add(cmd);
        }

        // process/{name}/start
        {
            Poco::JSON::Object cmd;
            cmd.set("name", "process/{name}/start");
            cmd.set("desc", "Start a process (requires control token)");
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

        // auth
        {
            Poco::JSON::Object cmd;
            cmd.set("name", "auth");
            cmd.set("desc", "Validate control token (for Take Control)");
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
        root->set("controlEnabled", !controlToken_.empty());
        root->set("readAuthRequired", !readToken_.empty());

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
        obj->set("manual", proc.manual);
        obj->set("oneshot", proc.oneshot);
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
    bool LauncherHttpRegistry::httpStaticRequest(
        const std::string& path,
        Poco::Net::HTTPServerRequest& req,
        Poco::Net::HTTPServerResponse& resp)
    {
        // Check read authorization (if read token is set)
        if (!checkReadAuth(req))
        {
            resp.setStatus(Poco::Net::HTTPResponse::HTTP_UNAUTHORIZED);
            resp.setContentType("application/json");
            std::ostream& out = resp.send();
            out << R"({"error":"unauthorized","message":"missing or invalid read token"})";
            out.flush();
            return true;
        }

        if (path == "/" || path == "/ui" || path == "/launcher.html")
            return sendHtmlFile("launcher.html", req, resp);

        if (path == "/launcher-app.js")
            return sendJsFile("launcher-app.js", req, resp);

        return false;  // Not our path
    }
    // -------------------------------------------------------------------------
    bool LauncherHttpRegistry::checkReadAuth(const Poco::Net::HTTPServerRequest& req)
    {
        // If read token is not set, access is open
        if (readToken_.empty())
            return true;

        return validateBearerToken(req, readToken_);
    }
    // -------------------------------------------------------------------------
    bool LauncherHttpRegistry::checkControlAuth(const Poco::Net::HTTPServerRequest& req)
    {
        // If control token is not set, control is disabled
        if (controlToken_.empty())
            return false;

        return validateBearerToken(req, controlToken_);
    }
    // -------------------------------------------------------------------------
    bool LauncherHttpRegistry::validateBearerToken(
        const Poco::Net::HTTPServerRequest& req,
        const std::string& expectedToken)
    {
        const std::string auth = req.get("Authorization", "");

        if (auth.size() <= 7)
            return false;

        if (auth.substr(0, 7) != "Bearer ")
            return false;

        return auth.substr(7) == expectedToken;
    }
    // -------------------------------------------------------------------------
    bool LauncherHttpRegistry::sendHtmlFile(
        const std::string& filename,
        Poco::Net::HTTPServerRequest& req,
        Poco::Net::HTTPServerResponse& resp)
    {
        std::string filepath;

        // Use custom template if specified
        if (!htmlTemplatePath_.empty() && filename == "launcher.html")
            filepath = htmlTemplatePath_;
        else
            filepath = findFile(filename);

        if (filepath.empty())
        {
            resp.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_FOUND);
            resp.setContentType("application/json");
            std::ostream& out = resp.send();
            out << R"({"error":"not found","message":")" << filename << R"( not found"})";
            out.flush();
            return true;
        }

        std::ifstream file(filepath);

        if (!file.is_open())
        {
            resp.setStatus(Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
            resp.setContentType("application/json");
            std::ostream& out = resp.send();
            out << R"({"error":"internal error","message":"failed to open )" << filename << R"("})";
            out.flush();
            return true;
        }

        std::ostringstream ss;
        ss << file.rdbuf();
        std::string content = applyTemplateVars(ss.str());

        resp.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
        resp.setContentType("text/html; charset=UTF-8");
        resp.setContentLength(content.size());
        std::ostream& out = resp.send();
        out << content;
        out.flush();

        return true;
    }
    // -------------------------------------------------------------------------
    bool LauncherHttpRegistry::sendJsFile(
        const std::string& filename,
        Poco::Net::HTTPServerRequest& req,
        Poco::Net::HTTPServerResponse& resp)
    {
        std::string filepath = findFile(filename);

        if (filepath.empty())
        {
            resp.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_FOUND);
            resp.setContentType("application/json");
            std::ostream& out = resp.send();
            out << R"({"error":"not found","message":")" << filename << R"( not found"})";
            out.flush();
            return true;
        }

        std::ifstream file(filepath);

        if (!file.is_open())
        {
            resp.setStatus(Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
            resp.setContentType("application/json");
            std::ostream& out = resp.send();
            out << R"({"error":"internal error","message":"failed to open )" << filename << R"("})";
            out.flush();
            return true;
        }

        std::ostringstream ss;
        ss << file.rdbuf();
        std::string content = ss.str();

        resp.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
        resp.setContentType("application/javascript; charset=UTF-8");
        resp.setContentLength(content.size());
        std::ostream& out = resp.send();
        out << content;
        out.flush();

        return true;
    }
    // -------------------------------------------------------------------------
    std::string LauncherHttpRegistry::findFile(const std::string& filename)
    {
        // If absolute or relative path specified, use as-is
        if (!filename.empty() && (filename[0] == '/' || filename[0] == '.'))
        {
            if (Poco::File(filename).exists())
                return filename;

            return "";
        }

        // Build search path: current dir, conf->getDataDir(), UNISET_DATADIR
        std::vector<std::string> paths = { "./" };

        try
        {
            auto conf = uniset_conf();

            if (conf)
            {
                std::string dataDir = conf->getDataDir();

                if (!dataDir.empty())
                    paths.push_back(dataDir + "/");
            }
        }
        catch (...) {}

#ifdef UNISET_DATADIR
        paths.push_back(std::string(UNISET_DATADIR) + "/");
#endif

        // Search in all paths
        for (const auto& p : paths)
        {
            std::string fullPath = p + filename;

            if (Poco::File(fullPath).exists())
                return fullPath;
        }

        return "";  // Not found
    }
    // -------------------------------------------------------------------------
    std::string LauncherHttpRegistry::applyTemplateVars(const std::string& content)
    {
        std::string result = content;

        // Replace {{NODE_NAME}}
        std::string nodeName = pm_.getNodeName();
        size_t pos = 0;

        while ((pos = result.find("{{NODE_NAME}}", pos)) != std::string::npos)
        {
            result.replace(pos, 13, nodeName);
            pos += nodeName.size();
        }

        // Replace {{API_URL}} - empty string means relative URLs (same origin)
        pos = 0;

        while ((pos = result.find("{{API_URL}}", pos)) != std::string::npos)
        {
            result.replace(pos, 11, "");
            pos += 0;
        }

        // Replace {{CONTROL_ENABLED}}
        std::string controlEnabled = controlToken_.empty() ? "false" : "true";
        pos = 0;

        while ((pos = result.find("{{CONTROL_ENABLED}}", pos)) != std::string::npos)
        {
            result.replace(pos, 19, controlEnabled);
            pos += controlEnabled.size();
        }

        // Replace {{READ_AUTH_REQUIRED}}
        std::string readAuthRequired = readToken_.empty() ? "false" : "true";
        pos = 0;

        while ((pos = result.find("{{READ_AUTH_REQUIRED}}", pos)) != std::string::npos)
        {
            result.replace(pos, 22, readAuthRequired);
            pos += readAuthRequired.size();
        }

        return result;
    }
    // -------------------------------------------------------------------------
} // end of namespace uniset
// -------------------------------------------------------------------------
#endif // DISABLE_REST_API
// -------------------------------------------------------------------------
