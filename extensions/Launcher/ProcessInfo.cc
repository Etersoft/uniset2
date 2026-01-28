/*
 * Copyright (c) 2026 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 */
// -------------------------------------------------------------------------
#include "ProcessInfo.h"
// -------------------------------------------------------------------------
namespace uniset
{
    // -------------------------------------------------------------------------
    std::string to_string(ProcessState state)
    {
        switch (state)
        {
            case ProcessState::Stopped:
                return "stopped";

            case ProcessState::Starting:
                return "starting";

            case ProcessState::Running:
                return "running";

            case ProcessState::Completed:
                return "completed";

            case ProcessState::Failed:
                return "failed";

            case ProcessState::Stopping:
                return "stopping";

            case ProcessState::Restarting:
                return "restarting";
        }

        return "unknown";
    }
    // -------------------------------------------------------------------------
    std::string to_string(ReadyCheckType type)
    {
        switch (type)
        {
            case ReadyCheckType::None:
                return "none";

            case ReadyCheckType::TCP:
                return "tcp";

            case ReadyCheckType::CORBA:
                return "corba";

            case ReadyCheckType::HTTP:
                return "http";

            case ReadyCheckType::File:
                return "file";
        }

        return "unknown";
    }
    // -------------------------------------------------------------------------
    ReadyCheckType readyCheckTypeFromString(const std::string& s)
    {
        if (s == "tcp")
            return ReadyCheckType::TCP;

        if (s == "corba")
            return ReadyCheckType::CORBA;

        if (s == "http")
            return ReadyCheckType::HTTP;

        if (s == "file")
            return ReadyCheckType::File;

        return ReadyCheckType::None;
    }
    // -------------------------------------------------------------------------
    bool ProcessInfo::shouldRunOnNode(const std::string& nodeName) const
    {
        if (nodeFilter.empty())
            return true;

        return nodeFilter.find(nodeName) != nodeFilter.end();
    }
    // -------------------------------------------------------------------------
    void ProcessInfo::reset()
    {
        state = ProcessState::Stopped;
        pid = 0;
        restartCount = 0;
        lastExitCode = 0;
        lastError.clear();
        healthFailCount = 0;
    }
    // -------------------------------------------------------------------------
} // end of namespace uniset
