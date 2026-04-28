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
    enum class BulkOperation : int
    {
        None = 0,
        Restart,
        Reload,
        Stop
    };

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
            void setPassthroughArgs(const std::vector<std::string>& args);
            void setForwardArgs(const std::vector<std::string>& args);

            // Process registration
            void addProcess(const ProcessInfo& proc);
            void addGroup(const ProcessGroup& group);

            // Lifecycle management
            bool startAll();
            void stopAll();
            void restartAll();  //!< Restart all running processes
            void reloadAll();   //!< Stop all, then start all (except skip, manual). Oneshot processes ARE re-run.
            void requestStop(); //!< Set stopping_ flag (async-signal-safe)

            /*!
             * Interrupt any active startup phase WITHOUT marking the process
             * for shutdown. Sets only stopping_=true (not shutdownRequested_),
             * so an in-flight restartAll()/reloadAll() bails out of its
             * startAll() phase quickly while the launcher process keeps
             * running. Suitable for HTTP-driven stop-all that wants to abort
             * an active bulk operation but does not want to terminate the
             * launcher itself. After the subsequent stopAll() completes,
             * stopping_ is restored to shutdownRequested_ (false here),
             * allowing restart-all/reload-all to be re-armed.
             */
            void cancelStartup();
            bool isBulkOperationInProgress() const;
            BulkOperation currentBulkOperation() const;
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
            void doStopAll();  //!< Internal stopAll without bulk guard
            void handleProcessExitByName(const std::string& name, int exitCode);
            void monitorLoop();

            // Helper methods for process startup
            std::vector<std::string> assembleArgs(const ProcessInfo& proc) const;
            std::vector<std::string> prepareProcessArgs(const ProcessInfo& proc);
            bool launchDaemonProcess(ProcessInfo& proc);

            //! Sleep in small chunks, checking cancelFlag. Returns true if cancelled.
            static bool interruptibleSleep(size_t msec, const std::atomic<bool>& cancelFlag,
                                           size_t pollInterval_msec = 500);

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
            std::atomic<bool> stopAllRunning_{false};
            std::atomic<bool> shutdownRequested_{false};  // monotonic: SIGINT/SIGTERM was received
            std::atomic<BulkOperation> currentBulkOp_{BulkOperation::None};
            mutable std::mutex mutex_;

            size_t healthCheckInterval_msec_ = 5000;
            size_t restartWindow_msec_ = 60000;
            size_t stopTimeout_msec_ = 5000;  // Time to wait for graceful shutdown before SIGKILL
            std::vector<std::string> commonArgs_;
            std::vector<std::string> passthroughArgs_;  // Arguments after "--" passed to all child processes
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
