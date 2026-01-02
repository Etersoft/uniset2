/*
 * Copyright (c) 2026 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 */
// -------------------------------------------------------------------------
#ifndef LauncherHTTPHandler_H_
#define LauncherHTTPHandler_H_
// -------------------------------------------------------------------------
#include <memory>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include "ProcessManager.h"
// -------------------------------------------------------------------------
namespace uniset
{
    /*!
     * HTTP Request Handler for Launcher REST API.
     *
     * REST API endpoints:
     * - GET  /api/v1/status           - Get all processes status
     * - GET  /api/v1/process/{name}   - Get specific process status
     * - POST /api/v1/process/{name}/restart - Restart process
     * - GET  /api/v1/health           - Health check (for Docker/K8s)
     * - GET  /api/v1/groups           - Get process groups
     */
    class LauncherHTTPHandler : public Poco::Net::HTTPRequestHandler
    {
        public:
            explicit LauncherHTTPHandler(ProcessManager& pm);

            void handleRequest(Poco::Net::HTTPServerRequest& request,
                               Poco::Net::HTTPServerResponse& response) override;

        private:
            void handleStatus(Poco::Net::HTTPServerResponse& response);
            void handleProcess(const std::string& name,
                               Poco::Net::HTTPServerResponse& response);
            void handleRestart(const std::string& name,
                               Poco::Net::HTTPServerResponse& response);
            void handleHealth(Poco::Net::HTTPServerResponse& response);
            void handleGroups(Poco::Net::HTTPServerResponse& response);

            void sendJSON(Poco::Net::HTTPServerResponse& response,
                          const std::string& json,
                          Poco::Net::HTTPResponse::HTTPStatus status =
                              Poco::Net::HTTPResponse::HTTP_OK);

            void sendError(Poco::Net::HTTPServerResponse& response,
                           Poco::Net::HTTPResponse::HTTPStatus status,
                           const std::string& message);

            std::string processToJSON(const ProcessInfo& proc);
            std::string groupToJSON(const ProcessGroup& group);

            ProcessManager& pm_;
    };

    /*!
     * HTTP Request Handler Factory for Launcher.
     */
    class LauncherHTTPHandlerFactory : public Poco::Net::HTTPRequestHandlerFactory
    {
        public:
            explicit LauncherHTTPHandlerFactory(ProcessManager& pm);

            Poco::Net::HTTPRequestHandler* createRequestHandler(
                const Poco::Net::HTTPServerRequest& request) override;

        private:
            ProcessManager& pm_;
    };

} // end of namespace uniset
// -------------------------------------------------------------------------
#endif // LauncherHTTPHandler_H_
// -------------------------------------------------------------------------
