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
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <Poco/Process.h>
#include <Poco/File.h>
#include <Poco/Exception.h>
#include <Poco/Net/StreamSocket.h>
#include <Poco/Net/SocketAddress.h>
#include "OmniNamesManager.h"
// -------------------------------------------------------------------------
namespace uniset
{
    // -------------------------------------------------------------------------
    OmniNamesManager::OmniNamesManager()
    {
        mylog = std::make_shared<DebugStream>();
        mylog->setLogName("OmniNames");
    }
    // -------------------------------------------------------------------------
    OmniNamesManager::~OmniNamesManager()
    {
        // Don't stop in destructor - explicit stop() should be called
    }
    // -------------------------------------------------------------------------
    void OmniNamesManager::setPort(int port)
    {
        port_ = port;
    }
    // -------------------------------------------------------------------------
    int OmniNamesManager::getPort() const
    {
        return port_;
    }
    // -------------------------------------------------------------------------
    void OmniNamesManager::setLogDir(const std::string& dir)
    {
        logDir_ = dir;
    }
    // -------------------------------------------------------------------------
    std::string OmniNamesManager::getLogDir() const
    {
        return logDir_.empty() ? getDefaultLogDir() : logDir_;
    }
    // -------------------------------------------------------------------------
    void OmniNamesManager::setConfFile(const std::string& confFile)
    {
        confFile_ = confFile;
    }
    // -------------------------------------------------------------------------
    int OmniNamesManager::calcPortFromUID(int defaultPort)
    {
        return defaultPort + 50000 + static_cast<int>(getuid());
    }
    // -------------------------------------------------------------------------
    std::string OmniNamesManager::getDefaultLogDir()
    {
        // Priority: $TMPDIR, then $HOME/tmp
        const char* tmpdir = std::getenv("TMPDIR");

        if (tmpdir && *tmpdir)
            return std::string(tmpdir) + "/omniORB";

        const char* home = std::getenv("HOME");

        if (home && *home)
            return std::string(home) + "/tmp/omniORB";

        // Fallback with UID for isolation
        return "/tmp/omniORB-" + std::to_string(getuid());
    }
    // -------------------------------------------------------------------------
    bool OmniNamesManager::isFirstRun() const
    {
        std::string dir = getLogDir();

        try
        {
            Poco::File f(dir);

            if (!f.exists())
                return true;

            // Check if there are any .dat files (omniNames data files)
            std::vector<std::string> files;
            f.list(files);

            for (const auto& file : files)
            {
                if (file.find(".dat") != std::string::npos)
                    return false;
            }

            return true;
        }
        catch (...)
        {
            return true;
        }
    }
    // -------------------------------------------------------------------------
    bool OmniNamesManager::checkTcpPort(const std::string& host, int port, size_t timeout_msec) const
    {
        try
        {
            Poco::Net::SocketAddress addr(host, static_cast<Poco::UInt16>(port));
            Poco::Net::StreamSocket socket;
            Poco::Timespan timeout(timeout_msec * 1000);  // microseconds
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
    bool OmniNamesManager::isRunning() const
    {
        if (port_ <= 0)
            return false;

        return checkTcpPort("localhost", port_, 500);
    }
    // -------------------------------------------------------------------------
    bool OmniNamesManager::wasStartedByUs() const
    {
        return startedByUs_;
    }
    // -------------------------------------------------------------------------
    bool OmniNamesManager::waitReady(size_t timeout_msec)
    {
        const size_t checkInterval = 100;  // ms
        size_t elapsed = 0;

        while (elapsed < timeout_msec)
        {
            if (isRunning())
                return true;

            std::this_thread::sleep_for(std::chrono::milliseconds(checkInterval));
            elapsed += checkInterval;
        }

        return false;
    }
    // -------------------------------------------------------------------------
    bool OmniNamesManager::start(bool runAdminCreateFlag)
    {
        if (port_ <= 0)
        {
            mylog->warn() << "Port not set" << std::endl;
            return false;
        }

        // Check if already running
        if (isRunning())
        {
            mylog->info() << "omniNames already running on port " << port_ << std::endl;
            startedByUs_ = false;

            // Still need to ensure repository is created
            if (runAdminCreateFlag && !confFile_.empty())
            {
                if (!runAdminCreate())
                    mylog->warn() << "Failed to create repository, but continuing..." << std::endl;
            }

            return true;
        }

        std::string dir = getLogDir();

        // Create log directory if needed
        try
        {
            Poco::File(dir).createDirectories();
        }
        catch (const std::exception& e)
        {
            mylog->warn() << "Failed to create log directory " << dir << ": " << e.what() << std::endl;
            return false;
        }

        // Build command arguments
        std::vector<std::string> args;
        bool firstRun = isFirstRun();

        if (firstRun)
        {
            mylog->info() << "First run, creating omniNames repository" << std::endl;
            args.push_back("-start");
            args.push_back(std::to_string(port_));
        }

        args.push_back("-logdir");
        args.push_back(dir);

        // Explicitly set ORB endpoint to override system config
        args.push_back("-ORBendPoint");
        args.push_back("giop:tcp::" + std::to_string(port_));

        // Start omniNames
        mylog->info() << "Starting omniNames on port " << port_
                      << " (logdir: " << dir << ")" << std::endl;

        try
        {
            Poco::ProcessHandle ph = Poco::Process::launch("omniNames", args);
            pid_ = ph.id();
            startedByUs_ = true;

            mylog->info() << "omniNames started with PID " << pid_ << std::endl;
        }
        catch (const std::exception& e)
        {
            mylog->warn() << "Failed to start omniNames: " << e.what() << std::endl;
            return false;
        }

        // Wait for ready
        if (!waitReady(5000))
        {
            mylog->warn() << "omniNames failed to become ready" << std::endl;
            stop(1000);
            return false;
        }

        mylog->info() << "omniNames is ready" << std::endl;

        // Run uniset2-admin --create if requested
        if (runAdminCreateFlag && !confFile_.empty())
        {
            if (!runAdminCreate())
            {
                mylog->warn() << "Failed to create repository, but continuing..." << std::endl;
                // Don't fail - omniNames is running, repository might already exist
            }
        }

        return true;
    }
    // -------------------------------------------------------------------------
    bool OmniNamesManager::runAdminCreate()
    {
        if (confFile_.empty())
        {
            mylog->warn() << "Config file not set, skipping admin --create" << std::endl;
            return false;
        }

        mylog->info() << "Running uniset2-admin --create" << std::endl;
        std::cout << "Creating CORBA repository..." << std::flush;

        std::string cmd = "uniset2-admin --confile " + confFile_ + " --create";

        if (port_ > 0)
            cmd += " -- --uniset-port " + std::to_string(port_);

        cmd += " >/dev/null 2>&1";

        // Temporarily ignore SIGCHLD to prevent ProcessManager from stealing wait() status
        struct sigaction sa_old, sa_new;
        sa_new.sa_handler = SIG_DFL;
        sigemptyset(&sa_new.sa_mask);
        sa_new.sa_flags = 0;
        sigaction(SIGCHLD, &sa_new, &sa_old);

        int ret = std::system(cmd.c_str());

        // Restore SIGCHLD handler
        sigaction(SIGCHLD, &sa_old, nullptr);

        if (ret == -1)
        {
            std::cout << " FAILED (system() error: " << strerror(errno) << ")" << std::endl;
            mylog->warn() << "system() failed: " << strerror(errno) << std::endl;
            return false;
        }

        if (WIFSIGNALED(ret))
        {
            std::cout << " FAILED (killed by signal " << WTERMSIG(ret) << ")" << std::endl;
            mylog->warn() << "uniset2-admin killed by signal " << WTERMSIG(ret) << std::endl;
            return false;
        }

        if (WIFEXITED(ret) && WEXITSTATUS(ret) != 0)
        {
            std::cout << " FAILED (exit code " << WEXITSTATUS(ret) << ")" << std::endl;
            mylog->warn() << "uniset2-admin --create exited with code " << WEXITSTATUS(ret) << std::endl;
            return false;
        }

        std::cout << " OK" << std::endl;
        mylog->info() << "Repository created successfully" << std::endl;
        return true;
    }
    // -------------------------------------------------------------------------
    void OmniNamesManager::stop(size_t timeout_msec)
    {
        if (!startedByUs_)
        {
            mylog->info() << "omniNames was not started by us, not stopping" << std::endl;
            return;
        }

        if (pid_ <= 0)
            return;

        mylog->info() << "Stopping omniNames (PID " << pid_ << ")" << std::endl;

        try
        {
            // Send SIGTERM
            Poco::Process::requestTermination(pid_);

            // Wait for graceful shutdown
            const size_t checkInterval = 100;
            size_t elapsed = 0;

            while (elapsed < timeout_msec)
            {
                if (!isRunning())
                {
                    mylog->info() << "omniNames stopped gracefully" << std::endl;
                    pid_ = 0;
                    startedByUs_ = false;
                    return;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(checkInterval));
                elapsed += checkInterval;
            }

            // Force kill
            mylog->warn() << "omniNames did not stop gracefully, sending SIGKILL" << std::endl;
            Poco::Process::kill(pid_);
        }
        catch (const std::exception& e)
        {
            mylog->warn() << "Error stopping omniNames: " << e.what() << std::endl;
        }

        pid_ = 0;
        startedByUs_ = false;
    }
    // -------------------------------------------------------------------------
    std::shared_ptr<DebugStream> OmniNamesManager::log()
    {
        return mylog;
    }
    // -------------------------------------------------------------------------
} // end of namespace uniset
// -------------------------------------------------------------------------
