/*
 * Copyright (c) 2026 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 */
// -------------------------------------------------------------------------
#ifndef ProcessInfo_H_
#define ProcessInfo_H_
// -------------------------------------------------------------------------
#include <string>
#include <vector>
#include <set>
#include <map>
#include <chrono>
#include <atomic>
#include <Poco/Process.h>
// -------------------------------------------------------------------------
namespace uniset
{
    // Process states
    enum class ProcessState
    {
        Stopped,    // Not started
        Starting,   // Starting (waiting for readyCheck)
        Running,    // Running normally
        Completed,  // Oneshot process completed successfully
        Failed,     // Crashed/exited unexpectedly
        Stopping    // Shutting down
    };

    std::string to_string(ProcessState state);

    // Ready check types
    enum class ReadyCheckType
    {
        None,   // No check (immediately ready)
        TCP,    // TCP port is available
        CORBA,  // CORBA object is registered
        HTTP,   // HTTP endpoint returns 200
        File    // File exists
    };

    std::string to_string(ReadyCheckType type);
    ReadyCheckType readyCheckTypeFromString(const std::string& s);

    // Ready check configuration
    struct ReadyCheck
    {
        ReadyCheckType type = ReadyCheckType::None;
        std::string target;         // e.g. "2809", "SharedMemory", "http://localhost:8080/health"
        size_t timeout_msec = 10000;    // Total timeout for waitForReady
        size_t pause_msec = 1000;       // Pause between checks (default 1 sec)
        size_t checkTimeout_msec = 1000; // Timeout for single check (health monitoring)

        bool empty() const
        {
            return type == ReadyCheckType::None;
        }
    };

    // Process configuration and runtime state
    struct ProcessInfo
    {
        // Configuration (from XML)
        std::string name;
        std::string type;           //!< Process type (for template lookup, e.g. "SharedMemory")
        std::string command;
        std::vector<std::string> args;      // Normal args (commonArgs + args)
        std::vector<std::string> rawArgs;   // Standalone args (no commonArgs)
        std::string workDir;
        std::map<std::string, std::string> env;  // Additional environment variables

        ReadyCheck readyCheck;

        // Command to run after process is ready (e.g. "uniset2-admin --create")
        std::string afterRun;
        bool critical = true;           // If fails, stop everything (default: true)
        bool restartOnFailure = true;
        int maxRestarts = 5;
        size_t restartDelay_msec = 3000;

        std::set<std::string> nodeFilter;  // Which nodes to run on (empty = all)
        std::string group;                  // Process group name
        bool skip = false;                  // Skip this process (don't start)
        bool oneshot = false;               // Oneshot process (exit 0 = success)
        size_t oneshotTimeout_msec = 30000; // Timeout for oneshot process

        // Runtime state
        ProcessState state = ProcessState::Stopped;
        Poco::Process::PID pid = 0;
        int restartCount = 0;
        int lastExitCode = 0;
        std::chrono::steady_clock::time_point lastStartTime;
        std::chrono::steady_clock::time_point lastFailTime;
        std::string lastError;

        // Helper methods
        bool shouldRunOnNode(const std::string& nodeName) const;
        void reset();
    };

    // Process group for ordered startup
    struct ProcessGroup
    {
        std::string name;
        int order = 0;
        std::set<std::string> depends;  // Groups this group depends on
        std::vector<std::string> processes;  // Process names in this group
    };

} // end of namespace uniset
// -------------------------------------------------------------------------
#endif // ProcessInfo_H_
// -------------------------------------------------------------------------
