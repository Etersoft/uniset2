/*
 * Copyright (c) 2026 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 */
// -------------------------------------------------------------------------
#include <chrono>
#include <thread>
#include <fstream>
#include <Poco/Net/StreamSocket.h>
#include <Poco/Net/SocketAddress.h>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/URI.h>
#include <Poco/Timespan.h>
#include "HealthChecker.h"
#include "Debug.h"
// -------------------------------------------------------------------------
namespace uniset
{
    // -------------------------------------------------------------------------
    HealthChecker::HealthChecker(std::shared_ptr<Configuration> conf)
        : conf_(conf)
    {
        if (conf_)
            ui_ = std::make_shared<UInterface>(conf_);
    }
    // -------------------------------------------------------------------------
    bool HealthChecker::waitForReady(const ReadyCheck& check, size_t timeout_msec)
    {
        if (check.empty())
            return true;

        // Для CORBA используем waitReady напрямую с полным таймаутом
        if (check.type == ReadyCheckType::CORBA)
            return checkCORBA(check.target, timeout_msec, check.pause_msec);

        // Для остальных типов — цикл с периодическими проверками
        auto start = std::chrono::steady_clock::now();
        auto deadline = start + std::chrono::milliseconds(timeout_msec);

        while (std::chrono::steady_clock::now() < deadline)
        {
            if (checkOnce(check))
                return true;

            std::this_thread::sleep_for(std::chrono::milliseconds(check.pause_msec));
        }

        return false;
    }
    // -------------------------------------------------------------------------
    bool HealthChecker::checkOnce(const ReadyCheck& check)
    {
        switch (check.type)
        {
            case ReadyCheckType::None:
                return true;

            case ReadyCheckType::TCP:
                return checkTCP(check.target, check.checkTimeout_msec);

            case ReadyCheckType::CORBA:
                // CORBA обрабатывается в waitForReady с полным таймаутом
                // Здесь для однократной проверки используем checkTimeout_msec
                return checkCORBA(check.target, check.checkTimeout_msec, check.pause_msec);

            case ReadyCheckType::HTTP:
                return checkHTTP(check.target, check.checkTimeout_msec);

            case ReadyCheckType::File:
                return checkFile(check.target);
        }

        return false;
    }
    // -------------------------------------------------------------------------
    bool HealthChecker::isProcessAlive(Poco::Process::PID pid)
    {
        if (pid <= 0)
            return false;

        return Poco::Process::isRunning(pid);
    }
    // -------------------------------------------------------------------------
    ReadyCheck HealthChecker::parseReadyCheck(const std::string& checkStr)
    {
        ReadyCheck check;

        if (checkStr.empty())
            return check;

        auto pos = checkStr.find(':');

        if (pos == std::string::npos)
        {
            // Assume TCP port number
            check.type = ReadyCheckType::TCP;
            check.target = "localhost:" + checkStr;
            return check;
        }

        std::string typeStr = checkStr.substr(0, pos);
        std::string target = checkStr.substr(pos + 1);

        check.type = readyCheckTypeFromString(typeStr);
        check.target = target;

        // Add localhost if TCP without host
        if (check.type == ReadyCheckType::TCP && target.find(':') == std::string::npos)
            check.target = "localhost:" + target;

        return check;
    }
    // -------------------------------------------------------------------------
    bool HealthChecker::checkTCP(const std::string& hostPort, size_t timeout_msec)
    {
        try
        {
            Poco::Net::SocketAddress addr(hostPort);
            Poco::Net::StreamSocket socket;
            // Convert msec to Poco::Timespan (seconds, microseconds)
            Poco::Timespan timeout(timeout_msec / 1000, (timeout_msec % 1000) * 1000);
            socket.connect(addr, timeout);
            socket.close();
            return true;
        }
        catch (...)
        {
            return false;
        }
    }
    // -------------------------------------------------------------------------
    bool HealthChecker::checkCORBA(const std::string& objectName, size_t timeout_msec, size_t pause_msec)
    {
        if (!ui_)
            return false;

        try
        {
            // Ищем объект во всех секциях конфигурации (objects, controllers, services)
            ObjectId id = conf_->getAnyObjectID(objectName);

            if (id == DefaultObjectId)
                return false;

            // waitReady ждёт готовности объекта с заданным timeout и pause между проверками
            return ui_->waitReady(id, timeout_msec, pause_msec);
        }
        catch (...)
        {
            return false;
        }
    }
    // -------------------------------------------------------------------------
    bool HealthChecker::checkHTTP(const std::string& url, size_t timeout_msec)
    {
        try
        {
            Poco::URI uri(url);
            Poco::Net::HTTPClientSession session(uri.getHost(), uri.getPort());
            // Convert msec to Poco::Timespan (seconds, microseconds)
            Poco::Timespan timeout(timeout_msec / 1000, (timeout_msec % 1000) * 1000);
            session.setTimeout(timeout);

            std::string path = uri.getPathAndQuery();

            if (path.empty())
                path = "/";

            Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_GET, path);
            session.sendRequest(request);

            Poco::Net::HTTPResponse response;
            session.receiveResponse(response);

            return response.getStatus() == Poco::Net::HTTPResponse::HTTP_OK;
        }
        catch (...)
        {
            return false;
        }
    }
    // -------------------------------------------------------------------------
    bool HealthChecker::checkFile(const std::string& path)
    {
        std::ifstream f(path);
        return f.good();
    }
    // -------------------------------------------------------------------------
} // end of namespace uniset
