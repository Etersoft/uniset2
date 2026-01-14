/*
 * Copyright (c) 2026 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 */
// -------------------------------------------------------------------------
#include <algorithm>
#include <set>
#include <cstdlib>
#include <cstdio>
#include <cerrno>
#include <cstring>
#include <regex>
#include <fstream>
#include <sys/wait.h>
#include <signal.h>
#include <dirent.h>
#include <Poco/Pipe.h>
#include <Poco/PipeStream.h>
#include "ProcessManager.h"
#include "ConfigLoader.h"
// -------------------------------------------------------------------------
namespace uniset
{
    // -------------------------------------------------------------------------
    // Find all child processes of a given PID by reading /proc
    static std::vector<pid_t> getChildPids(pid_t parentPid)
    {
        std::vector<pid_t> children;
        DIR* dir = opendir("/proc");

        if (!dir)
            return children;

        struct dirent* entry;

        while ((entry = readdir(dir)) != nullptr)
        {
            // Skip non-numeric entries
            if (entry->d_type != DT_DIR)
                continue;

            char* endp;
            long pid = strtol(entry->d_name, &endp, 10);

            if (*endp != '\0' || pid <= 0)
                continue;

            // Read /proc/<pid>/stat to get ppid
            std::string statPath = "/proc/" + std::string(entry->d_name) + "/stat";
            std::ifstream statFile(statPath);

            if (!statFile.is_open())
                continue;

            std::string line;

            if (!std::getline(statFile, line))
                continue;

            // Format: pid (comm) state ppid ...
            // Find the closing ) to skip command name (may contain spaces)
            size_t pos = line.rfind(')');

            if (pos == std::string::npos || pos + 2 >= line.size())
                continue;

            // Parse ppid (4th field, after "pid (comm) state")
            std::istringstream iss(line.substr(pos + 2));
            char state;
            pid_t ppid;
            iss >> state >> ppid;

            if (ppid == parentPid)
                children.push_back(static_cast<pid_t>(pid));
        }

        closedir(dir);
        return children;
    }
    // -------------------------------------------------------------------------
    // Collect all PIDs in process tree (parent + all descendants)
    static void collectProcessTree(pid_t pid, std::vector<pid_t>& pids)
    {
        if (pid <= 0)
            return;

        pids.push_back(pid);

        std::vector<pid_t> children = getChildPids(pid);

        for (pid_t child : children)
            collectProcessTree(child, pids);
    }
    // -------------------------------------------------------------------------
    // Send signal to all processes in tree
    static void signalProcessTree(pid_t pid, int sig)
    {
        std::vector<pid_t> pids;
        collectProcessTree(pid, pids);

        // Send signal to all (children first, then parent)
        for (auto it = pids.rbegin(); it != pids.rend(); ++it)
            ::kill(*it, sig);
    }
    // -------------------------------------------------------------------------
    // Check if any process in tree is still alive
    static bool isTreeAlive(pid_t pid)
    {
        std::vector<pid_t> pids;
        collectProcessTree(pid, pids);

        for (pid_t p : pids)
        {
            if (::kill(p, 0) == 0)  // Process exists
                return true;
        }

        return false;
    }
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
    void ProcessManager::setStopTimeout(size_t msec)
    {
        stopTimeout_msec_ = msec;
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

                // Check manual flag (start only via REST API)
                if (pit->second.manual)
                {
                    mylog->info() << "Skipping " << procName << " (manual=1, start via REST API)" << std::endl;
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

        // Preserve Restarting state during restartAll/reloadAll for UI feedback
        if (proc.state != ProcessState::Restarting)
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

        // Timeout - kill the process and its children
        mylog->warn() << proc.name << " oneshot timeout, killing" << std::endl;

        try
        {
            signalProcessTree(proc.pid, SIGKILL);
            // Wait for process to be reaped
            waitpid(proc.pid, nullptr, 0);
        }
        catch (...) {}

        return -1;  // Timeout
    }
    // -------------------------------------------------------------------------
    void ProcessManager::stopAll()
    {
        mylog->info() << "Stopping all processes..." << std::endl;

        // Collect processes to stop (under mutex)
        std::vector<std::pair<std::string, pid_t>> toStop;
        std::vector<std::string> groupOrder;

        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopping_ = true;

            try
            {
                groupOrder = depResolver_.resolveReverse();
            }
            catch (...)
            {
                // If dependency resolution fails, collect all processes
                for (auto& kv : processes_)
                {
                    if (kv.second.pid > 0)
                        toStop.push_back({kv.first, kv.second.pid});
                }

                groupOrder.clear();
            }

            // Collect processes in group order
            if (!groupOrder.empty())
            {
                for (const auto& groupName : groupOrder)
                {
                    auto git = groups_.find(groupName);

                    if (git == groups_.end())
                        continue;

                    mylog->info() << "Stopping group: " << groupName << std::endl;

                    for (const auto& procName : git->second.processes)
                    {
                        auto pit = processes_.find(procName);

                        if (pit != processes_.end() && pit->second.pid > 0)
                            toStop.push_back({procName, pit->second.pid});
                    }
                }
            }
        }

        // Stop processes without holding mutex (allows status queries)
        for (const auto& item : toStop)
        {
            const std::string& procName = item.first;
            pid_t pid = item.second;

            if (pid > 0 && HealthChecker::isProcessAlive(pid))
            {
                mylog->info() << "Stopping process: " << procName << " (PID " << pid << ")" << std::endl;

                try
                {
                    signalProcessTree(pid, SIGTERM);

                    const size_t checkInterval = 100;
                    const size_t iterations = stopTimeout_msec_ / checkInterval;

                    for (size_t i = 0; i < iterations; i++)
                    {
                        if (!isTreeAlive(pid))
                            break;

                        std::this_thread::sleep_for(std::chrono::milliseconds(checkInterval));
                    }

                    if (isTreeAlive(pid))
                    {
                        mylog->warn() << procName << " did not stop gracefully, sending SIGKILL" << std::endl;
                        signalProcessTree(pid, SIGKILL);
                    }
                }
                catch (const std::exception& e)
                {
                    mylog->warn() << "Error stopping " << procName << ": " << e.what() << std::endl;
                }
            }

            // Update state under mutex
            {
                std::lock_guard<std::mutex> lock(mutex_);
                auto pit = processes_.find(procName);

                if (pit != processes_.end())
                {
                    // Keep Restarting state for UI feedback during reloadAll
                    if (pit->second.state != ProcessState::Restarting)
                        pit->second.state = ProcessState::Stopped;

                    pit->second.pid = 0;

                    if (onStopped_)
                        onStopped_(pit->second);
                }
            }
        }

        mylog->info() << "All processes stopped" << std::endl;
    }
    // -------------------------------------------------------------------------
    void ProcessManager::restartAll()
    {
        mylog->info() << "Restarting all processes (respecting dependencies)..." << std::endl;

        // Collect names of processes to restart (those that are running or should be running)
        std::set<std::string> toRestart;
        {
            std::lock_guard<std::mutex> lock(mutex_);

            for (const auto& kv : processes_)
            {
                const auto& proc = kv.second;

                // Skip processes that shouldn't be running
                if (proc.skip)
                    continue;

                if (proc.manual && proc.state == ProcessState::Stopped)
                    continue;  // Manual process not started yet

                if (!proc.shouldRunOnNode(nodeName_))
                    continue;

                // Restart running or failed processes
                if (proc.state == ProcessState::Running ||
                        proc.state == ProcessState::Failed ||
                        proc.state == ProcessState::Restarting)
                {
                    toRestart.insert(proc.name);
                }
            }
        }

        if (toRestart.empty())
        {
            mylog->info() << "No processes to restart" << std::endl;
            return;
        }

        // Set all processes to Restarting state for UI feedback (blinking)
        {
            std::lock_guard<std::mutex> lock(mutex_);

            for (const auto& name : toRestart)
            {
                auto it = processes_.find(name);

                if (it != processes_.end())
                    it->second.state = ProcessState::Restarting;
            }
        }

        // Get group order for proper sequencing
        std::vector<std::string> groupOrder;

        try
        {
            groupOrder = depResolver_.resolve();
        }
        catch (...)
        {
            // If dependency resolution fails, just restart all without order
            mylog->warn() << "Dependency resolution failed, restarting without order" << std::endl;

            for (const auto& name : toRestart)
            {
                mylog->info() << "Restarting: " << name << std::endl;
                restartProcess(name);
            }

            return;
        }

        // Phase 1: Stop all processes in REVERSE order
        mylog->info() << "Phase 1: Stopping processes in reverse order..." << std::endl;
        std::vector<std::string> reverseOrder(groupOrder.rbegin(), groupOrder.rend());

        // Build ordered list of processes to stop (collect under mutex, execute without)
        std::vector<std::string> stopOrder;
        {
            std::lock_guard<std::mutex> lock(mutex_);

            for (const auto& groupName : reverseOrder)
            {
                auto git = groups_.find(groupName);

                if (git == groups_.end())
                    continue;

                for (const auto& procName : git->second.processes)
                {
                    if (toRestart.count(procName) > 0)
                        stopOrder.push_back(procName);
                }
            }
        }

        // Stop processes one by one
        // Note: we release mutex during the actual stop/wait to allow status queries
        for (const auto& procName : stopOrder)
        {
            pid_t pidToStop = 0;
            bool shouldStop = false;

            // Phase 1: under mutex - get PID and set state
            {
                std::lock_guard<std::mutex> lock(mutex_);
                auto pit = processes_.find(procName);

                if (pit != processes_.end())
                {
                    auto& proc = pit->second;

                    if (proc.state != ProcessState::Stopped &&
                            proc.state != ProcessState::Completed)
                    {
                        mylog->info() << "Stopping: " << procName << std::endl;
                        pidToStop = proc.pid;
                        shouldStop = true;
                        // State should already be Restarting from earlier
                    }
                }
            }

            // Phase 2: without mutex - perform the actual stop/wait
            if (shouldStop && pidToStop > 0)
            {
                try
                {
                    if (HealthChecker::isProcessAlive(pidToStop))
                    {
                        signalProcessTree(pidToStop, SIGTERM);

                        const size_t checkInterval = 100;
                        const size_t iterations = stopTimeout_msec_ / checkInterval;

                        for (size_t i = 0; i < iterations; i++)
                        {
                            if (!isTreeAlive(pidToStop))
                                break;

                            std::this_thread::sleep_for(std::chrono::milliseconds(checkInterval));
                        }

                        if (isTreeAlive(pidToStop))
                        {
                            mylog->warn() << procName << " did not stop gracefully, sending SIGKILL" << std::endl;
                            signalProcessTree(pidToStop, SIGKILL);
                        }
                    }
                }
                catch (const std::exception& e)
                {
                    mylog->warn() << "Error stopping " << procName << ": " << e.what() << std::endl;
                }
            }

            // Phase 3: under mutex - update state
            if (shouldStop)
            {
                std::lock_guard<std::mutex> lock(mutex_);
                auto pit = processes_.find(procName);

                if (pit != processes_.end())
                {
                    pit->second.pid = 0;
                    // Keep Restarting state for UI feedback

                    if (onStopped_)
                        onStopped_(pit->second);
                }
            }
        }

        // Phase 2: Start all processes in FORWARD order
        mylog->info() << "Phase 2: Starting processes in forward order..." << std::endl;

        // Build ordered list of processes to start
        std::vector<std::string> startOrder;
        {
            std::lock_guard<std::mutex> lock(mutex_);

            for (const auto& groupName : groupOrder)
            {
                auto git = groups_.find(groupName);

                if (git == groups_.end())
                    continue;

                for (const auto& procName : git->second.processes)
                {
                    if (toRestart.count(procName) > 0)
                        startOrder.push_back(procName);
                }
            }
        }

        // Start processes one by one
        for (const auto& procName : startOrder)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto pit = processes_.find(procName);

            if (pit != processes_.end())
            {
                mylog->info() << "Starting: " << procName << std::endl;

                try
                {
                    pit->second.reset();  // Reset state before starting
                    startProcess(pit->second);
                }
                catch (const std::exception& e)
                {
                    mylog->crit() << "Error starting " << procName << ": " << e.what() << std::endl;
                }
            }
        }

