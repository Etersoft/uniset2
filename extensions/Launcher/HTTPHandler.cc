/*
 * Copyright (c) 2026 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 */
// -------------------------------------------------------------------------
#include <sstream>
#include <Poco/URI.h>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Array.h>
#include <Poco/JSON/Stringifier.h>
#include "HTTPHandler.h"
// -------------------------------------------------------------------------
namespace uniset
{
    // -------------------------------------------------------------------------
    LauncherHTTPHandler::LauncherHTTPHandler(ProcessManager& pm)
        : pm_(pm)
    {
    }
    // -------------------------------------------------------------------------
    void LauncherHTTPHandler::handleRequest(Poco::Net::HTTPServerRequest& request,
                                            Poco::Net::HTTPServerResponse& response)
    {
        Poco::URI uri(request.getURI());
        std::string path = uri.getPath();
        std::string method = request.getMethod();

        // Parse path segments
        std::vector<std::string> segments;
        std::string segment;

        for (char c : path)
        {
            if (c == '/')
            {
                if (!segment.empty())
                {
                    segments.push_back(segment);
                    segment.clear();
                }
            }
            else
            {
                segment += c;
            }
        }

        if (!segment.empty())
            segments.push_back(segment);

        // Route request
        // /api/v1/status
        if (segments.size() >= 3 &&
                segments[0] == "api" && segments[1] == "v1")
        {
            if (segments[2] == "status" && method == "GET")
            {
                handleStatus(response);
                return;
            }

            if (segments[2] == "health" && method == "GET")
            {
                handleHealth(response);
                return;
            }

            if (segments[2] == "groups" && method == "GET")
            {
                handleGroups(response);
                return;
            }

            // /api/v1/process/{name}
            if (segments[2] == "process" && segments.size() >= 4)
            {
                std::string processName = segments[3];

                // /api/v1/process/{name}/restart
                if (segments.size() >= 5 && segments[4] == "restart" && method == "POST")
                {
                    handleRestart(processName, response);
                    return;
                }

                // GET /api/v1/process/{name}
                if (method == "GET")
                {
                    handleProcess(processName, response);
                    return;
                }
            }
        }

        // Not found
        sendError(response, Poco::Net::HTTPResponse::HTTP_NOT_FOUND, "Not found");
    }
    // -------------------------------------------------------------------------
    void LauncherHTTPHandler::handleStatus(Poco::Net::HTTPServerResponse& response)
    {
        Poco::JSON::Object root;
        root.set("node", pm_.getNodeName());

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

        root.set("processes", processes);
        root.set("allRunning", pm_.allRunning());
        root.set("anyCriticalFailed", pm_.anyCriticalFailed());

        std::ostringstream oss;
        Poco::JSON::Stringifier::stringify(root, oss, 2);
        sendJSON(response, oss.str());
    }
    // -------------------------------------------------------------------------
    void LauncherHTTPHandler::handleProcess(const std::string& name,
                                            Poco::Net::HTTPServerResponse& response)
    {
        auto proc = pm_.getProcessInfo(name);

        if (proc.name.empty())
        {
            sendError(response, Poco::Net::HTTPResponse::HTTP_NOT_FOUND,
                      "Process not found: " + name);
            return;
        }

        Poco::JSON::Object obj;
        obj.set("name", proc.name);
        obj.set("command", proc.command);
        obj.set("state", to_string(proc.state));
        obj.set("pid", proc.pid);
        obj.set("group", proc.group);
        obj.set("critical", proc.critical);
        obj.set("skip", proc.skip);
        obj.set("oneshot", proc.oneshot);
        obj.set("maxRestarts", proc.maxRestarts);
        obj.set("restartCount", proc.restartCount);
        obj.set("lastExitCode", proc.lastExitCode);

        if (!proc.lastError.empty())
            obj.set("lastError", proc.lastError);

        // Arguments
        Poco::JSON::Array args;

        for (const auto& arg : proc.args)
            args.add(arg);

        obj.set("args", args);

        // Ready check
        if (!proc.readyCheck.empty())
        {
            Poco::JSON::Object rc;
            rc.set("type", to_string(proc.readyCheck.type));
            rc.set("target", proc.readyCheck.target);
            rc.set("timeout_msec", proc.readyCheck.timeout_msec);
            obj.set("readyCheck", rc);
        }

        std::ostringstream oss;
        Poco::JSON::Stringifier::stringify(obj, oss, 2);
        sendJSON(response, oss.str());
    }
    // -------------------------------------------------------------------------
    void LauncherHTTPHandler::handleRestart(const std::string& name,
                                            Poco::Net::HTTPServerResponse& response)
    {
        bool ok = pm_.restartProcess(name);

        Poco::JSON::Object obj;
        obj.set("process", name);
        obj.set("success", ok);

        if (!ok)
            obj.set("error", "Failed to restart process");

        std::ostringstream oss;
        Poco::JSON::Stringifier::stringify(obj, oss, 2);

        sendJSON(response, oss.str(),
                 ok ? Poco::Net::HTTPResponse::HTTP_OK
                 : Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
    }
    // -------------------------------------------------------------------------
    void LauncherHTTPHandler::handleHealth(Poco::Net::HTTPServerResponse& response)
    {
        bool healthy = pm_.allRunning() && !pm_.anyCriticalFailed();

        Poco::JSON::Object obj;
        obj.set("status", healthy ? "healthy" : "unhealthy");
        obj.set("allRunning", pm_.allRunning());
        obj.set("anyCriticalFailed", pm_.anyCriticalFailed());

        std::ostringstream oss;
        Poco::JSON::Stringifier::stringify(obj, oss);

        sendJSON(response, oss.str(),
                 healthy ? Poco::Net::HTTPResponse::HTTP_OK
                 : Poco::Net::HTTPResponse::HTTP_SERVICE_UNAVAILABLE);
    }
    // -------------------------------------------------------------------------
    void LauncherHTTPHandler::handleGroups(Poco::Net::HTTPServerResponse& response)
    {
        Poco::JSON::Array groups;
        auto allGroups = pm_.getAllGroups();

        for (const auto& group : allGroups)
        {
            Poco::JSON::Object obj;
            obj.set("name", group.name);
            obj.set("order", group.order);

            Poco::JSON::Array depends;

            for (const auto& dep : group.depends)
                depends.add(dep);

            obj.set("depends", depends);

            Poco::JSON::Array processes;

            for (const auto& proc : group.processes)
                processes.add(proc);

            obj.set("processes", processes);

            groups.add(obj);
        }

        std::ostringstream oss;
        Poco::JSON::Stringifier::stringify(groups, oss, 2);
        sendJSON(response, oss.str());
    }
    // -------------------------------------------------------------------------
    void LauncherHTTPHandler::sendJSON(Poco::Net::HTTPServerResponse& response,
                                       const std::string& json,
                                       Poco::Net::HTTPResponse::HTTPStatus status)
    {
        response.setStatus(status);
        response.setContentType("application/json");
        response.setContentLength(json.length());
        response.send() << json;
    }
    // -------------------------------------------------------------------------
    void LauncherHTTPHandler::sendError(Poco::Net::HTTPServerResponse& response,
                                        Poco::Net::HTTPResponse::HTTPStatus status,
                                        const std::string& message)
    {
        Poco::JSON::Object obj;
        obj.set("error", message);

        std::ostringstream oss;
        Poco::JSON::Stringifier::stringify(obj, oss);

        sendJSON(response, oss.str(), status);
    }
    // -------------------------------------------------------------------------
    // Factory
    // -------------------------------------------------------------------------
    LauncherHTTPHandlerFactory::LauncherHTTPHandlerFactory(ProcessManager& pm)
        : pm_(pm)
    {
    }
    // -------------------------------------------------------------------------
    Poco::Net::HTTPRequestHandler* LauncherHTTPHandlerFactory::createRequestHandler(
        const Poco::Net::HTTPServerRequest& request)
    {
        return new LauncherHTTPHandler(pm_);
    }
    // -------------------------------------------------------------------------
} // end of namespace uniset
