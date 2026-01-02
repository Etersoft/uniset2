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
#include <cstring>
#include "Configuration.h"
#include "UniXML.h"
#include "UniSetUno.h"
// -------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// -------------------------------------------------------------------------
static UniSetUno* g_app = nullptr;
// -------------------------------------------------------------------------
static void signal_handler(int sig)
{
    if (sig == SIGTERM || sig == SIGINT)
    {
        if (g_app)
            g_app->stop();
    }
}
// -------------------------------------------------------------------------
// Find argument value in argv (simple search without uniset_conf)
static std::string findArgValue(int argc, const char* const* argv,
                                const std::string& argName,
                                const std::string& defaultValue = "")
{
    for (int i = 1; i < argc - 1; i++)
    {
        if (argName == argv[i])
            return argv[i + 1];
    }

    return defaultValue;
}
// -------------------------------------------------------------------------
int main(int argc, const char* argv[])
{
    // Check for help
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            // Check if next argument is extension name
            std::string extName;

            if (i + 1 < argc && argv[i + 1][0] != '-')
                extName = argv[i + 1];

            UniSetUno::help_print(argc, argv, extName);
            return 0;
        }
    }

    // Parse uno-specific arguments (before uniset_init)
    std::string appName = findArgValue(argc, argv, "--uno-name");

    if (appName.empty())
        appName = findArgValue(argc, argv, "--app-name");

    std::string confile = findArgValue(argc, argv, "--confile", "configure.xml");

    try
    {
        // Load XML first (before uniset_init)
        auto xml = std::make_shared<UniXML>(confile);

        // Collect args from extension configurations
        auto extArgs = UniSetUno::collectExtensionArgs(xml, appName);

        // Build extended argv: program name + extension args + original args
        // Priority: original argv overrides extension args (original args come last)
        std::vector<std::string> argsStorage;
        std::vector<const char*> newArgv;

        // Program name
        newArgv.push_back(argv[0]);

        // Extension args from config (lower priority)
        for (const auto& arg : extArgs)
        {
            argsStorage.push_back(arg);
            newArgv.push_back(argsStorage.back().c_str());
        }

        // Original args (higher priority - can override config)
        for (int i = 1; i < argc; i++)
            newArgv.push_back(argv[i]);

        int newArgc = static_cast<int>(newArgv.size());

        // Initialize UniSet configuration with pre-loaded XML
        auto conf = uniset_init(newArgc, newArgv.data(), xml);

        if (!conf)
        {
            cerr << "Failed to initialize UniSet configuration" << endl;
            return 1;
        }

        // Setup signal handlers
        signal(SIGTERM, signal_handler);
        signal(SIGINT, signal_handler);

        // Create and initialize app
        UniSetUno app;
        g_app = &app;

        if (!app.init(newArgc, newArgv.data(), appName))
        {
            cerr << "Failed to initialize UniSetUno" << endl;
            return 1;
        }

        // Print startup info
        cout << "UniSetUno starting with extensions:" << endl;

        for (const auto& name : app.getExtensionNames())
            cout << "  - " << name << endl;

        cout << endl;

        // Run (blocking)
        app.run();

        cout << "UniSetUno finished" << endl;
        return 0;
    }
    catch (const uniset::SystemError& ex)
    {
        cerr << "SystemError: " << ex << endl;
    }
    catch (const uniset::Exception& ex)
    {
        cerr << "UniSet Exception: " << ex << endl;
    }
    catch (const std::exception& ex)
    {
        cerr << "Error: " << ex.what() << endl;
    }

    return 1;
}
// -------------------------------------------------------------------------
