/*
 * Copyright (c) 2026 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 */
// -------------------------------------------------------------------------
#ifndef LauncherHttpRegistry_H_
#define LauncherHttpRegistry_H_
// -------------------------------------------------------------------------
#ifndef DISABLE_REST_API
// -------------------------------------------------------------------------
#include <memory>
#include "UHttpRequestHandler.h"
#include "ProcessManager.h"
// -------------------------------------------------------------------------
namespace uniset
{
    /*!
     * HTTP Registry for Launcher.
     * Implements IHttpRequestRegistry to work with UHttpServer.
     *
     * REST API endpoints:
     * - GET  /api/v2/launcher              - Overall status
     * - GET  /api/v2/launcher/status       - All processes status
     * - GET  /api/v2/launcher/processes    - List all processes
     * - GET  /api/v2/launcher/process/{name} - Specific process
     * - POST /api/v2/launcher/process/{name}/restart - Restart process
     * - GET  /api/v2/launcher/health       - Health check
     * - GET  /api/v2/launcher/groups       - Process groups
     * - GET  /api/v2/launcher/help         - API help
     */
    class LauncherHttpRegistry :
        public UHttp::IHttpRequestRegistry,
        public std::enable_shared_from_this<LauncherHttpRegistry>
    {
        public:
            explicit LauncherHttpRegistry(ProcessManager& pm);
            virtual ~LauncherHttpRegistry() = default;

            // Configuration
            void setReadToken(const std::string& token);
            void setControlToken(const std::string& token);
            void setHtmlTemplate(const std::string& path);

            // IHttpRequestRegistry interface
            Poco::JSON::Object::Ptr httpRequest(const UHttp::HttpRequestContext& ctx) override;
            Poco::JSON::Array::Ptr httpGetObjectsList(const UHttp::HttpRequestContext& ctx) override;
            Poco::JSON::Object::Ptr httpHelpRequest(const UHttp::HttpRequestContext& ctx) override;

            // Static file serving (HTML, JS)
            bool httpStaticRequest(
                const std::string& path,
                Poco::Net::HTTPServerRequest& req,
                Poco::Net::HTTPServerResponse& resp) override;

        private:
            Poco::JSON::Object::Ptr handleStatus();
            Poco::JSON::Object::Ptr handleProcesses();
            Poco::JSON::Object::Ptr handleProcess(const std::string& name);
            Poco::JSON::Object::Ptr handleRestart(const std::string& name);
            Poco::JSON::Object::Ptr handleStop(const std::string& name);
            Poco::JSON::Object::Ptr handleStart(const std::string& name);
            Poco::JSON::Object::Ptr handleHealth();
            Poco::JSON::Object::Ptr handleGroups();
            Poco::JSON::Object::Ptr handleHelp();

            Poco::JSON::Object::Ptr processToJSON(const ProcessInfo& proc);
            Poco::JSON::Object::Ptr groupToJSON(const ProcessGroup& group);

            // Authorization helpers
            bool checkReadAuth(const Poco::Net::HTTPServerRequest& req);
            bool checkControlAuth(const Poco::Net::HTTPServerRequest& req);
            static bool validateBearerToken(const Poco::Net::HTTPServerRequest& req,
                                            const std::string& expectedToken);

            // File serving helpers
            bool sendHtmlFile(const std::string& filename,
                              Poco::Net::HTTPServerRequest& req,
                              Poco::Net::HTTPServerResponse& resp);
            bool sendJsFile(const std::string& filename,
                            Poco::Net::HTTPServerRequest& req,
                            Poco::Net::HTTPServerResponse& resp);
            std::string findFile(const std::string& filename);
            std::string applyTemplateVars(const std::string& content);

            ProcessManager& pm_;
            std::string readToken_;
            std::string controlToken_;
            std::string htmlTemplatePath_;
    };

} // end of namespace uniset
// -------------------------------------------------------------------------
#endif // DISABLE_REST_API
// -------------------------------------------------------------------------
#endif // LauncherHttpRegistry_H_
// -------------------------------------------------------------------------
