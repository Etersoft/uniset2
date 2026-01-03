/*
 * Copyright (c) 2026 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 */
// -------------------------------------------------------------------------
#ifndef ConfigLoader_H_
#define ConfigLoader_H_
// -------------------------------------------------------------------------
#include <memory>
#include <string>
#include <vector>
#include <map>
#include "ProcessInfo.h"
#include "UniXML.h"
#include "Configuration.h"
// -------------------------------------------------------------------------
namespace uniset
{
    /*!
     * Configuration loader for Launcher.
     * Loads process and group definitions from XML configuration.
     *
     * Expected XML structure:
     * <Launcher name="Launcher1" healthCheckInterval="5000" restartDelay="3000">
     *   <ProcessGroups>
     *     <group name="naming" order="0">
     *       <process name="omniNames" command="omniNames" args="-start"
     *                readyCheck="tcp:2809" critical="true"/>
     *     </group>
     *     <group name="sm" order="1" depends="naming">
     *       <process name="SharedMemory" command="uniset2-smemory"
     *                args="--confile ${CONFFILE}" readyCheck="corba:SharedMemory"/>
     *     </group>
     *   </ProcessGroups>
     *   <Environment>
     *     <var name="CONFFILE" value="/app/configure.xml"/>
     *   </Environment>
     * </Launcher>
     */
    class ConfigLoader
    {
        public:
            struct LauncherConfig
            {
                std::string name;
                size_t healthCheckInterval_msec = 5000;
                size_t restartDelay_msec = 1000;
                size_t maxRestartDelay_msec = 30000;  // Max delay for exponential backoff
                size_t restartWindow_msec = 60000;
                int maxRestarts = 0;  // 0 = infinite restarts
                int httpPort = 0;  // 0 = disabled

                std::vector<std::string> commonArgs;  // Common args prepended to all processes

                std::vector<ProcessGroup> groups;
                std::map<std::string, ProcessInfo> processes;
                std::map<std::string, std::string> environment;

                std::string sharedMemoryName;  // Auto-detected SharedMemory process name
            };

            ConfigLoader() = default;

            /*!
             * Load launcher configuration from XML file.
             * \param xmlFile Path to XML configuration file
             * \param launcherName Name of Launcher section (default: first found)
             * \return Loaded configuration
             */
            LauncherConfig load(const std::string& xmlFile,
                                const std::string& launcherName = "");

            /*!
             * Load launcher configuration from UniXML.
             * \param xml UniXML object
             * \param launcherName Name of Launcher section
             * \return Loaded configuration
             */
            LauncherConfig load(const std::shared_ptr<UniXML>& xml,
                                const std::string& launcherName = "");

            /*!
             * Check if Naming Service is required.
             * Based on localIOR parameter: if localIOR=1, Naming Service is NOT needed.
             * \param conf UniSet Configuration object
             * \return true if Naming Service is required
             */
            static bool needsNamingService(const std::shared_ptr<Configuration>& conf);

        private:
            void loadGroups(xmlNode* launcherNode, LauncherConfig& config);
            void loadProcess(xmlNode* processNode, const std::string& groupName,
                             LauncherConfig& config);
            void loadEnvironment(xmlNode* launcherNode, LauncherConfig& config);

            std::vector<std::string> parseArgs(const std::string& argsStr);
            std::set<std::string> parseNodeFilter(const std::string& filterStr);
    };

} // end of namespace uniset
// -------------------------------------------------------------------------
#endif // ConfigLoader_H_
// -------------------------------------------------------------------------