        mylog->info() << "Restart all completed" << std::endl;
    }
    // -------------------------------------------------------------------------
    void ProcessManager::reloadAll()
    {
        mylog->info() << "Reloading all processes (stop all, then start all)..." << std::endl;

        // Set all running processes to Restarting state for UI feedback (blinking)
        {
            std::lock_guard<std::mutex> lock(mutex_);

            for (auto& kv : processes_)
            {
                auto& proc = kv.second;

                if (proc.state == ProcessState::Running ||
                        proc.state == ProcessState::Failed)
                {
                    proc.state = ProcessState::Restarting;
                }
            }
        }

        // Stop all running processes
        stopAll();

        // Reset all processes before starting
        {
            std::lock_guard<std::mutex> lock(mutex_);

            for (auto& kv : processes_)
                kv.second.reset();
        }

        // Start all processes (except skip and manual)
        startAll();

        mylog->info() << "Reload all completed" << std::endl;
    }
    // -------------------------------------------------------------------------
    void ProcessManager::stopProcess(ProcessInfo& proc)
    {
        if (proc.state == ProcessState::Stopped ||
                proc.state == ProcessState::Completed)
            return;

        mylog->info() << "Stopping process: " << proc.name << " (PID " << proc.pid << ")" << std::endl;

        // Preserve Restarting state during restartAll/reloadAll for UI feedback
        if (proc.state != ProcessState::Restarting)
            proc.state = ProcessState::Stopping;

        if (proc.pid > 0 && HealthChecker::isProcessAlive(proc.pid))
        {
            try
            {
                // Send SIGTERM to process and all its children
                signalProcessTree(proc.pid, SIGTERM);

                // Wait for graceful shutdown
                const size_t checkInterval = 100;  // ms
                const size_t iterations = stopTimeout_msec_ / checkInterval;

                for (size_t i = 0; i < iterations; i++)
                {
                    if (!isTreeAlive(proc.pid))
                        break;

                    std::this_thread::sleep_for(std::chrono::milliseconds(checkInterval));
                }

                // Force kill if still running
                if (isTreeAlive(proc.pid))
                {
                    mylog->warn() << proc.name << " did not stop gracefully after "
                                  << stopTimeout_msec_ << "ms, sending SIGKILL" << std::endl;
                    signalProcessTree(proc.pid, SIGKILL);
                }
            }
            catch (const std::exception& e)
            {
                mylog->warn() << "Error stopping " << proc.name << ": " << e.what() << std::endl;
            }
        }

        // Keep Restarting state for UI feedback during restartAll/reloadAll
        if (proc.state != ProcessState::Restarting)
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

            // Manual processes are optional - only check if they were started
            if (kv.second.manual && kv.second.state == ProcessState::Stopped)
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
    std::vector<std::string> ProcessManager::getFullArgs(const std::string& name) const
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = processes_.find(name);

        if (it == processes_.end())
            return {};

        const auto& proc = it->second;
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

        // Add forwarded args
        for (const auto& arg : forwardArgs_)
            args.push_back(arg);

        // Add passthrough args
        if (!passthroughArgs_.empty())
            args.push_back(passthroughArgs_);

        return args;
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

                if (proc.manual)
                    out << " (manual)";

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

                if (proc.manual)
                    out << "      manual: yes (start via REST API)" << std::endl;

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
