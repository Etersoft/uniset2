/*
 * Copyright (c) 2026 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 */
// -------------------------------------------------------------------------
#include <catch.hpp>
#include "ProcessInfo.h"
// -------------------------------------------------------------------------
using namespace uniset;
// -------------------------------------------------------------------------
// Additional shouldRunOnNode tests (basic tests are in test_process_manager.cc)
// -------------------------------------------------------------------------
TEST_CASE("ProcessInfo: shouldRunOnNode with single node", "[processinfo][nodefilter]")
{
    ProcessInfo proc;
    proc.name = "TestProcess";
    proc.nodeFilter = {"Node1"};

    REQUIRE(proc.shouldRunOnNode("Node1") == true);
    REQUIRE(proc.shouldRunOnNode("Node2") == false);
}
// -------------------------------------------------------------------------
TEST_CASE("ProcessInfo: shouldRunOnNode case sensitivity", "[processinfo][nodefilter]")
{
    ProcessInfo proc;
    proc.name = "TestProcess";
    proc.nodeFilter = {"Node1"};

    // Node names are case-sensitive
    REQUIRE(proc.shouldRunOnNode("Node1") == true);
    REQUIRE(proc.shouldRunOnNode("node1") == false);
    REQUIRE(proc.shouldRunOnNode("NODE1") == false);
}
// -------------------------------------------------------------------------
TEST_CASE("ProcessInfo: shouldRunOnNode with empty node name", "[processinfo][nodefilter]")
{
    ProcessInfo proc;
    proc.name = "TestProcess";

    // Empty nodeFilter - should run on any node including empty
    REQUIRE(proc.shouldRunOnNode("") == true);

    // With filter - empty node name should not match
    proc.nodeFilter = {"Node1"};
    REQUIRE(proc.shouldRunOnNode("") == false);
}
// -------------------------------------------------------------------------
// Default values tests
// -------------------------------------------------------------------------
TEST_CASE("ProcessInfo: default values", "[processinfo]")
{
    ProcessInfo proc;

    REQUIRE(proc.name.empty());
    REQUIRE(proc.command.empty());
    REQUIRE(proc.args.empty());
    REQUIRE(proc.group.empty());
    REQUIRE(proc.nodeFilter.empty());
    REQUIRE(proc.skip == false);
    REQUIRE(proc.manual == false);
    REQUIRE(proc.critical == true);  // default is now true
    REQUIRE(proc.oneshot == false);
    REQUIRE(proc.maxRestarts == 0);  // 0 = infinite restarts, -1 = no restart
    REQUIRE(proc.maxRestartDelay_msec == 30000);  // 30 sec max backoff
    REQUIRE(proc.state == ProcessState::Stopped);
    REQUIRE(proc.pid == 0);
}
// -------------------------------------------------------------------------
// skip attribute tests
// -------------------------------------------------------------------------
TEST_CASE("ProcessInfo: skip flag independence", "[processinfo][skip]")
{
    ProcessInfo proc;
    proc.name = "SkippedProcess";
    proc.skip = true;

    // skip is independent of nodeFilter
    // shouldRunOnNode only checks nodeFilter, not skip
    REQUIRE(proc.skip == true);
    REQUIRE(proc.shouldRunOnNode("Node1") == true);  // nodeFilter is empty

    // With matching nodeFilter, shouldRunOnNode still returns true
    // Caller (ProcessManager) should check skip separately
    proc.nodeFilter = {"Node1"};
    REQUIRE(proc.shouldRunOnNode("Node1") == true);
    REQUIRE(proc.skip == true);
}
// -------------------------------------------------------------------------
// Multiple nodes in filter
// -------------------------------------------------------------------------
TEST_CASE("ProcessInfo: shouldRunOnNode with multiple nodes", "[processinfo][nodefilter]")
{
    ProcessInfo proc;
    proc.name = "MultiNodeProcess";
    proc.nodeFilter = {"Node1", "Node2", "Node3"};

    REQUIRE(proc.shouldRunOnNode("Node1") == true);
    REQUIRE(proc.shouldRunOnNode("Node2") == true);
    REQUIRE(proc.shouldRunOnNode("Node3") == true);
    REQUIRE(proc.shouldRunOnNode("Node4") == false);
    REQUIRE(proc.shouldRunOnNode("Node5") == false);
}
// -------------------------------------------------------------------------
