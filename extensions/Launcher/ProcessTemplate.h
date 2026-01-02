/*
 * Copyright (c) 2026 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 */
// -------------------------------------------------------------------------
#ifndef ProcessTemplate_H_
#define ProcessTemplate_H_
// -------------------------------------------------------------------------
#include <string>
#include <vector>
#include <map>
// -------------------------------------------------------------------------
namespace uniset
{
    /*!
     * Process template for automatic configuration.
     * Contains default parameters for known UniSet process types.
     */
    struct ProcessTemplate
    {
        std::string type;           //!< Type identifier (SharedMemory, UNetExchange, etc.)
        std::string command;        //!< Command to run (uniset2-smemory, etc.)
        std::string argsPattern;    //!< Arguments pattern (${name} is replaced with process name)
        std::string readyCheck;     //!< Ready check pattern (${name} is replaced, empty = no check)
        size_t readyTimeout_msec = 30000;  //!< Ready check timeout (total wait time)
        size_t checkTimeout_msec = 1000;   //!< Single check timeout (for health monitoring)
        size_t checkPause_msec = 1000;     //!< Pause between checks
        int group = 2;              //!< Startup group (0=naming, 1=sharedmemory, 2=exchanges)
        bool critical = false;      //!< Process is critical by default
        bool needsSharedMemory = true;  //!< Add --smemory-id automatically
        std::vector<std::string> prefixes;  //!< Name prefixes for auto-detection
    };

    /*!
     * Registry of built-in process templates.
     */
    class ProcessTemplateRegistry
    {
        public:
            ProcessTemplateRegistry();

            /*!
             * Find template by type name.
             * \param type Type name (e.g. "SharedMemory", "UNetExchange")
             * \return Pointer to template or nullptr if not found
             */
            const ProcessTemplate* findByType(const std::string& type) const;

            /*!
             * Find template by process name using prefix matching.
             * \param name Process name (e.g. "SharedMemory1", "UNetExchange1")
             * \return Pointer to template or nullptr if no prefix matches
             */
            const ProcessTemplate* findByPrefix(const std::string& name) const;

            /*!
             * Detect process type by name.
             * \param name Process name
             * \return Type string or empty if not detected
             */
            std::string detectType(const std::string& name) const;

            /*!
             * Register custom template.
             * \param tmpl Template to register
             */
            void registerTemplate(const ProcessTemplate& tmpl);

            /*!
             * Get all registered templates.
             */
            const std::vector<ProcessTemplate>& templates() const
            {
                return templates_;
            }

            /*!
             * Expand pattern by replacing ${name} with actual name.
             * \param pattern Pattern string with ${name} placeholder
             * \param name Process name to substitute
             * \return Expanded string
             */
            static std::string expandPattern(const std::string& pattern, const std::string& name);

        private:
            void registerBuiltinTemplates();

            std::vector<ProcessTemplate> templates_;
            std::map<std::string, size_t> typeIndex_;  // type -> index in templates_
    };

    /*!
     * Get global template registry (singleton).
     */
    ProcessTemplateRegistry& getProcessTemplateRegistry();

} // end of namespace uniset
// -------------------------------------------------------------------------
#endif // ProcessTemplate_H_
// -------------------------------------------------------------------------
