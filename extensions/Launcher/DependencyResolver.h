/*
 * Copyright (c) 2026 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 */
// -------------------------------------------------------------------------
#ifndef DependencyResolver_H_
#define DependencyResolver_H_
// -------------------------------------------------------------------------
#include <string>
#include <vector>
#include <set>
#include <map>
#include <stdexcept>
// -------------------------------------------------------------------------
namespace uniset
{
    class CyclicDependencyException : public std::runtime_error
    {
        public:
            explicit CyclicDependencyException(const std::string& msg)
                : std::runtime_error(msg) {}
    };

    class UnknownDependencyException : public std::runtime_error
    {
        public:
            explicit UnknownDependencyException(const std::string& msg)
                : std::runtime_error(msg) {}
    };

    /*!
     * Topological sort for process group dependencies.
     * Ensures groups are started in correct order based on their dependencies.
     */
    class DependencyResolver
    {
        public:
            DependencyResolver() = default;

            /*! Add a group with its dependencies */
            void addGroup(const std::string& name, const std::set<std::string>& depends = {});

            /*! Add dependency: 'group' depends on 'dependsOn' */
            void addDependency(const std::string& group, const std::string& dependsOn);

            /*! Check if group exists */
            bool hasGroup(const std::string& name) const;

            /*! Clear all groups */
            void clear();

            /*!
             * Perform topological sort.
             * Returns groups in order they should be started.
             * Throws CyclicDependencyException if cycle detected.
             * Throws UnknownDependencyException if dependency refers to unknown group.
             */
            std::vector<std::string> resolve() const;

            /*!
             * Reverse order for shutdown.
             * Returns groups in order they should be stopped.
             */
            std::vector<std::string> resolveReverse() const;

            /*! Get dependencies for a group */
            std::set<std::string> getDependencies(const std::string& name) const;

        private:
            struct Node
            {
                std::string name;
                std::set<std::string> depends;
            };

            enum class VisitState { White, Gray, Black };

            void dfs(const std::string& name,
                     std::map<std::string, VisitState>& visited,
                     std::vector<std::string>& result) const;

            std::map<std::string, Node> nodes_;
    };

} // end of namespace uniset
// -------------------------------------------------------------------------
#endif // DependencyResolver_H_
// -------------------------------------------------------------------------
