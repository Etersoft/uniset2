/*
 * Copyright (c) 2026 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 */
// -------------------------------------------------------------------------
#ifndef UniSetUno_H_
#define UniSetUno_H_
// -------------------------------------------------------------------------
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include "UniSetTypes.h"
#include "UniSetObject.h"
#include "SharedMemory.h"
#include "UniXML.h"
#include "Debug.h"
// -------------------------------------------------------------------------
namespace uniset
{
    /*!
     * \brief UniSetUno - Combined application running multiple extensions in single process
     *
     * Runs SharedMemory and multiple extensions (UNetExchange, MBTCPMaster, etc.)
     * in a single process, sharing memory through direct pointers instead of CORBA.
     *
     * Benefits:
     * - No IPC overhead (direct memory access)
     * - Single process instead of many
     * - Simpler deployment
     * - Lower memory footprint
     *
     * XML Configuration:
     * \code
     * <UniSetUno name="MyApp" logfile="uno.log">
     *   <extensions>
     *     <extension type="SharedMemory" name="SharedMemory"/>
     *     <extension type="UNetExchange" name="UNetExchange1" prefix="unet"/>
     *     <extension type="MBTCPMaster" name="MBTCPMaster1" prefix="mbtcp"/>
     *     <extension type="MBTCPMultiMaster" name="MBMulti1" prefix="mbtcp"/>
     *     <extension type="MBSlave" name="MBSlave1" prefix="mbs"/>
     *     <extension type="RTUExchange" name="RTU1" prefix="rs"/>
     *     <extension type="OPCUAServer" name="OPCUAServer1" prefix="opcua"/>
     *     <extension type="OPCUAExchange" name="OPCUAExchange1" prefix="opcua"/>
     *     <extension type="IOControl" name="IOControl1" prefix="io"/>
     *     <extension type="LogicProcessor" name="LProc1" prefix="lproc"/>
     *   </extensions>
     * </UniSetUno>
     * \endcode
     */
    class UniSetUno
    {
        public:
            UniSetUno();
            ~UniSetUno();

            /*!
             * Initialize from command line and XML configuration
             * \param argc Command line argument count
             * \param argv Command line arguments
             * \param appName Name of UniSetApp section in config (default: first found)
             * \return true if initialization succeeded
             */
            bool init(int argc, const char* const* argv, const std::string& appName = "");

            /*!
             * Run the application (blocking)
             * Activates all extensions and runs the ORB
             */
            void run();

            /*!
             * Stop the application
             */
            void stop();

            /*!
             * Get SharedMemory instance
             */
            std::shared_ptr<SharedMemory> getSharedMemory() const;

            /*!
             * Get extension by name
             */
            std::shared_ptr<UniSetObject> getExtension(const std::string& name) const;

            /*!
             * Get all extension names
             */
            std::vector<std::string> getExtensionNames() const;

            /*!
             * Check if all extensions are running
             */
            bool isRunning() const;

            /*!
             * Get log stream
             */
            std::shared_ptr<DebugStream> log();

            /*!
             * Print help for UniSetUno and optionally for extensions
             * \param argc Command line argument count
             * \param argv Command line arguments
             * \param extName If empty, print help for all extensions. If specified, print help for that extension only.
             */
            static void help_print(int argc, const char* const* argv, const std::string& extName = "");

            /*!
             * Print help for specific extension by type name
             * \return true if extension was found
             */
            static bool help_extension(int argc, const char* const* argv, const std::string& extType);

            /*!
             * Parse command-line arguments string into vector.
             * Handles quoted strings (single and double quotes).
             * \param argsStr String with space-separated arguments
             * \return Vector of individual arguments
             */
            static std::vector<std::string> parseArgsString(const std::string& argsStr);

            /*!
             * Collect args from all extensions in UniSetUno configuration.
             * Used for pre-loading args before uniset_init().
             * \param xml Loaded XML configuration
             * \param appName UniSetUno section name (empty = first found)
             * \return Vector of all collected arguments
             */
            static std::vector<std::string> collectExtensionArgs(
                const std::shared_ptr<UniXML>& xml,
                const std::string& appName);

            // Supported extension types
            static constexpr const char* EXT_SHARED_MEMORY = "SharedMemory";
            static constexpr const char* EXT_UNET_EXCHANGE = "UNetExchange";
            static constexpr const char* EXT_MBTCP_MASTER = "MBTCPMaster";
            static constexpr const char* EXT_MBTCP_MULTIMASTER = "MBTCPMultiMaster";
            static constexpr const char* EXT_MB_SLAVE = "MBSlave";
            static constexpr const char* EXT_RTU_EXCHANGE = "RTUExchange";
            static constexpr const char* EXT_OPCUA_SERVER = "OPCUAServer";
            static constexpr const char* EXT_OPCUA_EXCHANGE = "OPCUAExchange";
            static constexpr const char* EXT_IO_CONTROL = "IOControl";
            static constexpr const char* EXT_LOGIC_PROCESSOR = "LogicProcessor";
            static constexpr const char* EXT_MQTT_PUBLISHER = "MQTTPublisher";
            static constexpr const char* EXT_WEBSOCKET_GATE = "UWebSocketGate";
            static constexpr const char* EXT_BACKEND_CLICKHOUSE = "BackendClickHouse";
            static constexpr const char* EXT_DBSERVER_POSTGRESQL = "DBServerPostgreSQL";

        private:
            struct ExtensionInfo
            {
                std::string type;
                std::string name;
                std::string prefix;
                std::string args;  //!< Additional command-line arguments for this extension
                std::shared_ptr<UniSetObject> object;
            };

            bool loadConfig(const std::string& appName);
            bool validateExtensions();
            static std::string getDefaultPrefix(const std::string& type);
            bool createExtensions(int argc, const char* const* argv);

            std::shared_ptr<UniSetObject> createExtension(
                int argc, const char* const* argv,
                const ExtensionInfo& info);

            std::shared_ptr<DebugStream> applog;
            std::shared_ptr<SharedMemory> shm_;
            std::vector<ExtensionInfo> extensions_;
            std::unordered_map<std::string, std::shared_ptr<UniSetObject>> extByName_;

            bool running_ = false;
            std::string logfile_;
    };

} // end of namespace uniset
// -------------------------------------------------------------------------
#endif // UniSetUno_H_
// -------------------------------------------------------------------------
