/*
 * Copyright (c) 2026 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 */
// -------------------------------------------------------------------------
#include <iostream>
#include <csignal>
#include <atomic>
#include <Poco/Util/HelpFormatter.h>
#include "Configuration.h"
#include "UniSetTypes.h"
#include "ProcessManager.h"
#include "ConfigLoader.h"
#include "OmniNamesManager.h"
#include "Debug.h"
#ifndef DISABLE_REST_API
#include "UHttpServer.h"
#include "LauncherHttpRegistry.h"
#endif
// -------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// -------------------------------------------------------------------------
static std::atomic<bool> g_shutdown{false};
static ProcessManager* g_pm = nullptr;
// -------------------------------------------------------------------------
static void signal_handler(int sig)
{
    if (sig == SIGTERM || sig == SIGINT)
    {
        std::cerr << "Received signal " << sig << ", terminating..." << std::endl;
        g_shutdown = true;

        if (g_pm)
            g_pm->stopMonitoring();
    }
}
// -------------------------------------------------------------------------
static void print_help(const std::string& prog)
{
    cout << prog << " --confile configure.xml [--localNode NodeName] [OPTIONS] [-- ARGS...]\n"
         << "\n"
         << "UniSet2 Process Launcher - manages startup sequence and health monitoring.\n"
         << "\n"
         << "Options:\n"
         << "  --confile FILE       Configuration file (required)\n"
         << "  --localNode NAME     Local node name (default: from config localNode attribute)\n"
         << "  --launcher-name NAME Launcher section name in config\n"
         << "  --http-port PORT     HTTP API port (0 = disabled)\n"
         << "  --http-host HOST     HTTP API host (default: 0.0.0.0)\n"
         << "  --http-whitelist IPs Comma-separated whitelist (CIDR, ranges, IPs)\n"
         << "  --http-blacklist IPs Comma-separated blacklist (CIDR, ranges, IPs)\n"
         << "  --read-token TOKEN   Bearer token for read access (UI, GET API)\n"
         << "  --control-token TOKEN Bearer token for control (POST restart/stop/start)\n"
         << "  --html-template FILE Custom HTML template file\n"
         << "  --health-interval MS Health check interval in ms (default: 5000)\n"
         << "  --stop-timeout MS    Graceful shutdown timeout in ms (default: 5000)\n"
         << "  --uniset-port PORT   UniSet/CORBA port (default: auto=UID+52809)\n"
         << "  --omni-logdir DIR    omniNames log directory (default: $TMPDIR/omniORB)\n"
         << "  --disable-admin-create  Don't run uniset2-admin --create after omniNames start\n"
         << "  --no-monitor         Don't monitor processes after startup\n"
         << "  --runlist, --dry-run Show what will be launched without starting\n"
         << "  --verbose            Verbose output\n"
         << "  --help               Show this help\n"
         << "\n"
         << "REST API endpoints (when --http-port is set):\n"
         << "  GET  /                                       - Web UI\n"
         << "  GET  /ui                                     - Web UI\n"
         << "  GET  /api/v2/launcher                        - Launcher status\n"
         << "  GET  /api/v2/launcher/status                 - All processes status\n"
         << "  GET  /api/v2/launcher/processes              - List all processes (detailed)\n"
         << "  GET  /api/v2/launcher/process/{name}         - Specific process status\n"
         << "  POST /api/v2/launcher/process/{name}/restart - Restart process\n"
         << "  POST /api/v2/launcher/process/{name}/stop    - Stop process\n"
         << "  POST /api/v2/launcher/process/{name}/start   - Start process\n"
         << "  GET  /api/v2/launcher/health                 - Health check (for Docker/K8s)\n"
         << "  GET  /api/v2/launcher/groups                 - Process groups\n"
         << "  GET  /api/v2/launcher/help                   - API help\n"
         << "\n"
         << "Whitelist/Blacklist examples:\n"
         << "  --http-whitelist \"192.168.1.0/24,10.0.0.1\"\n"
         << "  --http-blacklist \"192.168.1.100,172.16.0.10-172.16.0.20\"\n"
         << "\n"
         << "Argument forwarding:\n"
         << "  Unknown arguments (e.g. --uniset-port, --lockDir) are automatically\n"
         << "  forwarded to child processes.\n"
         << "\n"
         << "  Arguments after '--' are also passed to children (explicit passthrough).\n"
         << "  Example: uniset2-launcher --confile config.xml --uniset-port 2809\n"
         << "  Example: uniset2-launcher --confile config.xml -- --custom-arg value\n"
         << endl;
}
// -------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    // Parse command line
    std::string confFile;
    std::string nodeName;
    std::string launcherName;
    int httpPort = 0;
    std::string httpHost = "0.0.0.0";
    std::string httpWhitelist;
    std::string httpBlacklist;
    std::string readToken;
    std::string controlToken;
    std::string htmlTemplate;
    size_t healthInterval = 5000;
    size_t stopTimeout = 5000;
    int unisetPort = OmniNamesManager::calcPortFromUID();  // default: auto (UID + 52809)
    std::string omniLogDir;
    bool disableAdminCreate = false;
    bool noMonitor = false;
    bool verbose = false;
    bool dryRun = false;
    std::string passthroughArgs;  // Arguments after "--" to pass to child processes
    std::vector<std::string> unknownArgs;  // Unknown arguments to pass to child processes

    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];

        // Everything after "--" is passed to child processes
        if (arg == "--")
        {
            for (int j = i + 1; j < argc; j++)
            {
                if (!passthroughArgs.empty())
                    passthroughArgs += " ";

                // Quote arguments containing spaces
                std::string parg = argv[j];

                if (parg.find(' ') != std::string::npos)
                    passthroughArgs += "\"" + parg + "\"";
                else
                    passthroughArgs += parg;
            }

            break;  // Stop parsing launcher arguments
        }

        if (arg == "--help" || arg == "-h")
        {
            print_help(argv[0]);
            return 0;
        }

        if (arg == "--confile" && i + 1 < argc)
        {
            confFile = argv[++i];
            continue;
        }

        if (arg == "--localNode" && i + 1 < argc)
        {
            nodeName = argv[++i];
            continue;
        }

        if (arg == "--launcher-name" && i + 1 < argc)
        {
            launcherName = argv[++i];
            continue;
        }

        if (arg == "--http-port" && i + 1 < argc)
        {
            httpPort = std::stoi(argv[++i]);
            continue;
        }

        if (arg == "--http-host" && i + 1 < argc)
        {
            httpHost = argv[++i];
            continue;
        }

        if (arg == "--http-whitelist" && i + 1 < argc)
        {
            httpWhitelist = argv[++i];
            continue;
        }

        if (arg == "--http-blacklist" && i + 1 < argc)
        {
            httpBlacklist = argv[++i];
            continue;
        }

        if (arg == "--read-token" && i + 1 < argc)
        {
            readToken = argv[++i];
            continue;
        }

        if (arg == "--control-token" && i + 1 < argc)
        {
            controlToken = argv[++i];
            continue;
        }

        if (arg == "--html-template" && i + 1 < argc)
        {
            htmlTemplate = argv[++i];
            continue;
        }

        if (arg == "--health-interval" && i + 1 < argc)
        {
            healthInterval = std::stoul(argv[++i]);
            continue;
        }

        if (arg == "--stop-timeout" && i + 1 < argc)
        {
            stopTimeout = std::stoul(argv[++i]);
            continue;
        }

        if (arg == "--omni-logdir" && i + 1 < argc)
        {
            omniLogDir = argv[++i];
            continue;
        }

        if (arg == "--uniset-port" && i + 1 < argc)
        {
            std::string portArg = argv[++i];

            if (portArg == "auto")
                unisetPort = OmniNamesManager::calcPortFromUID();
            else
                unisetPort = std::stoi(portArg);

            continue;
        }

        if (arg == "--disable-admin-create")
        {
            disableAdminCreate = true;
            continue;
        }

        if (arg == "--no-monitor")
        {
            noMonitor = true;
            continue;
        }

        if (arg == "--runlist" || arg == "--dry-run")
        {
            dryRun = true;
            continue;
        }

        if (arg == "--verbose" || arg == "-v")
        {
            verbose = true;
            continue;
        }

        // Collect unknown arguments to pass to child processes
        if (arg[0] == '-')
        {
            unknownArgs.push_back(arg);

            // Check if this argument has a value (next arg doesn't start with -)
            if (i + 1 < argc && argv[i + 1][0] != '-')
            {
                unknownArgs.push_back(argv[++i]);
            }
        }
    }

    // Validate required arguments
    if (confFile.empty())
    {
        cerr << "Error: --confile is required" << endl;
        print_help(argv[0]);
        return 1;
    }

    // Setup signal handlers
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGCHLD, SIG_IGN);  // Prevent zombie processes

    // Set environment variables for child processes (NODE_NAME set after determining nodeName)
    setenv("CONFFILE", confFile.c_str(), 0);  // Don't override if set

    try
    {
        // Build argv for uniset_init with --uniset-port if specified
        std::vector<std::string> initArgsStorage;
        std::vector<char*> initArgv;

        // Copy original argv
        for (int i = 0; i < argc; i++)
            initArgv.push_back(argv[i]);

        // Add --uniset-port with calculated value if needed
        std::string unisetPortStr;
        if (unisetPort > 0)
        {
            unisetPortStr = std::to_string(unisetPort);
            initArgsStorage.push_back("--uniset-port");
            initArgsStorage.push_back(unisetPortStr);
            initArgv.push_back(const_cast<char*>(initArgsStorage[0].c_str()));
            initArgv.push_back(const_cast<char*>(initArgsStorage[1].c_str()));
        }

        // Initialize UniSet configuration (required for CORBA checks)
        auto conf = uniset_init(static_cast<int>(initArgv.size()), initArgv.data(), confFile);

        if (!conf)
        {
            cerr << "Failed to initialize UniSet configuration" << endl;
            return 1;
        }

        // Get localNode from config if not specified on command line
        if (nodeName.empty())
        {
            nodeName = conf->getLocalNodeName();

            if (nodeName.empty())
            {
                cerr << "Error: --localNode not specified and localNode not found in config" << endl;
                return 1;
            }

            if (verbose)
                cout << "Using localNode from config: " << nodeName << endl;
        }

        // Set NODE_NAME environment variable for child processes
        setenv("NODE_NAME", nodeName.c_str(), 0);

        // Start omniNames if needed (not localIOR mode and port specified)
        std::unique_ptr<OmniNamesManager> omniManager;

        if (!conf->isLocalIOR() && unisetPort > 0)
        {
            omniManager = std::make_unique<OmniNamesManager>();
            omniManager->setPort(unisetPort);
            omniManager->setConfFile(confFile);

            if (!omniLogDir.empty())
                omniManager->setLogDir(omniLogDir);

            if (verbose)
                omniManager->log()->addLevel(Debug::ANY);

            cout << "Starting omniNames on port " << unisetPort << "..." << endl;

            if (!omniManager->start(!disableAdminCreate))
            {
                cerr << "Failed to start omniNames" << endl;
                return 1;
            }

            cout << "omniNames is ready" << endl;
        }
        else if (verbose)
        {
            cout << "localIOR mode or no port specified - omniNames not started" << endl;
        }

        // Load launcher-specific configuration
        cout << "Loading configuration from " << confFile << endl;

        ConfigLoader loader;
        auto config = loader.load(confFile, launcherName);

        // Override settings from command line
        if (httpPort > 0)
            config.httpPort = httpPort;

        config.healthCheckInterval_msec = healthInterval;

        // Apply environment variables from config
        for (const auto& kv : config.environment)
            setenv(kv.first.c_str(), kv.second.c_str(), 0);

        // Create ProcessManager with UniSet configuration for CORBA checks
        ProcessManager pm(conf);
        g_pm = &pm;

        pm.setNodeName(nodeName);
        pm.setHealthCheckInterval(config.healthCheckInterval_msec);
        pm.setRestartWindow(config.restartWindow_msec);
        pm.setStopTimeout(stopTimeout);
        pm.setCommonArgs(config.commonArgs);

        if (!passthroughArgs.empty())
        {
            pm.setPassthroughArgs(passthroughArgs);

            if (verbose)
                cout << "Passthrough args: " << passthroughArgs << endl;
        }

        // Add --uniset-port to forwarded args for child processes
        if (unisetPort > 0)
        {
            unknownArgs.push_back("--uniset-port");
            unknownArgs.push_back(std::to_string(unisetPort));
        }

        if (!unknownArgs.empty())
        {
            pm.setForwardArgs(unknownArgs);

            if (verbose)
            {
                cout << "Forwarding args:";

                for (const auto& a : unknownArgs)
                    cout << " " << a;

                cout << endl;
            }
        }

        // Setup logging
        if (verbose)
        {
            pm.log()->addLevel(Debug::ANY);
            pm.log()->showDateTime(true);
        }
        else
        {
            pm.log()->addLevel(Debug::INFO);
            pm.log()->addLevel(Debug::WARN);
            pm.log()->addLevel(Debug::CRIT);
        }

        // Register groups and processes
        for (const auto& group : config.groups)
            pm.addGroup(group);

        for (const auto& kv : config.processes)
            pm.addProcess(kv.second);

        // Dry-run mode: show what will be launched and exit
        if (dryRun)
        {
            pm.printRunList(cout);
            return 0;
        }

        // Setup callbacks
        pm.setOnProcessStarted([](const ProcessInfo & proc)
        {
            cout << "[STARTED] " << proc.name << " (PID " << proc.pid << ")" << endl;
        });

        pm.setOnProcessFailed([](const ProcessInfo & proc)
        {
            cerr << "[FAILED] " << proc.name << ": " << proc.lastError << endl;
        });

        pm.setOnProcessStopped([](const ProcessInfo & proc)
        {
            cout << "[STOPPED] " << proc.name << endl;
        });

        // Start HTTP server
#ifndef DISABLE_REST_API
        std::shared_ptr<UHttp::UHttpServer> httpServer;

        if (config.httpPort > 0)
        {
            cout << "Starting HTTP server on " << httpHost << ":" << config.httpPort << endl;

            auto launcherRegistry = std::make_shared<LauncherHttpRegistry>(pm);
            launcherRegistry->setReadToken(readToken);
            launcherRegistry->setControlToken(controlToken);
            launcherRegistry->setHtmlTemplate(htmlTemplate);
            std::shared_ptr<UHttp::IHttpRequestRegistry> registry = launcherRegistry;
            httpServer = std::make_shared<UHttp::UHttpServer>(registry, httpHost, config.httpPort);

            if (!httpWhitelist.empty())
            {
                httpServer->setWhitelist(uniset::explode_str(httpWhitelist, ','));
                cout << "HTTP whitelist: " << httpWhitelist << endl;
            }

            if (!httpBlacklist.empty())
            {
                httpServer->setBlacklist(uniset::explode_str(httpBlacklist, ','));
                cout << "HTTP blacklist: " << httpBlacklist << endl;
            }

            httpServer->start();
        }

#endif

        // Start all processes
        cout << "Starting processes for node " << nodeName << "..." << endl;

        int exitCode = 0;

        if (!pm.startAll())
        {
            cerr << "Critical process failed to start" << endl;
            exitCode = 1;
        }
        else
        {
            if (pm.allRunning())
                cout << "All processes started successfully" << endl;
            else
                cerr << "Some processes failed to start (non-critical)" << endl;

            // Start monitoring
            if (!noMonitor)
            {
                pm.startMonitoring();
                cout << "Process monitoring enabled (interval: "
                     << config.healthCheckInterval_msec << "ms)" << endl;
            }

            // Wait for shutdown signal
            cout << "Launcher running. Press Ctrl+C to stop." << endl;

            while (!g_shutdown)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));

                // Check for critical failures
                if (pm.anyCriticalFailed())
                {
                    cerr << "Critical process failed, shutting down" << endl;
                    exitCode = 1;
                    break;
                }
            }
        }

        // Shutdown
        cout << "Shutting down..." << endl;

#ifndef DISABLE_REST_API

        if (httpServer)
        {
            httpServer->stop();
            httpServer.reset();  // Release before pm is destroyed
        }

#endif

        pm.stopMonitoring();
        pm.stopAll();

        // Stop omniNames if we started it
        if (omniManager && omniManager->wasStartedByUs())
        {
            cout << "Stopping omniNames..." << endl;
            omniManager->stop(stopTimeout);
        }

        cout << "Shutdown complete" << endl;
        return exitCode;
    }
    catch (const std::exception& e)
    {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
}
// -------------------------------------------------------------------------
