/*
 * Copyright (c) 2026 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 */
// -------------------------------------------------------------------------
#include <catch.hpp>
#include "DependencyResolver.h"
// -------------------------------------------------------------------------
using namespace uniset;
// -------------------------------------------------------------------------
TEST_CASE("DependencyResolver: empty resolver", "[dependency]")
{
    DependencyResolver resolver;

    auto order = resolver.resolve();
    REQUIRE(order.empty());
}
// -------------------------------------------------------------------------
TEST_CASE("DependencyResolver: single group", "[dependency]")
{
    DependencyResolver resolver;
    resolver.addGroup("group1");

    auto order = resolver.resolve();
    REQUIRE(order.size() == 1);
    REQUIRE(order[0] == "group1");
}
// -------------------------------------------------------------------------
TEST_CASE("DependencyResolver: two independent groups", "[dependency]")
{
    DependencyResolver resolver;
    resolver.addGroup("group1");
    resolver.addGroup("group2");

    auto order = resolver.resolve();
    REQUIRE(order.size() == 2);
    // Both groups present (order not guaranteed for independent groups)
    REQUIRE((order[0] == "group1" || order[0] == "group2"));
    REQUIRE((order[1] == "group1" || order[1] == "group2"));
    REQUIRE(order[0] != order[1]);
}
// -------------------------------------------------------------------------
TEST_CASE("DependencyResolver: linear dependency chain", "[dependency]")
{
    DependencyResolver resolver;
    resolver.addGroup("naming");
    resolver.addGroup("sm", {"naming"});
    resolver.addGroup("exchanges", {"sm"});

    auto order = resolver.resolve();
    REQUIRE(order.size() == 3);

    // Find positions
    size_t namingPos = 0, smPos = 0, exchangesPos = 0;

    for (size_t i = 0; i < order.size(); i++)
    {
        if (order[i] == "naming") namingPos = i;
        else if (order[i] == "sm") smPos = i;
        else if (order[i] == "exchanges") exchangesPos = i;
    }

    // naming must come before sm
    REQUIRE(namingPos < smPos);
    // sm must come before exchanges
    REQUIRE(smPos < exchangesPos);
}
// -------------------------------------------------------------------------
TEST_CASE("DependencyResolver: multiple dependencies", "[dependency]")
{
    DependencyResolver resolver;
    resolver.addGroup("naming");
    resolver.addGroup("sm", {"naming"});
    resolver.addGroup("unet", {"sm"});
    resolver.addGroup("modbus", {"sm"});
    resolver.addGroup("opcua", {"sm"});

    auto order = resolver.resolve();
    REQUIRE(order.size() == 5);

    // Find positions
    size_t namingPos = 0, smPos = 0;

    for (size_t i = 0; i < order.size(); i++)
    {
        if (order[i] == "naming") namingPos = i;
        else if (order[i] == "sm") smPos = i;
    }

    // naming before sm
    REQUIRE(namingPos < smPos);

    // unet, modbus, opcua all after sm
    for (size_t i = 0; i < order.size(); i++)
    {
        if (order[i] == "unet" || order[i] == "modbus" || order[i] == "opcua")
            REQUIRE(i > smPos);
    }
}
// -------------------------------------------------------------------------
TEST_CASE("DependencyResolver: cyclic dependency throws", "[dependency]")
{
    DependencyResolver resolver;
    resolver.addGroup("a", {"b"});
    resolver.addGroup("b", {"a"});

    REQUIRE_THROWS_AS(resolver.resolve(), CyclicDependencyException);
}
// -------------------------------------------------------------------------
TEST_CASE("DependencyResolver: unknown dependency throws", "[dependency]")
{
    DependencyResolver resolver;
    resolver.addGroup("a", {"unknown"});

    REQUIRE_THROWS_AS(resolver.resolve(), UnknownDependencyException);
}
// -------------------------------------------------------------------------
TEST_CASE("DependencyResolver: reverse order", "[dependency]")
{
    DependencyResolver resolver;
    resolver.addGroup("naming");
    resolver.addGroup("sm", {"naming"});
    resolver.addGroup("exchanges", {"sm"});

    auto order = resolver.resolve();
    auto reverseOrder = resolver.resolveReverse();

    REQUIRE(reverseOrder.size() == order.size());

    // Reverse order should be opposite
    for (size_t i = 0; i < order.size(); i++)
        REQUIRE(reverseOrder[i] == order[order.size() - 1 - i]);
}
// -------------------------------------------------------------------------
TEST_CASE("DependencyResolver: hasGroup", "[dependency]")
{
    DependencyResolver resolver;
    resolver.addGroup("group1");

    REQUIRE(resolver.hasGroup("group1"));
    REQUIRE_FALSE(resolver.hasGroup("group2"));
}
// -------------------------------------------------------------------------
TEST_CASE("DependencyResolver: getDependencies", "[dependency]")
{
    DependencyResolver resolver;
    resolver.addGroup("naming");
    resolver.addGroup("sm", {"naming"});

    auto deps = resolver.getDependencies("sm");
    REQUIRE(deps.size() == 1);
    REQUIRE(deps.count("naming") == 1);

    auto noDeps = resolver.getDependencies("naming");
    REQUIRE(noDeps.empty());

    auto unknown = resolver.getDependencies("unknown");
    REQUIRE(unknown.empty());
}
// -------------------------------------------------------------------------
TEST_CASE("DependencyResolver: addDependency", "[dependency]")
{
    DependencyResolver resolver;
    resolver.addGroup("naming");
    resolver.addGroup("sm");
    resolver.addDependency("sm", "naming");

    auto deps = resolver.getDependencies("sm");
    REQUIRE(deps.count("naming") == 1);
}
// -------------------------------------------------------------------------
TEST_CASE("DependencyResolver: clear", "[dependency]")
{
    DependencyResolver resolver;
    resolver.addGroup("group1");
    resolver.addGroup("group2");

    resolver.clear();

    REQUIRE_FALSE(resolver.hasGroup("group1"));
    REQUIRE_FALSE(resolver.hasGroup("group2"));
    REQUIRE(resolver.resolve().empty());
}
// -------------------------------------------------------------------------
