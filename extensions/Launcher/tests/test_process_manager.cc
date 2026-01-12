/*
 * Copyright (c) 2026 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 */
// -------------------------------------------------------------------------
#include <catch.hpp>
#include <thread>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <map>
#include "ProcessInfo.h"
#include "ProcessManager.h"
// -------------------------------------------------------------------------
using namespace uniset;
// -------------------------------------------------------------------------
// ProcessInfo tests
// -------------------------------------------------------------------------
TEST_CASE("ProcessInfo: to_string ProcessState", "[process]")
{
    REQUIRE(to_string(ProcessState::Stopped) == "stopped");
    REQUIRE(to_string(ProcessState::Starting) == "starting");
    REQUIRE(to_string(ProcessState::Running) == "running");
    REQUIRE(to_string(ProcessState::Failed) == "failed");
    REQUIRE(to_string(ProcessState::Stopping) == "stopping");
    REQUIRE(to_string(ProcessState::Restarting) == "restarting");
    REQUIRE(to_string(ProcessState::Completed) == "completed");
}
// -------------------------------------------------------------------------
TEST_CASE("ProcessInfo: to_string ReadyCheckType", "[process]")
{
    REQUIRE(to_string(ReadyCheckType::None) == "none");
    REQUIRE(to_string(ReadyCheckType::TCP) == "tcp");
    REQUIRE(to_string(ReadyCheckType::CORBA) == "corba");
    REQUIRE(to_string(ReadyCheckType::HTTP) == "http");
    REQUIRE(to_string(ReadyCheckType::File) == "file");
}
// -------------------------------------------------------------------------
TEST_CASE("ProcessInfo: readyCheckTypeFromString", "[process]")
{
    REQUIRE(readyCheckTypeFromString("tcp") == ReadyCheckType::TCP);
    REQUIRE(readyCheckTypeFromString("corba") == ReadyCheckType::CORBA);
    REQUIRE(readyCheckTypeFromString("http") == ReadyCheckType::HTTP);
    REQUIRE(readyCheckTypeFromString("file") == ReadyCheckType::File);
    REQUIRE(readyCheckTypeFromString("unknown") == ReadyCheckType::None);
    REQUIRE(readyCheckTypeFromString("") == ReadyCheckType::None);
}
// -------------------------------------------------------------------------
TEST_CASE("ProcessInfo: shouldRunOnNode empty filter", "[process]")
{
    ProcessInfo proc;
    proc.name = "test";
    // Empty filter means run on all nodes
    REQUIRE(proc.shouldRunOnNode("Node1") == true);
    REQUIRE(proc.shouldRunOnNode("Node2") == true);
    REQUIRE(proc.shouldRunOnNode("AnyNode") == true);
}
// -------------------------------------------------------------------------
TEST_CASE("ProcessInfo: shouldRunOnNode with filter", "[process]")
{
    ProcessInfo proc;
    proc.name = "test";
    proc.nodeFilter = {"Node1", "Node2"};

    REQUIRE(proc.shouldRunOnNode("Node1") == true);
    REQUIRE(proc.shouldRunOnNode("Node2") == true);
    REQUIRE(proc.shouldRunOnNode("Node3") == false);
    REQUIRE(proc.shouldRunOnNode("OtherNode") == false);
}
// -------------------------------------------------------------------------
TEST_CASE("ProcessInfo: reset", "[process]")
{
    ProcessInfo proc;
    proc.name = "test";
    proc.state = ProcessState::Running;
    proc.pid = 12345;
    proc.restartCount = 3;
    proc.lastExitCode = 1;
    proc.lastError = "some error";

    proc.reset();

    REQUIRE(proc.state == ProcessState::Stopped);
    REQUIRE(proc.pid == 0);
    REQUIRE(proc.restartCount == 0);
    REQUIRE(proc.lastExitCode == 0);
    REQUIRE(proc.lastError.empty());
    // name should not be reset
    REQUIRE(proc.name == "test");
}
// -------------------------------------------------------------------------
TEST_CASE("ReadyCheck: empty", "[process]")
{
    ReadyCheck check;
    REQUIRE(check.empty() == true);

    check.type = ReadyCheckType::TCP;
    REQUIRE(check.empty() == false);
}
// -------------------------------------------------------------------------
// ProcessManager tests
// -------------------------------------------------------------------------
TEST_CASE("ProcessManager: setNodeName", "[manager]")
{
    ProcessManager pm;
    pm.setNodeName("TestNode");
    REQUIRE(pm.getNodeName() == "TestNode");
}
// -------------------------------------------------------------------------
TEST_CASE("ProcessManager: addProcess and getProcessInfo", "[manager]")
{
    ProcessManager pm;
    pm.setNodeName("Node1");

    ProcessInfo proc;
    proc.name = "TestProcess";
    proc.command = "/bin/echo";
    proc.args = {"hello"};

    pm.addProcess(proc);

    auto info = pm.getProcessInfo("TestProcess");
    REQUIRE(info.name == "TestProcess");
    REQUIRE(info.command == "/bin/echo");
    REQUIRE(info.state == ProcessState::Stopped);

    // Unknown process returns empty
    auto unknown = pm.getProcessInfo("Unknown");
    REQUIRE(unknown.name.empty());
}
// -------------------------------------------------------------------------
TEST_CASE("ProcessManager: addGroup", "[manager]")
{
    ProcessManager pm;

    ProcessGroup group;
    group.name = "testgroup";
    group.order = 1;
    group.depends = {"othergroup"};
    group.processes = {"proc1", "proc2"};

    pm.addGroup(group);

    auto groups = pm.getAllGroups();
    REQUIRE(groups.size() == 1);
    REQUIRE(groups[0].name == "testgroup");
    REQUIRE(groups[0].processes.size() == 2);
}
// -------------------------------------------------------------------------
TEST_CASE("ProcessManager: getAllProcesses", "[manager]")
{
    ProcessManager pm;

    ProcessInfo proc1;
    proc1.name = "Process1";
    proc1.command = "/bin/true";

    ProcessInfo proc2;
    proc2.name = "Process2";
    proc2.command = "/bin/false";

    pm.addProcess(proc1);
    pm.addProcess(proc2);

    auto all = pm.getAllProcesses();
    REQUIRE(all.size() == 2);
}
// -------------------------------------------------------------------------
TEST_CASE("ProcessManager: getProcessState", "[manager]")
{
    ProcessManager pm;

    ProcessInfo proc;
    proc.name = "TestProcess";
    proc.command = "/bin/true";
    pm.addProcess(proc);

    REQUIRE(pm.getProcessState("TestProcess") == ProcessState::Stopped);
    REQUIRE(pm.getProcessState("Unknown") == ProcessState::Stopped);
}
// -------------------------------------------------------------------------
TEST_CASE("ProcessManager: start simple process", "[.integration]")
{
    ProcessManager pm;
    pm.setNodeName("Node1");

    // Add a simple group with a process that runs for a bit
    ProcessGroup group;
    group.name = "test";
    group.order = 0;
    group.processes = {"sleep_test"};
    pm.addGroup(group);

    ProcessInfo proc;
    proc.name = "sleep_test";
    proc.command = "/bin/sleep";
    proc.args = {"2"};  // Sleep for 2 seconds
    proc.group = "test";
    pm.addProcess(proc);

    // Start all
    bool result = pm.startAll();
    REQUIRE(result == true);

    // Give process time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Should be running
    auto state = pm.getProcessState("sleep_test");
    REQUIRE(state == ProcessState::Running);

    // Stop all (cleanup)
    pm.stopAll();
}
// -------------------------------------------------------------------------
TEST_CASE("ProcessManager: node filter skips processes", "[manager]")
{
    ProcessManager pm;
    pm.setNodeName("Node1");

    ProcessGroup group;
    group.name = "test";
    group.order = 0;
    group.processes = {"filtered_proc"};
    pm.addGroup(group);

    ProcessInfo proc;
    proc.name = "filtered_proc";
    proc.command = "/bin/sleep";
    proc.args = {"10"};
    proc.nodeFilter = {"Node2", "Node3"};  // Not Node1
    proc.group = "test";
    pm.addProcess(proc);

    bool started = false;
    pm.setOnProcessStarted([&started](const ProcessInfo&)
    {
        started = true;
    });

    // Start should succeed (nothing to start for this node)
    REQUIRE(pm.startAll() == true);

    // Process should NOT have been started
    REQUIRE(started == false);

    pm.stopAll();
}
// -------------------------------------------------------------------------
TEST_CASE("ProcessManager: callbacks", "[.integration]")
{
    ProcessManager pm;
    pm.setNodeName("Node1");

    ProcessGroup group;
    group.name = "test";
    group.order = 0;
    group.processes = {"callback_test"};
    pm.addGroup(group);

    ProcessInfo proc;
    proc.name = "callback_test";
    proc.command = "/bin/sleep";
    proc.args = {"2"};  // Sleep long enough
    proc.group = "test";
    pm.addProcess(proc);

    std::string startedProc;
    std::string stoppedProc;

    pm.setOnProcessStarted([&startedProc](const ProcessInfo & p)
    {
        startedProc = p.name;
    });

    pm.setOnProcessStopped([&stoppedProc](const ProcessInfo & p)
    {
        stoppedProc = p.name;
    });

    pm.startAll();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE(startedProc == "callback_test");

    pm.stopAll();
    REQUIRE(stoppedProc == "callback_test");
}
// -------------------------------------------------------------------------
TEST_CASE("ProcessManager: allRunning and anyCriticalFailed", "[.integration]")
{
    ProcessManager pm;
    pm.setNodeName("Node1");

    // Initially all running (no processes)
    REQUIRE(pm.allRunning() == true);
    REQUIRE(pm.anyCriticalFailed() == false);

    ProcessGroup group;
    group.name = "test";
    group.order = 0;
    group.processes = {"proc1"};
    pm.addGroup(group);

    ProcessInfo proc;
    proc.name = "proc1";
    proc.command = "/bin/sleep";
    proc.args = {"60"};
    proc.group = "test";
    pm.addProcess(proc);

    // Before start, not all running
    REQUIRE(pm.allRunning() == false);

    pm.startAll();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Now should be running
    REQUIRE(pm.allRunning() == true);

    pm.stopAll();
}
// -------------------------------------------------------------------------
TEST_CASE("ProcessManager: node filter allows matching processes", "[manager][nodefilter]")
{
    ProcessManager pm;
    pm.setNodeName("Node1");

    ProcessGroup group;
    group.name = "test";
    group.order = 0;
    group.processes = {"matching_proc"};
    pm.addGroup(group);

    ProcessInfo proc;
    proc.name = "matching_proc";
    proc.command = "/bin/sleep";
    proc.args = {"10"};
    proc.nodeFilter = {"Node1", "Node2"};  // Includes Node1
    proc.group = "test";
    pm.addProcess(proc);

    bool started = false;
    pm.setOnProcessStarted([&started](const ProcessInfo&)
    {
        started = true;
    });

    REQUIRE(pm.startAll() == true);

    // Give process time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Process SHOULD have been started (Node1 is in filter)
    REQUIRE(started == true);
    REQUIRE(pm.getProcessState("matching_proc") == ProcessState::Running);

    pm.stopAll();
}
// -------------------------------------------------------------------------
TEST_CASE("ProcessManager: empty node filter runs on all nodes", "[manager][nodefilter]")
{
    ProcessManager pm;
    pm.setNodeName("AnyNode");

    ProcessGroup group;
    group.name = "test";
    group.order = 0;
    group.processes = {"universal_proc"};
    pm.addGroup(group);

    ProcessInfo proc;
    proc.name = "universal_proc";
    proc.command = "/bin/sleep";
    proc.args = {"10"};
    // nodeFilter is empty - should run on all nodes
    proc.group = "test";
    pm.addProcess(proc);

    bool started = false;
    pm.setOnProcessStarted([&started](const ProcessInfo&)
    {
        started = true;
    });

    REQUIRE(pm.startAll() == true);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Process SHOULD have been started (empty filter = all nodes)
    REQUIRE(started == true);

    pm.stopAll();
}
// -------------------------------------------------------------------------
TEST_CASE("ProcessManager: skip attribute prevents start", "[manager][skip]")
{
    ProcessManager pm;
    pm.setNodeName("Node1");

    ProcessGroup group;
    group.name = "test";
    group.order = 0;
    group.processes = {"skipped_proc"};
    pm.addGroup(group);

    ProcessInfo proc;
    proc.name = "skipped_proc";
    proc.command = "/bin/sleep";
    proc.args = {"10"};
    proc.skip = true;  // Skip this process
    proc.group = "test";
    pm.addProcess(proc);

    bool started = false;
    pm.setOnProcessStarted([&started](const ProcessInfo&)
    {
        started = true;
    });

    REQUIRE(pm.startAll() == true);

    // Process should NOT have been started (skip=true)
    REQUIRE(started == false);

    pm.stopAll();
}
// -------------------------------------------------------------------------
TEST_CASE("ProcessManager: skip overrides matching nodeFilter", "[manager][skip][nodefilter]")
{
    ProcessManager pm;
    pm.setNodeName("Node1");

    ProcessGroup group;
    group.name = "test";
    group.order = 0;
    group.processes = {"skip_filter_proc"};
    pm.addGroup(group);

    ProcessInfo proc;
    proc.name = "skip_filter_proc";
    proc.command = "/bin/sleep";
    proc.args = {"10"};
    proc.nodeFilter = {"Node1"};  // Matches current node
    proc.skip = true;              // But skip is set
    proc.group = "test";
    pm.addProcess(proc);

    bool started = false;
    pm.setOnProcessStarted([&started](const ProcessInfo&)
    {
        started = true;
    });

    REQUIRE(pm.startAll() == true);

    // Process should NOT have been started (skip takes precedence)
    REQUIRE(started == false);

    pm.stopAll();
}
// -------------------------------------------------------------------------
// Tests for public stopProcess(name) and startProcess(name) methods
// -------------------------------------------------------------------------
TEST_CASE("ProcessManager: stopProcess by name - not found", "[manager]")
{
    ProcessManager pm;
    pm.setNodeName("Node1");

    // Stopping unknown process should return false
    REQUIRE(pm.stopProcess("unknown_process") == false);
}
// -------------------------------------------------------------------------
TEST_CASE("ProcessManager: startProcess by name - not found", "[manager]")
{
    ProcessManager pm;
    pm.setNodeName("Node1");

    // Starting unknown process should return false
    REQUIRE(pm.startProcess("unknown_process") == false);
}
// -------------------------------------------------------------------------
TEST_CASE("ProcessManager: startProcess by name - skipped process", "[manager]")
{
    ProcessManager pm;
    pm.setNodeName("Node1");

    ProcessGroup group;
    group.name = "test";
    group.order = 0;
    group.processes = {"skipped_proc"};
    pm.addGroup(group);

    ProcessInfo proc;
    proc.name = "skipped_proc";
    proc.command = "/bin/sleep";
    proc.args = {"10"};
    proc.group = "test";
    proc.skip = true;
    pm.addProcess(proc);

    // Starting skipped process should return false
    REQUIRE(pm.startProcess("skipped_proc") == false);
}
// -------------------------------------------------------------------------
TEST_CASE("ProcessManager: startProcess by name - wrong node", "[manager][nodefilter]")
{
    ProcessManager pm;
    pm.setNodeName("Node1");

    ProcessGroup group;
    group.name = "test";
    group.order = 0;
    group.processes = {"node2_proc"};
    pm.addGroup(group);

    ProcessInfo proc;
    proc.name = "node2_proc";
    proc.command = "/bin/sleep";
    proc.args = {"10"};
    proc.group = "test";
    proc.nodeFilter = {"Node2"};  // Not Node1
    pm.addProcess(proc);

    // Starting process for wrong node should return false
    REQUIRE(pm.startProcess("node2_proc") == false);
}
// -------------------------------------------------------------------------
TEST_CASE("ProcessManager: manual process not started by startAll", "[manager][manual]")
{
    ProcessManager pm;
    pm.setNodeName("Node1");
    pm.setStopTimeout(1000);

    ProcessGroup group;
    group.name = "test";
    group.order = 0;
    group.processes = {"manual_proc"};
    pm.addGroup(group);

    ProcessInfo proc;
    proc.name = "manual_proc";
    proc.command = "/bin/sleep";
    proc.args = {"10"};
    proc.group = "test";
    proc.manual = true;  // Manual start only
    pm.addProcess(proc);

    // startAll should succeed
    REQUIRE(pm.startAll() == true);

    // Process should NOT have been started (manual=true)
    auto info = pm.getProcessInfo("manual_proc");
    REQUIRE(info.state == ProcessState::Stopped);
    REQUIRE(info.pid == 0);

    // allRunning should still return true (manual process is ignored when stopped)
    REQUIRE(pm.allRunning() == true);

    pm.stopAll();
}
// -------------------------------------------------------------------------
TEST_CASE("ProcessManager: manual process can be started via API", "[.integration][manual]")
{
    ProcessManager pm;
    pm.setNodeName("Node1");
    pm.setStopTimeout(1000);

    ProcessGroup group;
    group.name = "test";
    group.order = 0;
    group.processes = {"manual_proc"};
    pm.addGroup(group);

    ProcessInfo proc;
    proc.name = "manual_proc";
    proc.command = "/bin/sleep";
    proc.args = {"60"};
    proc.group = "test";
    proc.manual = true;  // Manual start only
    pm.addProcess(proc);

    // startAll should succeed but not start manual process
    REQUIRE(pm.startAll() == true);

    auto info1 = pm.getProcessInfo("manual_proc");
    REQUIRE(info1.state == ProcessState::Stopped);

    // Manual start via API should work
    REQUIRE(pm.startProcess("manual_proc") == true);

    // Give some time for process to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto info2 = pm.getProcessInfo("manual_proc");
    REQUIRE(info2.state == ProcessState::Running);
    REQUIRE(info2.pid > 0);

    // Now allRunning should check the running manual process
    REQUIRE(pm.allRunning() == true);

    pm.stopAll();
}
// -------------------------------------------------------------------------
TEST_CASE("ProcessManager: stopProcess and startProcess by name", "[.integration]")
{
    ProcessManager pm;
    pm.setNodeName("Node1");

    ProcessGroup group;
    group.name = "test";
    group.order = 0;
    group.processes = {"lifecycle_proc"};
    pm.addGroup(group);

    ProcessInfo proc;
    proc.name = "lifecycle_proc";
    proc.command = "/bin/sleep";
    proc.args = {"60"};
    proc.group = "test";
    pm.addProcess(proc);

    // Start by name
    REQUIRE(pm.startProcess("lifecycle_proc") == true);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE(pm.getProcessState("lifecycle_proc") == ProcessState::Running);

    // Stop by name
    REQUIRE(pm.stopProcess("lifecycle_proc") == true);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE(pm.getProcessState("lifecycle_proc") == ProcessState::Stopped);

    // Start again
    REQUIRE(pm.startProcess("lifecycle_proc") == true);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE(pm.getProcessState("lifecycle_proc") == ProcessState::Running);

    pm.stopAll();
}
// -------------------------------------------------------------------------
// State during startup attempts tests
// -------------------------------------------------------------------------
TEST_CASE("ProcessManager: state is 'starting' during failed startup attempts", "[.integration][restart][state]")
{
    // This test verifies that process state is "starting" (not "running")
    // during multiple failed startup attempts with readyCheck timeout

    ProcessManager pm;
    pm.setNodeName("Node1");
    pm.setStopTimeout(1000);

    ProcessGroup group;
    group.name = "test";
    group.order = 0;
    group.processes = {"failing_proc"};
    pm.addGroup(group);

    ProcessInfo proc;
    proc.name = "failing_proc";
    proc.command = "/bin/sleep";
    proc.args = {"60"};  // Process that runs but readyCheck will fail
    proc.group = "test";
    proc.maxRestarts = 5;
    proc.restartDelay_msec = 100;  // Short delay for test
    proc.maxRestartDelay_msec = 200;
    proc.critical = false;  // Don't stop launcher on failure

    // Set up a TCP readyCheck that will always fail (no service on this port)
    proc.readyCheck.type = ReadyCheckType::TCP;
    proc.readyCheck.target = "localhost:59999";  // Unlikely to have a service here
    proc.readyCheck.timeout_msec = 200;  // Short timeout
    proc.readyCheck.pause_msec = 50;

    pm.addProcess(proc);

    // Track states during startup
    std::vector<ProcessState> observedStates;
    std::mutex statesMutex;

    // Start in background thread to observe states
    std::atomic<bool> stopObserving{false};
    std::thread observer([&]()
    {
        while (!stopObserving)
        {
            auto state = pm.getProcessState("failing_proc");
            {
                std::lock_guard<std::mutex> lock(statesMutex);
                observedStates.push_back(state);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });

    // Start process (will fail readyCheck multiple times)
    pm.startProcess("failing_proc");

    // Wait for a few restart attempts
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    stopObserving = true;
    observer.join();

    // Stop the process
    pm.stopAll();

    // Analyze observed states
    bool sawStarting = false;
    bool sawRunningDuringStartup = false;
    std::map<ProcessState, int> stateCounts;

    {
        std::lock_guard<std::mutex> lock(statesMutex);

        for (auto state : observedStates)
        {
            stateCounts[state]++;

            if (state == ProcessState::Starting)
                sawStarting = true;

            // If we see Running state, that's the bug - process should be Starting
            // until readyCheck passes
            if (state == ProcessState::Running)
                sawRunningDuringStartup = true;
        }
    }

    // Print observed states for debugging
    UNSCOPED_INFO("Observed states during startup attempts:");

    for (const auto& kv : stateCounts)
    {
        UNSCOPED_INFO("  " << to_string(kv.first) << ": " << kv.second << " times");
    }

    // The process should have been in Starting state during startup attempts
    REQUIRE(sawStarting == true);

    // The process should NOT have been in Running state (readyCheck always fails)
    REQUIRE(sawRunningDuringStartup == false);
}
// -------------------------------------------------------------------------
TEST_CASE("ProcessManager: restart after exhausted maxRestarts gives new attempts", "[.integration][restart][maxrestarts]")
{
    // Simulate process that fails to start (readyCheck always fails)
    // 1. Start with maxRestarts=3 → fails 3 times → Failed
    // 2. Manual restart via API
    // 3. Should get 3 new attempts → Failed again

    ProcessManager pm;
    pm.setNodeName("Node1");
    pm.setStopTimeout(500);

    ProcessGroup group;
    group.name = "test";
    group.order = 0;
    group.processes = {"failing_proc"};
    pm.addGroup(group);

    ProcessInfo proc;
    proc.name = "failing_proc";
    proc.command = "/bin/sleep";
    proc.args = {"60"};
    proc.group = "test";
    proc.maxRestarts = 3;
    proc.restartDelay_msec = 50;   // Short delays for test
    proc.maxRestartDelay_msec = 100;
    proc.critical = false;

    // TCP readyCheck that always fails
    proc.readyCheck.type = ReadyCheckType::TCP;
    proc.readyCheck.target = "localhost:59999";
    proc.readyCheck.timeout_msec = 100;
    proc.readyCheck.pause_msec = 20;

    pm.addProcess(proc);

    // Track state changes
    int startingCount1 = 0;

    // Observer thread
    std::atomic<bool> stopObserving{false};
    std::mutex countMutex;

    auto observerFunc = [&]()
    {
        ProcessState lastState = ProcessState::Stopped;

        while (!stopObserving)
        {
            auto state = pm.getProcessState("failing_proc");

            if (state != lastState)
            {
                std::lock_guard<std::mutex> lock(countMutex);

                if (state == ProcessState::Starting)
                    startingCount1++;

                lastState = state;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    };

    std::thread observer(observerFunc);

    // === Phase 1: First start attempt ===
    REQUIRE(pm.startProcess("failing_proc") == false);  // Should fail (readyCheck fails)

    // Wait for state to settle
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    stopObserving = true;
    observer.join();

    // Should have seen Starting state multiple times (3 attempts)
    {
        std::lock_guard<std::mutex> lock(countMutex);
        UNSCOPED_INFO("Phase 1 - Starting count: " << startingCount1);
        REQUIRE(startingCount1 >= 1);  // At least 1 (may miss some due to timing)
    }

    // Process should be in Failed state
    REQUIRE(pm.getProcessState("failing_proc") == ProcessState::Failed);

    // restartCount should be 3 (maxRestarts attempts made)
    auto info1 = pm.getProcessInfo("failing_proc");
    UNSCOPED_INFO("Phase 1 - restartCount: " << info1.restartCount);
    REQUIRE(info1.restartCount == 3);

    // === Phase 2: Manual restart via API ===
    int startingCount2 = 0;
    stopObserving = false;

    std::thread observer2([&]()
    {
        ProcessState lastState = ProcessState::Failed;

        while (!stopObserving)
        {
            auto state = pm.getProcessState("failing_proc");

            if (state != lastState)
            {
                std::lock_guard<std::mutex> lock(countMutex);

                if (state == ProcessState::Starting || state == ProcessState::Restarting)
                    startingCount2++;

                lastState = state;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    // Restart should return false (because readyCheck will fail)
    pm.restartProcess("failing_proc");

    // Wait for state to settle
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    stopObserving = true;
    observer2.join();

    // Should have tried to start again
    {
        std::lock_guard<std::mutex> lock(countMutex);
        UNSCOPED_INFO("Phase 2 - Starting/Restarting count: " << startingCount2);
        REQUIRE(startingCount2 >= 1);  // Should have attempted restart
    }

    // Process should be in Failed state again
    auto finalState = pm.getProcessState("failing_proc");
    REQUIRE(finalState == ProcessState::Failed);

    // restartCount should be 3 again (new round of 3 attempts after manual restart)
    auto info2 = pm.getProcessInfo("failing_proc");
    UNSCOPED_INFO("Phase 2 - restartCount: " << info2.restartCount);
    REQUIRE(info2.restartCount == 3);  // 3 new attempts

    pm.stopAll();
}
// -------------------------------------------------------------------------
// Restart configuration tests
// -------------------------------------------------------------------------
TEST_CASE("ProcessInfo: restart defaults", "[process][restart]")
{
    ProcessInfo proc;

    REQUIRE(proc.maxRestarts == 0);  // 0 = infinite, -1 = no restart
    REQUIRE(proc.restartDelay_msec == 1000);  // 1 second initial delay
    REQUIRE(proc.maxRestartDelay_msec == 30000);  // 30 seconds max
    REQUIRE(proc.critical == true);  // critical processes are restarted by default
}
// -------------------------------------------------------------------------
TEST_CASE("ProcessManager: stopProcess kills child processes", "[.integration][processtree]")
{
    ProcessManager pm;
    pm.setNodeName("Node1");
    pm.setStopTimeout(2000);  // 2 seconds

    ProcessGroup group;
    group.name = "test";
    group.order = 0;
    group.processes = {"parent_proc"};
    pm.addGroup(group);

    // Create a process that spawns children
    // bash -c spawns a subshell, which runs sleep in background and foreground
    ProcessInfo proc;
    proc.name = "parent_proc";
    proc.command = "/bin/bash";
    proc.args = {"-c", "sleep 60 & sleep 60 & wait"};
    proc.group = "test";
    pm.addProcess(proc);

    // Start
    REQUIRE(pm.startAll() == true);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));  // Let children spawn

    // Get parent PID
    auto info = pm.getProcessInfo("parent_proc");
    pid_t parentPid = info.pid;
    REQUIRE(parentPid > 0);
    REQUIRE(pm.getProcessState("parent_proc") == ProcessState::Running);

    // Find child processes (sleep commands)
    std::vector<pid_t> childPids;
    std::string cmd = "pgrep -P " + std::to_string(parentPid);
    FILE* fp = popen(cmd.c_str(), "r");

    if (fp)
    {
        char buf[64];

        while (fgets(buf, sizeof(buf), fp))
        {
            pid_t childPid = std::stoi(buf);

            if (childPid > 0)
                childPids.push_back(childPid);
        }

        pclose(fp);
    }

    // Should have child processes
    REQUIRE(childPids.size() >= 1);

    // Stop the parent - should kill all children too
    pm.stopAll();

    // Give time for signals to propagate
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Check parent is stopped
    REQUIRE(pm.getProcessState("parent_proc") == ProcessState::Stopped);

    // Check all children are dead
    for (pid_t childPid : childPids)
    {
        // kill(pid, 0) returns 0 if process exists, -1 if not
        bool childAlive = (kill(childPid, 0) == 0);
        REQUIRE(childAlive == false);
    }
}
// -------------------------------------------------------------------------
