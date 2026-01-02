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
