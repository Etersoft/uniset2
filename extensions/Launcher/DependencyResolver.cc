/*
 * Copyright (c) 2026 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 */
// -------------------------------------------------------------------------
#include "DependencyResolver.h"
#include <algorithm>
// -------------------------------------------------------------------------
namespace uniset
{
    // -------------------------------------------------------------------------
    void DependencyResolver::addGroup(const std::string& name, const std::set<std::string>& depends)
    {
        nodes_[name] = Node{name, depends};
    }
    // -------------------------------------------------------------------------
    void DependencyResolver::addDependency(const std::string& group, const std::string& dependsOn)
    {
        if (nodes_.find(group) == nodes_.end())
            nodes_[group] = Node{group, {}};

        nodes_[group].depends.insert(dependsOn);
    }
    // -------------------------------------------------------------------------
    bool DependencyResolver::hasGroup(const std::string& name) const
    {
        return nodes_.find(name) != nodes_.end();
    }
    // -------------------------------------------------------------------------
    void DependencyResolver::clear()
    {
        nodes_.clear();
    }
    // -------------------------------------------------------------------------
    std::set<std::string> DependencyResolver::getDependencies(const std::string& name) const
    {
        auto it = nodes_.find(name);

        if (it != nodes_.end())
            return it->second.depends;

        return {};
    }
    // -------------------------------------------------------------------------
    std::vector<std::string> DependencyResolver::resolve() const
    {
        // Validate all dependencies exist
        for (const auto& kv : nodes_)
        {
            for (const auto& dep : kv.second.depends)
            {
                if (nodes_.find(dep) == nodes_.end())
                {
                    throw UnknownDependencyException(
                        "Group '" + kv.first + "' depends on unknown group '" + dep + "'");
                }
            }
        }

        std::map<std::string, VisitState> visited;

        for (const auto& kv : nodes_)
            visited[kv.first] = VisitState::White;

        std::vector<std::string> result;

        for (const auto& kv : nodes_)
        {
            if (visited[kv.first] == VisitState::White)
                dfs(kv.first, visited, result);
        }

        return result;
    }
    // -------------------------------------------------------------------------
    std::vector<std::string> DependencyResolver::resolveReverse() const
    {
        auto result = resolve();
        std::reverse(result.begin(), result.end());
        return result;
    }
    // -------------------------------------------------------------------------
    void DependencyResolver::dfs(const std::string& name,
                                 std::map<std::string, VisitState>& visited,
                                 std::vector<std::string>& result) const
    {
        visited[name] = VisitState::Gray;

        auto it = nodes_.find(name);

        if (it != nodes_.end())
        {
            for (const auto& dep : it->second.depends)
            {
                if (visited[dep] == VisitState::Gray)
                {
                    throw CyclicDependencyException(
                        "Cyclic dependency detected: " + name + " -> " + dep);
                }

                if (visited[dep] == VisitState::White)
                    dfs(dep, visited, result);
            }
        }

        visited[name] = VisitState::Black;
        result.push_back(name);
    }
    // -------------------------------------------------------------------------
} // end of namespace uniset
