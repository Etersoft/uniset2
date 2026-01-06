/*
 * Copyright (c) 2026 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 */
// -------------------------------------------------------------------------
#ifndef OmniNamesManager_H_
#define OmniNamesManager_H_
// -------------------------------------------------------------------------
#include <memory>
#include <string>
#include <Poco/Process.h>
#include "Debug.h"
// -------------------------------------------------------------------------
namespace uniset
{
    /*!
     * OmniNames lifecycle manager.
     * Handles automatic startup of omniNames for development/testing.
     *
     * Features:
     * - Auto-detect log directory ($TMPDIR/omniORB or $HOME/tmp/omniORB)
     * - Check if omniNames already running on port
     * - Create repository via uniset2-admin --create
     * - Graceful shutdown
     */
    class OmniNamesManager
    {
        public:
            OmniNamesManager();
            ~OmniNamesManager();

            // Configuration
            void setPort(int port);
            int getPort() const;

            void setLogDir(const std::string& dir);
            std::string getLogDir() const;

            void setConfFile(const std::string& confFile);

            /*!
             * Start omniNames if not already running.
             * \param runAdminCreate Run uniset2-admin --create after start
             * \return true if omniNames is running (started or was already running)
             */
            bool start(bool runAdminCreate = true);

            /*!
             * Stop omniNames (only if we started it).
             * \param timeout_msec Graceful shutdown timeout before SIGKILL
             */
            void stop(size_t timeout_msec = 5000);

            //! Check if omniNames is running on configured port
            bool isRunning() const;

            //! Check if we started omniNames (vs it was already running)
            bool wasStartedByUs() const;

            //! Wait for omniNames to be ready (TCP port listening)
            bool waitReady(size_t timeout_msec = 5000);

            //! Run uniset2-admin --create to initialize repository
            bool runAdminCreate();

            //! Calculate port from UID (defaultPort + 50000 + UID)
            static int calcPortFromUID(int defaultPort = 2809);

            //! Get default log directory
            static std::string getDefaultLogDir();

            // Logging
            std::shared_ptr<DebugStream> log();

        private:
            bool checkTcpPort(const std::string& host, int port, size_t timeout_msec = 1000) const;
            bool isFirstRun() const;

            int port_ = 0;
            std::string logDir_;
            std::string confFile_;
            Poco::Process::PID pid_ = 0;
            bool startedByUs_ = false;

            std::shared_ptr<DebugStream> mylog;
    };

} // end of namespace uniset
// -------------------------------------------------------------------------
#endif // OmniNamesManager_H_
// -------------------------------------------------------------------------
