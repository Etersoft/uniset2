/*
 * Copyright (c) 2026 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 */
// -------------------------------------------------------------------------
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <cerrno>
#include <cstring>
#include <regex>
#include <sys/wait.h>
#include <Poco/Pipe.h>
#include <Poco/PipeStream.h>
#include "ProcessManager.h"
#include "ConfigLoader.h"
// -------------------------------------------------------------------------
namespace uniset
{
    // -------------------------------------------------------------------------
    ProcessManager::ProcessManager(std::shared_ptr<Configuration> conf)
        : conf_(conf)
        , healthChecker_(std::make_unique<HealthChecker>(conf))
        , mylog(std::make_shared<DebugStream>())
    {
        mylog->setLogName("Launcher");
    }
    // -------------------------------------------------------------------------
    ProcessManager::~ProcessManager()
    {
        stopMonitoring();
        stopAll();
    }
    // -------------------------------------------------------------------------
    void ProcessManager::setNodeName(const std::string& name)
    {
        nodeName_ = name;
    }
    // -------------------------------------------------------------------------
    std::string ProcessManager::getNodeName() const
    {
        return nodeName_;
    }
    // -------------------------------------------------------------------------
    void ProcessManager::setHealthCheckInterval(size_t msec)
    {
        healthCheckInterval_msec_ = msec;
    }
    // -------------------------------------------------------------------------
    void ProcessManager::setRestartWindow(size_t msec)
    {
        restartWindow_msec_ = msec;
    }
    // -------------------------------------------------------------------------
    void ProcessManager::setCommonArgs(const std::vector<std::string>& args)
    {
        commonArgs_ = args;
    }
    // -------------------------------------------------------------------------
    void ProcessManager::setPassthroughArgs(const std::string& args)
    {
        passthroughArgs_ = args;
    }
    // -------------------------------------------------------------------------
    void ProcessManager::setForwardArgs(const std::vector<std::string>& args)
    {
        forwardArgs_ = args;
    }
    // -------------------------------------------------------------------------
    void ProcessManager::addProcess(const ProcessInfo& proc)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        processes_[proc.name] = proc;
    }
    // -------------------------------------------------------------------------
    void ProcessManager::addGroup(const ProcessGroup& group)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        groups_[group.name] = group;
        depResolver_.addGroup(group.name, group.depends);
    }
    // -------------------------------------------------------------------------
    std::vector<std::string> ProcessManager::resolveStartOrder()
    {
        return depResolver_.resolve();
    }
    // -------------------------------------------------------------------------
    bool ProcessManager::startAll()
    {
        std::lock_guard<std::mutex> lock(mutex_);

        mylog->info() << "Starting all processes..." << std::endl;

        stopping_ = false;

        // Get groups in dependency order
        std::vector<std::string> groupOrder;

        try
        {
            groupOrder = depResolver_.resolve();
        }
        catch (const std::exception& e)
        {
            mylog->crit() << "Failed to resolve dependencies: " << e.what() << std::endl;
            return false;
        }

        // Start each group
        for (const auto& groupName : groupOrder)
        {
            if (stopping_)
                break;

            auto git = groups_.find(groupName);

            if (git == groups_.end())
                continue;

            mylog->info() << "Starting group: " << groupName << std::endl;

            // Start all processes in this group
            for (const auto& procName : git->second.processes)
            {
                if (stopping_)
                    break;

                auto pit = processes_.find(procName);

                if (pit == processes_.end())
                {
                    mylog->warn() << "Process not found: " << procName << std::endl;
                    continue;
                }

                // Check skip flag
                if (pit->second.skip)
                {
                    mylog->info() << "Skipping " << procName << " (skip=1)" << std::endl;
                    continue;
                }

                // Check node filter
                if (!pit->second.shouldRunOnNode(nodeName_))
                {
                    mylog->info() << "Skipping " << procName
                                  << " (not for node " << nodeName_ << ")" << std::endl;
                    continue;
                }

                if (!startProcess(pit->second))
                {
                    if (pit->second.critical)
                    {
                        mylog->crit() << "Critical process " << procName
                                      << " failed to start" << std::endl;
                        return false;
                    }
                }
            }
        }

        mylog->info() << "All processes started" << std::endl;
        return true;
    }
    // -------------------------------------------------------------------------
    bool ProcessManager::startProcess(ProcessInfo& proc)
    {
        mylog->info() << "Starting process: " << proc.name << std::endl;

        proc.state = ProcessState::Starting;
        proc.lastStartTime = std::chrono::steady_clock::now();

        // Oneshot processes: run and wait for completion
        if (proc.oneshot)
        {
            std::vector<std::string> args;

            if (!proc.rawArgs.empty())
                args = proc.rawArgs;
            else
            {
                for (const auto& arg : commonArgs_)
                    args.push_back(arg);

                for (const auto& arg : proc.args)
                    args.push_back(arg);
            }

            // Add forwarded args (unknown launcher args like --uniset-port)
            for (const auto& arg : forwardArgs_)
                args.push_back(arg);

            // Add passthrough args (from -- on command line)
            if (!passthroughArgs_.empty())
                args.push_back(passthroughArgs_);

            // Expand environment variables
            expandEnvironment(args);

            mylog->info() << "Running oneshot: " << proc.command;

            for (const auto& arg : args)
                *mylog << " " << arg;

            *mylog << std::endl;

            try
            {
                Poco::Pipe outPipe;
                std::string workDir = proc.workDir.empty() ? "." : proc.workDir;

                Poco::ProcessHandle ph = Poco::Process::launch(
                                             proc.command,
                                             args,
                                             workDir,
                                             nullptr,    // stdin
                                             &outPipe,   // stdout
                                             &outPipe    // stderr -> stdout
                                         );

                proc.pid = ph.id();

                // Wait for process to complete (do this before reading to avoid race)
                int exitCode = -1;

                try
                {
                    exitCode = ph.wait();
                }
                catch (const std::exception& waitEx)
                {
                    mylog->warn() << "wait() exception: " << waitEx.what() << std::endl;
                    // Process may have already been reaped, check if it's still alive
                    if (!HealthChecker::isProcessAlive(proc.pid))
                        exitCode = 0;  // Assume success if we can't determine
                }

                // Read output
                Poco::PipeInputStream istr(outPipe);
                std::string line;

                while (std::getline(istr, line))
                {
                    if (!line.empty())
                        mylog->info() << "  " << line << std::endl;
                }
                proc.lastExitCode = exitCode;
                proc.pid = 0;

                mylog->info() << proc.name << " exited with code " << exitCode << std::endl;

                if (exitCode == 0)
                {
                    proc.state = ProcessState::Completed;
                    mylog->info() << proc.name << " completed successfully" << std::endl;

                    if (onStarted_)
                        onStarted_(proc);

                    return true;
                }
                else
                {
                    proc.state = ProcessState::Failed;
                    proc.lastError = "Exit code " + std::to_string(exitCode);
                    mylog->crit() << proc.name << " failed with exit code " << exitCode << std::endl;

                    if (onFailed_)
                        onFailed_(proc);

                    return false;
                }
            }
            catch (const std::exception& e)
            {
                proc.state = ProcessState::Failed;
                proc.lastError = e.what();
                mylog->crit() << "Failed to run oneshot " << proc.name << ": " << e.what() << std::endl;

                if (onFailed_)
                    onFailed_(proc);

                return false;
            }
        }

        // Prepare arguments
        std::vector<std::string> args;

        // If rawArgs is set, use only rawArgs (no commonArgs)
        if (!proc.rawArgs.empty())
        {
            args = proc.rawArgs;
        }
        else
        {
            // Use commonArgs + args
            args.reserve(commonArgs_.size() + proc.args.size());

            for (const auto& arg : commonArgs_)
                args.push_back(arg);

            for (const auto& arg : proc.args)
                args.push_back(arg);
        }

        // Add forwarded args (unknown launcher args like --uniset-port)
        for (const auto& arg : forwardArgs_)
            args.push_back(arg);

        // Add passthrough args (from -- on command line)
        if (!passthroughArgs_.empty())
            args.push_back(passthroughArgs_);

        // Expand environment variables
        expandEnvironment(args);

        // Prepare environment
        Poco::Process::Env env;

        for (const auto& kv : proc.env)
            env[kv.first] = expandEnvVar(kv.second);

        try
        {
            // Use current directory if workDir is not specified
            std::string workDir = proc.workDir.empty() ? "." : proc.workDir;

            Poco::ProcessHandle ph = Poco::Process::launch(
                                         proc.command,
                                         args,
                                         workDir,
                                         nullptr,  // stdin
                                         nullptr,  // stdout
                                         nullptr,  // stderr
                                         env
                                     );

            proc.pid = ph.id();
            mylog->info() << proc.name << " started with PID " << proc.pid << std::endl;
        }
        catch (const std::exception& e)
        {
            proc.state = ProcessState::Failed;
            proc.lastError = e.what();
            mylog->crit() << "Failed to start " << proc.name << ": " << e.what() << std::endl;

            if (onFailed_)
                onFailed_(proc);

            return false;
        }

        // Wait for ready check (daemon processes)
        if (!waitForProcessReady(proc))
        {
            mylog->warn() << proc.name << " ready check failed" << std::endl;
            proc.state = ProcessState::Failed;
            proc.lastError = "Ready check timeout";

            if (onFailed_)
                onFailed_(proc);

            return false;
        }

        // Run afterRun hook if specified
        if (!proc.afterRun.empty())
        {
            mylog->info() << "Running afterRun hook for " << proc.name << ": " << proc.afterRun << std::endl;

            // Expand environment variables in afterRun
            std::string afterRunCmd = expandEnvVar(proc.afterRun);

            // Execute via shell
            int ret = std::system(afterRunCmd.c_str());

            if (ret != 0)
            {
                mylog->warn() << "afterRun hook failed with code " << ret << std::endl;
                // Don't fail the process, just warn
            }
            else
            {
                mylog->info() << "afterRun hook completed successfully" << std::endl;
            }
        }

        proc.state = ProcessState::Running;
        mylog->info() << proc.name << " is ready" << std::endl;

        if (onStarted_)
            onStarted_(proc);

        return true;
    }
    // -------------------------------------------------------------------------
    bool ProcessManager::waitForProcessReady(ProcessInfo& proc)
    {
        if (proc.readyCheck.empty())
            return true;

        return healthChecker_->waitForReady(proc.readyCheck, proc.readyCheck.timeout_msec);
    }
    // -------------------------------------------------------------------------
    int ProcessManager::waitForProcessExit(ProcessInfo& proc)
    {
        if (proc.pid <= 0)
            return -1;

        auto deadline = std::chrono::steady_clock::now() +
                        std::chrono::milliseconds(proc.oneshotTimeout_msec);

        while (std::chrono::steady_clock::now() < deadline)
        {
            int status = 0;
            pid_t result = waitpid(proc.pid, &status, WNOHANG);

            if (result > 0)
            {
                // Process has exited
                if (WIFEXITED(status))
                {
                    int exitCode = WEXITSTATUS(status);
                    mylog->info() << proc.name << " exited with code " << exitCode << std::endl;
                    return exitCode;
                }
                else if (WIFSIGNALED(status))
                {
                    int sig = WTERMSIG(status);
                    mylog->warn() << proc.name << " terminated by signal " << sig << std::endl;
                    return 128 + sig;
                }

                return -1;  // Unknown exit reason
            }
            else if (result == 0)
            {
                // Process still running, wait a bit
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            else
            {
                // result < 0 - error (e.g., ECHILD - no child process)
                if (errno == ECHILD)
                {
                    mylog->warn() << proc.name << " process already reaped (ECHILD)" << std::endl;
                    return -1;  // Can't determine exit code
                }

                mylog->warn() << proc.name << " waitpid error: " << strerror(errno) << std::endl;
                return -1;
            }
        }

        // Timeout - kill the process
        mylog->warn() << proc.name << " oneshot timeout, killing" << std::endl;

        try
        {
            Poco::Process::kill(proc.pid);
            // Wait for process to be reaped
            waitpid(proc.pid, nullptr, 0);
        }
        catch (...) {}

        return -1;  // Timeout
    }
    // -------------------------------------------------------------------------
    void ProcessManager::stopAll()
    {
        std::lock_guard<std::mutex> lock(mutex_);

        mylog->info() << "Stopping all processes..." << std::endl;
        stopping_ = true;

        // Get groups in reverse order
        std::vector<std::string> groupOrder;

        try
        {
            groupOrder = depResolver_.resolveReverse();
        }
        catch (...)
        {
            // If dependency resolution fails, just stop all processes
            for (auto& kv : processes_)
                stopProcess(kv.second);

            return;
        }

        // Stop each group in reverse order
        for (const auto& groupName : groupOrder)
        {
            auto git = groups_.find(groupName);

            if (git == groups_.end())
                continue;

            mylog->info() << "Stopping group: " << groupName << std::endl;

            for (const auto& procName : git->second.processes)
            {
                auto pit = processes_.find(procName);

                if (pit != processes_.end())
                    stopProcess(pit->second);
            }
        }

        mylog->info() << "All processes stopped" << std::endl;
    }
    // -------------------------------------------------------------------------
    void ProcessManager::stopProcess(ProcessInfo& proc)
    {
        if (proc.state == ProcessState::Stopped ||
                proc.state == ProcessState::Completed)
            return;

        mylog->info() << "Stopping process: " << proc.name << " (PID " << proc.pid << ")" << std::endl;

        proc.state = ProcessState::Stopping;

        if (proc.pid > 0 && HealthChecker::isProcessAlive(proc.pid))
        {
            try
            {
                // Send SIGTERM
                Poco::Process::requestTermination(proc.pid);

                // Wait up to 5 seconds for graceful shutdown
                for (int i = 0; i < 50; i++)
                {
                    if (!HealthChecker::isProcessAlive(proc.pid))
                        break;

                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }

                // Force kill if still running
                if (HealthChecker::isProcessAlive(proc.pid))
                {
                    mylog->warn() << proc.name << " did not stop gracefully, killing" << std::endl;
                    Poco::Process::kill(proc.pid);
                }
            }
            catch (const std::exception& e)
            {
                mylog->warn() << "Error stopping " << proc.name << ": " << e.what() << std::endl;
            }
        }

        proc.state = ProcessState::Stopped;
        proc.pid = 0;

        if (onStopped_)
            onStopped_(proc);
    }
    // -------------------------------------------------------------------------
    bool ProcessManager::restartProcess(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = processes_.find(name);

        if (it == processes_.end())
        {
            mylog->warn() << "Process not found: " << name << std::endl;
            return false;
        }

        mylog->info() << "Restarting process: " << name << std::endl;

        // Set restarting state for UI feedback
        it->second.state = ProcessState::Restarting;

        stopProcess(it->second);
        it->second.reset();
        return startProcess(it->second);
    }
    // -------------------------------------------------------------------------
    bool ProcessManager::stopProcess(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = processes_.find(name);

        if (it == processes_.end())
        {
            mylog->warn() << "Process not found: " << name << std::endl;
            return false;
        }

        mylog->info() << "Stopping process by name: " << name << std::endl;
        stopProcess(it->second);
        return true;
    }
    // -------------------------------------------------------------------------
    bool ProcessManager::startProcess(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = processes_.find(name);

        if (it == processes_.end())
        {
            mylog->warn() << "Process not found: " << name << std::endl;
            return false;
        }

        // Skip if process is marked to skip or not for this node
        if (it->second.skip)
        {
            mylog->warn() << "Cannot start " << name << " (skip=true)" << std::endl;
            return false;
        }

        if (!it->second.shouldRunOnNode(nodeName_))
        {
            mylog->warn() << "Cannot start " << name
                          << " (not for node " << nodeName_ << ")" << std::endl;
            return false;
        }

        mylog->info() << "Starting process by name: " << name << std::endl;
        it->second.reset();
        return startProcess(it->second);
    }
    // -------------------------------------------------------------------------
    void ProcessManager::startMonitoring()
    {
        if (running_)
            return;

        running_ = true;
        monitorThread_ = std::thread(&ProcessManager::monitorLoop, this);
        mylog->info() << "Process monitoring started" << std::endl;
    }
    // -------------------------------------------------------------------------
    void ProcessManager::stopMonitoring()
    {
        if (!running_)
            return;

        running_ = false;

        if (monitorThread_.joinable())
            monitorThread_.join();

        mylog->info() << "Process monitoring stopped" << std::endl;
    }
    // -------------------------------------------------------------------------
    bool ProcessManager::isMonitoring() const
    {
        return running_;
    }
    // -------------------------------------------------------------------------
    void ProcessManager::monitorLoop()
    {
        while (running_)
        {
            {
                std::lock_guard<std::mutex> lock(mutex_);

                for (auto& kv : processes_)
                {
                    auto& proc = kv.second;

                    // Skip oneshot processes - they already completed
                    if (proc.oneshot)
                        continue;

                    if (proc.state != ProcessState::Running)
                        continue;

                    if (!HealthChecker::isProcessAlive(proc.pid))
                    {
                        mylog->warn() << proc.name << " (PID " << proc.pid
                                      << ") is no longer running" << std::endl;

                        // Try to get exit code
                        int exitCode = -1;

                        try
                        {
                            // Note: This may not work if process already reaped
                            exitCode = proc.lastExitCode;
                        }
                        catch (...) {}

                        handleProcessExit(proc, exitCode);
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(healthCheckInterval_msec_));
        }
    }
    // -------------------------------------------------------------------------
    void ProcessManager::handleProcessExit(ProcessInfo& proc, int exitCode)
    {
        proc.state = ProcessState::Failed;
        proc.lastExitCode = exitCode;
        proc.lastFailTime = std::chrono::steady_clock::now();

        mylog->crit() << proc.name << " exited with code " << exitCode << std::endl;

        if (onFailed_)
            onFailed_(proc);

        // Automatic restart logic:
        // - maxRestarts=-1: no restart (explicit disable)
        // - maxRestarts=0: infinite restarts (default)
        // - maxRestarts>0: limited restarts
        // - ignoreFail=true (critical=false): no restart
        if (!stopping_)
        {
            // Check if restart is disabled
            bool noRestart = (proc.maxRestarts < 0) || !proc.critical;

            if (noRestart)
            {
                if (proc.maxRestarts < 0)
                {
                    mylog->info() << proc.name << " restart disabled (maxRestarts=-1)" << std::endl;
                }
                else
                {
                    mylog->info() << proc.name << " restart disabled (ignoreFail=true)" << std::endl;
                }

                // Critical process with disabled restart - stop everything
                if (proc.critical && proc.maxRestarts < 0)
                {
                    mylog->crit() << "Critical process " << proc.name
                                  << " failed, initiating shutdown" << std::endl;
                    stopping_ = true;
                }

                return;
            }

            // Check restart window
            auto now = std::chrono::steady_clock::now();
            auto sinceStart = std::chrono::duration_cast<std::chrono::milliseconds>(
                                  now - proc.lastStartTime).count();

            // If process ran long enough, reset restart counter
            if (sinceStart > (long)restartWindow_msec_)
                proc.restartCount = 0;

            // maxRestarts=0 means infinite restarts
            bool canRestart = (proc.maxRestarts == 0) || (proc.restartCount < proc.maxRestarts);

            if (canRestart)
            {
                proc.restartCount++;

                // Exponential backoff: delay = min(initialDelay * 2^(attempt-1), maxDelay)
                size_t delay = proc.restartDelay_msec;

                if (proc.restartCount > 1)
                {
                    delay = proc.restartDelay_msec * (1 << (proc.restartCount - 1));

                    if (delay > proc.maxRestartDelay_msec)
                        delay = proc.maxRestartDelay_msec;
                }

                // Set restarting state before delay
                proc.state = ProcessState::Restarting;

                if (proc.maxRestarts == 0)
                {
                    mylog->info() << "Restarting " << proc.name
                                  << " (attempt " << proc.restartCount << ", delay " << delay << "ms)"
                                  << std::endl;
                }
                else
                {
                    mylog->info() << "Restarting " << proc.name
                                  << " (attempt " << proc.restartCount << "/" << proc.maxRestarts
                                  << ", delay " << delay << "ms)" << std::endl;
                }

                // Delay before restart (exponential backoff)
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));

                startProcess(proc);
            }
            else
            {
                mylog->crit() << proc.name << " exceeded max restarts ("
                              << proc.maxRestarts << "), giving up" << std::endl;

                // Critical process exhausted restarts - stop everything
                if (proc.critical)
                {
                    mylog->crit() << "Critical process " << proc.name
                                  << " cannot restart, initiating shutdown" << std::endl;
                    stopping_ = true;
                }
            }
        }
    }
    // -------------------------------------------------------------------------
    ProcessState ProcessManager::getProcessState(const std::string& name) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = processes_.find(name);

        if (it != processes_.end())
            return it->second.state;

        return ProcessState::Stopped;
    }
    // -------------------------------------------------------------------------
    ProcessInfo ProcessManager::getProcessInfo(const std::string& name) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = processes_.find(name);

        if (it != processes_.end())
            return it->second;

        return ProcessInfo{};
    }
    // -------------------------------------------------------------------------
    std::vector<ProcessInfo> ProcessManager::getAllProcesses() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<ProcessInfo> result;
        result.reserve(processes_.size());

        for (const auto& kv : processes_)
            result.push_back(kv.second);

        return result;
    }
    // -------------------------------------------------------------------------
    std::vector<ProcessGroup> ProcessManager::getAllGroups() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<ProcessGroup> result;
        result.reserve(groups_.size());

        for (const auto& kv : groups_)
            result.push_back(kv.second);

        return result;
    }
    // -------------------------------------------------------------------------
    bool ProcessManager::allRunning() const
    {
        std::lock_guard<std::mutex> lock(mutex_);

        for (const auto& kv : processes_)
        {
            // Skip processes that are not meant to run
            if (kv.second.skip)
                continue;

            if (!kv.second.shouldRunOnNode(nodeName_))
                continue;

            // Oneshot processes should be Completed, daemons should be Running
            if (kv.second.oneshot)
            {
                if (kv.second.state != ProcessState::Completed)
                    return false;
            }
            else
            {
                if (kv.second.state != ProcessState::Running)
                    return false;
            }
        }

        return true;
    }
    // -------------------------------------------------------------------------
    bool ProcessManager::anyCriticalFailed() const
    {
        std::lock_guard<std::mutex> lock(mutex_);

        for (const auto& kv : processes_)
        {
            if (kv.second.critical && kv.second.state == ProcessState::Failed)
                return true;
        }

        return false;
    }
    // -------------------------------------------------------------------------
    void ProcessManager::setOnProcessStarted(ProcessCallback cb)
    {
        onStarted_ = std::move(cb);
    }
    // -------------------------------------------------------------------------
    void ProcessManager::setOnProcessStopped(ProcessCallback cb)
    {
        onStopped_ = std::move(cb);
    }
    // -------------------------------------------------------------------------
    void ProcessManager::setOnProcessFailed(ProcessCallback cb)
    {
        onFailed_ = std::move(cb);
    }
    // -------------------------------------------------------------------------
    std::shared_ptr<DebugStream> ProcessManager::log()
    {
        return mylog;
    }
    // -------------------------------------------------------------------------
    void ProcessManager::expandEnvironment(std::vector<std::string>& args)
    {
        for (auto& arg : args)
            arg = expandEnvVar(arg);
    }
    // -------------------------------------------------------------------------
    std::string ProcessManager::expandEnvVar(const std::string& s)
    {
        std::string result = s;

        // Replace ${VAR} patterns
        std::regex envRegex(R"(\$\{([^}]+)\})");
        std::smatch match;

        while (std::regex_search(result, match, envRegex))
        {
            std::string varName = match[1].str();
            const char* value = std::getenv(varName.c_str());
            std::string replacement = value ? value : "";

            result = result.substr(0, match.position()) +
                     replacement +
                     result.substr(match.position() + match.length());
        }

        // Replace $VAR patterns (without braces)
        std::regex envRegex2(R"(\$([A-Za-z_][A-Za-z0-9_]*))");

        while (std::regex_search(result, match, envRegex2))
        {
            std::string varName = match[1].str();
            const char* value = std::getenv(varName.c_str());
            std::string replacement = value ? value : "";

            result = result.substr(0, match.position()) +
                     replacement +
                     result.substr(match.position() + match.length());
        }

        return result;
    }
    // -------------------------------------------------------------------------
    void ProcessManager::printRunList(std::ostream& out) const
    {
        std::lock_guard<std::mutex> lock(mutex_);

        out << "=== Launcher Run List for " << nodeName_ << " ===" << std::endl;
        out << std::endl;

        // Show Naming Service status
        bool needNaming = ConfigLoader::needsNamingService(conf_);
        out << "Naming Service: " << (needNaming ? "REQUIRED (localIOR=0)" : "NOT NEEDED (localIOR=1)") << std::endl;
        out << std::endl;

        // Get groups in dependency order
        std::vector<std::string> groupOrder;

        try
        {
            groupOrder = depResolver_.resolve();
        }
        catch (const std::exception& e)
        {
            out << "ERROR: Failed to resolve dependencies: " << e.what() << std::endl;
            return;
        }

        int processNum = 1;
        std::vector<std::string> skipped;

        for (const auto& groupName : groupOrder)
        {
            auto git = groups_.find(groupName);

            if (git == groups_.end())
                continue;

            const auto& group = git->second;

            // Print group header
            out << "Group " << group.order << ": " << groupName;

            if (!group.depends.empty())
            {
                out << " (depends: ";
                bool first = true;

                for (const auto& dep : group.depends)
                {
                    if (!first)
                        out << ", ";

                    out << dep;
                    first = false;
                }

                out << ")";
            }

            out << std::endl;

            // Print processes in this group
            for (const auto& procName : group.processes)
            {
                auto pit = processes_.find(procName);

                if (pit == processes_.end())
                    continue;

                const auto& proc = pit->second;

                // Check if should run on this node
                if (!proc.shouldRunOnNode(nodeName_))
                {
                    std::string filterStr;

                    for (const auto& n : proc.nodeFilter)
                    {
                        if (!filterStr.empty())
                            filterStr += ",";

                        filterStr += n;
                    }

                    skipped.push_back(proc.name + " (nodeFilter: " + filterStr + ")");
                    continue;
                }

                // Skip if marked to skip
                if (proc.skip)
                {
                    skipped.push_back(proc.name + " (skip=true)");
                    continue;
                }

                out << "  [" << processNum++ << "] " << proc.name;

                if (proc.oneshot)
                    out << " (oneshot)";

                out << std::endl;

                // Print type if detected
                if (!proc.type.empty())
                    out << "      Type: " << proc.type << std::endl;

                // Build command line
                out << "      Command: " << proc.command;

                // Add args
                std::vector<std::string> allArgs;

                if (!proc.rawArgs.empty())
                {
                    allArgs = proc.rawArgs;
                }
                else
                {
                    allArgs = commonArgs_;
                    allArgs.insert(allArgs.end(), proc.args.begin(), proc.args.end());
                }

                for (const auto& arg : allArgs)
                    out << " " << arg;

                // Add forwarded args
                for (const auto& arg : forwardArgs_)
                    out << " " << arg;

                // Add passthrough args
                if (!passthroughArgs_.empty())
                    out << " " << passthroughArgs_;

                out << std::endl;

                // Print ready check
                if (!proc.readyCheck.empty())
                {
                    out << "      Ready: " << to_string(proc.readyCheck.type) << ":"
                        << proc.readyCheck.target
                        << " (timeout: " << proc.readyCheck.timeout_msec << "ms)"
                        << std::endl;
                }

                if (!proc.critical)
                    out << "      ignoreFail: yes" << std::endl;
                out << std::endl;
            }
        }

        // Print skipped processes
        if (!skipped.empty())
        {
            out << "Skipped:" << std::endl;

            for (const auto& s : skipped)
                out << "  - " << s << std::endl;

            out << std::endl;
        }

        out << "Total: " << (processNum - 1) << " processes to start" << std::endl;
    }
    // -------------------------------------------------------------------------
} // end of namespace uniset
