/*
 * Copyright (c) 2026 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 */
// -------------------------------------------------------------------------
#ifndef ProcessManager_H_
#define ProcessManager_H_
// -------------------------------------------------------------------------
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <atomic>
#include <thread>
#include <functional>
#include <iostream>
#include <Poco/Process.h>
#include "ProcessInfo.h"
#include "DependencyResolver.h"
#include "HealthChecker.h"
#include "Configuration.h"
#include "Debug.h"
// -------------------------------------------------------------------------
namespace uniset
{
    /*!
     * Process Manager - manages process lifecycle.
     * Handles startup order, health monitoring, and automatic restarts.
     *
     * Process Tree Termination:
     * When stopping a process, the manager terminates the entire process tree
     * (the process and all its descendants). This ensures no orphaned child
     * processes remain after stop. The termination algorithm:
     * 1. Send SIGTERM to all processes in the tree (leaves to root)
     * 2. Wait stopTimeout_msec_ for graceful shutdown
     * 3. Send SIGKILL to any remaining processes
     */
    class ProcessManager
    {
        public:
            explicit ProcessManager(std::shared_ptr<Configuration> conf = nullptr);
            ~ProcessManager();

            // Configuration
            void setNodeName(const std::string& name);
            std::string getNodeName() const;

            void setHealthCheckInterval(size_t msec);
            void setRestartWindow(size_t msec);
            void setStopTimeout(size_t msec);
            void setCommonArgs(const std::vector<std::string>& args);
            void setPassthroughArgs(const std::string& args);
            void setForwardArgs(const std::vector<std::string>& args);

            // Process registration
            void addProcess(const ProcessInfo& proc);
            void addGroup(const ProcessGroup& group);

            // Lifecycle management
            bool startAll();
            void stopAll();
            void restartAll();  //!< Restart all running processes
            void reloadAll();   //!< Stop all, then start all (except skip, manual)
            bool restartProcess(const std::string& name);
            bool stopProcess(const std::string& name);
            bool startProcess(const std::string& name);

            // Monitoring
            void startMonitoring();
            void stopMonitoring();
            bool isMonitoring() const;

            // State queries
            ProcessState getProcessState(const std::string& name) const;
            ProcessInfo getProcessInfo(const std::string& name) const;
            std::vector<ProcessInfo> getAllProcesses() const;
            std::vector<ProcessGroup> getAllGroups() const;

            bool allRunning() const;
            bool anyCriticalFailed() const;

            //! Get full arguments list for a process (commonArgs + args + forwardArgs)
            std::vector<std::string> getFullArgs(const std::string& name) const;

            /*!
             * Print run list (dry-run mode).
             * Shows what will be launched, with what parameters, in what order.
             * \param out Output stream (e.g. std::cout)
             */
            void printRunList(std::ostream& out) const;

            // Callbacks
            using ProcessCallback = std::function<void(const ProcessInfo&)>;
            void setOnProcessStarted(ProcessCallback cb);
            void setOnProcessStopped(ProcessCallback cb);
            void setOnProcessFailed(ProcessCallback cb);

            // Debug
            std::shared_ptr<DebugStream> log();

        private:
            bool startProcessWithUnlock(ProcessInfo& proc, std::unique_lock<std::mutex>& lock);
            bool startOneshotWithUnlock(ProcessInfo& proc, std::unique_lock<std::mutex>& lock);
            void stopProcess(ProcessInfo& proc);
            void handleProcessExitByName(const std::string& name, int exitCode);
            void monitorLoop();

            // Helper methods for process startup
            std::vector<std::string> prepareProcessArgs(const ProcessInfo& proc);
            bool launchDaemonProcess(ProcessInfo& proc);

            std::vector<std::string> resolveStartOrder();
            void expandEnvironment(std::vector<std::string>& args);
            std::string expandEnvVar(const std::string& s);

            std::shared_ptr<Configuration> conf_;
            std::unique_ptr<HealthChecker> healthChecker_;
            DependencyResolver depResolver_;

            std::string nodeName_;
            std::map<std::string, ProcessInfo> processes_;
            std::map<std::string, ProcessGroup> groups_;

            std::thread monitorThread_;
            std::atomic<bool> running_{false};
            std::atomic<bool> stopping_{false};
            mutable std::mutex mutex_;

            size_t healthCheckInterval_msec_ = 5000;
            size_t restartWindow_msec_ = 60000;
            size_t stopTimeout_msec_ = 5000;  // Time to wait for graceful shutdown before SIGKILL
            std::vector<std::string> commonArgs_;
            std::string passthroughArgs_;  // Arguments after "--" passed to all child processes
            std::vector<std::string> forwardArgs_;  // Unknown arguments forwarded to child processes

            ProcessCallback onStarted_;
            ProcessCallback onStopped_;
            ProcessCallback onFailed_;

            std::shared_ptr<DebugStream> mylog;
    };

} // end of namespace uniset
// -------------------------------------------------------------------------
#endif // ProcessManager_H_
// -------------------------------------------------------------------------
