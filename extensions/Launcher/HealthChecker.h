/*
 * Copyright (c) 2026 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 */
// -------------------------------------------------------------------------
#ifndef HealthChecker_H_
#define HealthChecker_H_
// -------------------------------------------------------------------------
#include <memory>
#include <string>
#include <Poco/Process.h>
#include "ProcessInfo.h"
#include "Configuration.h"
// -------------------------------------------------------------------------
namespace uniset
{
    /*!
     * Health checker for process readiness and liveness.
     * Supports multiple check types: TCP, CORBA, HTTP, File.
     */
    class HealthChecker
    {
        public:
            explicit HealthChecker(std::shared_ptr<Configuration> conf = nullptr);

            /*!
             * Check if process is ready based on ReadyCheck configuration.
             * Blocks until ready or timeout.
             * \param check ReadyCheck configuration
             * \param timeout_msec Maximum wait time in milliseconds
             * \return true if ready, false if timeout
             */
            bool waitForReady(const ReadyCheck& check, size_t timeout_msec);

            /*!
             * Single check (non-blocking).
             * \return true if check passes
             */
            bool checkOnce(const ReadyCheck& check);

            /*!
             * Check if process is alive by PID.
             * \param pid Process ID
             * \return true if process exists
             */
            static bool isProcessAlive(Poco::Process::PID pid);

            /*!
             * Parse ready check string like "tcp:2809" or "corba:SharedMemory"
             */
            static ReadyCheck parseReadyCheck(const std::string& checkStr);

        private:
            bool checkTCP(const std::string& hostPort, size_t timeout_msec);
            bool checkCORBA(const std::string& objectName, size_t timeout_msec, size_t pause_msec);
            bool checkHTTP(const std::string& url, size_t timeout_msec);
            bool checkFile(const std::string& path);

            std::shared_ptr<Configuration> conf_;
    };

} // end of namespace uniset
// -------------------------------------------------------------------------
#endif // HealthChecker_H_
// -------------------------------------------------------------------------
