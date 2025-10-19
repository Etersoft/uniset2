/*
 * Copyright (c) 2025 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Lesser Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
// --------------------------------------------------------------------------
// Logging module for uniset2
// generated with deep seek AI
// --------------------------------------------------------------------------
var unisetLogModuleVersion = "v1"
// --------------------------------------------------------------------------
// Example
// var mylog = uniset_log_create("my", true, true);
// mylog.level("crit", "warn", "info", "level1");
// mylog.info("info message");
// mylog.log9("level9 message");
// mylog.crit("crit message");
// --------------------------------------------------------------------------
var LogLevels = {
    NONE:       0,
    INFO:       1 << 0,   // 1
    INIT:       1 << 1,   // 2
    WARN:       1 << 2,   // 4
    CRIT:       1 << 3,   // 8
    LEVEL1:     1 << 4,   // 16
    LEVEL2:     1 << 5,   // 32
    LEVEL3:     1 << 6,   // 64
    LEVEL4:     1 << 7,   // 128
    LEVEL5:     1 << 8,   // 256
    LEVEL6:     1 << 9,   // 512
    LEVEL7:     1 << 10,  // 1024
    LEVEL8:     1 << 11,  // 2048
    LEVEL9:     1 << 12,  // 4096
    REPOSITORY: 1 << 13,  // 8192
    SYSTEM:     1 << 14,  // 16384
    EXCEPTION:  1 << 15,  // 32768
    ANY:        0xFFFF    // Все уровни
};

// Совместимость со старым кодом - оставляем ALL как синоним для ANY
LogLevels.ALL = LogLevels.ANY;
// --------------------------------------------------------------------------
// Function to create logging object
function uniset_log_create(logname, showdate, showlogname, showemoji, internallog) {
    if (logname === undefined) logname = "default";
    if (showdate === undefined) showdate = true;
    if (showlogname === undefined) showlogname = true;
    if (showemoji === undefined) showemoji = true;
    if (internallog === undefined) internallog = true;

    // Bitmask of enabled levels (default: crit, warn, info)
    var enabledLevels = LogLevels.CRIT | LogLevels.WARN | LogLevels.INFO;
    if( internallog && uniset_internal_log_level !== undefined )
        uniset_internal_log_level(enabledLevels);

    // Widths for alignment
    var levelWidth = 9;   // maximum level width (REPOSITORY, EXCEPTION)
    var nameWidth = 10;   // maximum module name width

    // Function to check if level is enabled
    function isLevelEnabled(level) {
        return (enabledLevels & level) !== 0;
    }

    // Function to format date/time
    function formatTime() {
        if (!showdate) return "";
        var now = new Date();
        return now.toISOString().replace('T', ' ').substring(0, 19) + " ";
    }

    // Function to pad string
    function padString(str, width, alignLeft) {
        if (alignLeft === undefined) alignLeft = true;

        if (str.length >= width) {
            return str.substring(0, width);
        }

        var padding = ' '.repeat(width - str.length);
        return alignLeft ? str + padding : padding + str;
    }

    // Function to format level
    function formatLevel(level) {
        var levelStr = levelNames[level] || "UNKNOWN";
        return padString(levelStr, levelWidth, false); // right alignment
    }

    // Function to format module name
    function formatName(name) {
        if (!showlogname) return "";
        return padString(name, nameWidth, true); // left alignment
    }

    // Function to get emoji for level
    function getEmoji(level) {
        if (!showemoji) return "";

        switch(level) {
            case LogLevels.CRIT: return "🔴 ";
            case LogLevels.WARN: return "🟡 ";
            case LogLevels.INFO: return "🔵 ";
            case LogLevels.INIT: return "⚙️ ";
            case LogLevels.LEVEL1: return "1️⃣ ";
            case LogLevels.LEVEL2: return "2️⃣ ";
            case LogLevels.LEVEL3: return "3️⃣ ";
            case LogLevels.LEVEL4: return "4️⃣ ";
            case LogLevels.LEVEL5: return "5️⃣ ";
            case LogLevels.LEVEL6: return "6️⃣ ";
            case LogLevels.LEVEL7: return "7️⃣ ";
            case LogLevels.LEVEL8: return "8️⃣ ";
            case LogLevels.LEVEL9: return "9️⃣ ";
            case LogLevels.REPOSITORY: return "🗄️ ";
            case LogLevels.SYSTEM: return "💻 ";
            case LogLevels.EXCEPTION: return "🚨 ";
            default: return "";
        }
    }

    // Function to format base message (without data)
    function formatBaseMessage(level) {
        var timeStr = formatTime();
        var levelStr = "[" + formatLevel(level) + "]";
        var nameStr = formatName(logname);
        var separator = nameStr ? " " : "";
        var nameSuffix = nameStr ? ": " : " ";

        return timeStr + levelStr + separator + nameStr + nameSuffix;
    }

    // Function to convert arguments to string
    function argsToString(args) {
        if (args.length === 0) return "";

        var result = [];
        for (var i = 0; i < args.length; i++) {
            var arg = args[i];

            if (arg === null) {
                result.push("null");
            } else if (arg === undefined) {
                result.push("undefined");
            } else if (typeof arg === 'object') {
                try {
                    result.push(JSON.stringify(arg, null, 2));
                } catch (e) {
                    result.push("[Object]");
                }
            } else {
                result.push(String(arg));
            }
        }

        return result.join(" ");
    }

    // Function to set levels from string names
    function setLevelsFromStrings() {
        enabledLevels = LogLevels.NONE;
        for (var i = 0; i < arguments.length; i++) {
            var levelName = arguments[i];
            if (levelMap[levelName]) {
                enabledLevels |= levelMap[levelName];
            }
        }
        if( internallog && uniset_internal_log_level !== undefined )
            uniset_internal_log_level(enabledLevels);
    }

    // Function to log message
    function logMessage(level, args) {
        if (isLevelEnabled(level)) {
            var message = argsToString(args);
            var emoji = getEmoji(level);
            var formatted = formatBaseMessage(level) + message;

            if( internallog && uniset_internal_log !== undefined )
                uniset_internal_log(emoji + formatted, level)
            else
                console.log(emoji + formatted);
        }
    }

    // Create logging object
    var logger = {
        // Set levels using string names
        level: function() {
            if (arguments.length === 0) {
                // Return current mask
                return enabledLevels;
            }
            setLevelsFromStrings.apply(null, arguments);
            return enabledLevels;
        },

        // Set levels using bitmask
        setLevelMask: function(mask) {
            enabledLevels = mask;
            return enabledLevels;
        },

        // Enable all levels
        enableAll: function() {
            enabledLevels = LogLevels.ANY;
            return enabledLevels;
        },

        // Disable all levels
        disableAll: function() {
            enabledLevels = LogLevels.NONE;
            return enabledLevels;
        },

        // Check if level is enabled
        isEnabled: function(levelName) {
            var level = levelMap[levelName];
            return level ? isLevelEnabled(level) : false;
        },

        // Set widths for alignment
        setWidths: function(levelW, nameW) {
            if (levelW !== undefined) levelWidth = Math.max(5, levelW);
            if (nameW !== undefined) nameWidth = Math.max(3, nameW);
        },

        // Enable/disable date display
        showDate: function(show) {
            if (show !== undefined) {
                showdate = show;
            }
            return showdate;
        },

        // Enable/disable module name display
        showLogName: function(show) {
            if (show !== undefined) {
                showlogname = show;
            }
            return showlogname;
        },

        // Enable/disable emoji display
        showEmoji: function(show) {
            if (show !== undefined) {
                showemoji = show;
            }
            return showemoji;
        },

        internallog: function(show) {
            if (show !== undefined) {
                internallog = show;
                if( uniset_internal_log_level !== undefined )
                    uniset_internal_log_level(enabledLevels);
            }
            return internallog;
        },

        // Logging functions supporting any number of arguments
        info: function() {
            logMessage(LogLevels.INFO, arguments);
        },

        init: function() {
            logMessage(LogLevels.INIT, arguments);
        },

        warn: function() {
            logMessage(LogLevels.WARN, arguments);
        },

        crit: function() {
            logMessage(LogLevels.CRIT, arguments);
        },

        log1: function() {
            logMessage(LogLevels.LEVEL1, arguments);
        },

        log2: function() {
            logMessage(LogLevels.LEVEL2, arguments);
        },

        log3: function() {
            logMessage(LogLevels.LEVEL3, arguments);
        },

        log4: function() {
            logMessage(LogLevels.LEVEL4, arguments);
        },

        log5: function() {
            logMessage(LogLevels.LEVEL5, arguments);
        },

        log6: function() {
            logMessage(LogLevels.LEVEL6, arguments);
        },

        log7: function() {
            logMessage(LogLevels.LEVEL7, arguments);
        },

        log8: function() {
            logMessage(LogLevels.LEVEL8, arguments);
        },

        log9: function() {
            logMessage(LogLevels.LEVEL9, arguments);
        },

        repository: function() {
            logMessage(LogLevels.REPOSITORY, arguments);
        },

        system: function() {
            logMessage(LogLevels.SYSTEM, arguments);
        },

        exception: function() {
            logMessage(LogLevels.EXCEPTION, arguments);
        },

        // Universal logging function
        log: function(levelName) {
            var level = levelMap[levelName];
            if (level && isLevelEnabled(level)) {
                // Skip first argument (levelName)
                var args = Array.prototype.slice.call(arguments, 1);
                logMessage(level, args);
            }
        }
    };

    return logger;
}

// --------------------------------------------------------------------------
// Global default logger
var defaultLogger = uniset_log_create("global", true, true, true);

// Global convenience functions (use defaultLogger)
function uinfo() {
    defaultLogger.info.apply(defaultLogger, arguments);
}

function uinit() {
    defaultLogger.init.apply(defaultLogger, arguments);
}

function uwarn() {
    defaultLogger.warn.apply(defaultLogger, arguments);
}

function ucrit() {
    defaultLogger.crit.apply(defaultLogger, arguments);
}

function ulog1() {
    defaultLogger.log1.apply(defaultLogger, arguments);
}

function ulog2() {
    defaultLogger.log2.apply(defaultLogger, arguments);
}

function ulog3() {
    defaultLogger.log3.apply(defaultLogger, arguments);
}

function ulog4() {
    defaultLogger.log4.apply(defaultLogger, arguments);
}

function ulog5() {
    defaultLogger.log5.apply(defaultLogger, arguments);
}

function ulog6() {
    defaultLogger.log6.apply(defaultLogger, arguments);
}

function ulog7() {
    defaultLogger.log7.apply(defaultLogger, arguments);
}

function ulog8() {
    defaultLogger.log8.apply(defaultLogger, arguments);
}

function ulog9() {
    defaultLogger.log9.apply(defaultLogger, arguments);
}

function urepository() {
    defaultLogger.repository.apply(defaultLogger, arguments);
}

function usystem() {
    defaultLogger.system.apply(defaultLogger, arguments);
}

function uexception() {
    defaultLogger.exception.apply(defaultLogger, arguments);
}

function ulog(level) {
    defaultLogger.log.apply(defaultLogger, arguments);
}

// Function to configure global logger
function uniset_log() {
    if (arguments.length === 0) {
        return defaultLogger.level();
    }
    return defaultLogger.level.apply(defaultLogger, arguments);
}

// Function to manage emoji display in global logger
function uniset_log_show_emoji(show) {
    if (show !== undefined) {
        defaultLogger.showEmoji(show);
    }
    return defaultLogger.showEmoji();
}
// --------------------------------------------------------------------------
// Mapping of string names to bit masks
var levelMap = {
    "none": LogLevels.NONE,
    "info": LogLevels.INFO,
    "init": LogLevels.INIT,
    "warn": LogLevels.WARN,
    "crit": LogLevels.CRIT,
    "level1": LogLevels.LEVEL1,
    "level2": LogLevels.LEVEL2,
    "level3": LogLevels.LEVEL3,
    "level4": LogLevels.LEVEL4,
    "level5": LogLevels.LEVEL5,
    "level6": LogLevels.LEVEL6,
    "level7": LogLevels.LEVEL7,
    "level8": LogLevels.LEVEL8,
    "level9": LogLevels.LEVEL9,
    "repository": LogLevels.REPOSITORY,
    "system": LogLevels.SYSTEM,
    "exception": LogLevels.EXCEPTION,
    "any": LogLevels.ANY
};

// Reverse mapping for formatting
var levelNames = {
    [LogLevels.NONE]: "NONE",
    [LogLevels.INFO]: "INFO",
    [LogLevels.INIT]: "INIT",
    [LogLevels.WARN]: "WARN",
    [LogLevels.CRIT]: "CRIT",
    [LogLevels.LEVEL1]: "LEVEL1",
    [LogLevels.LEVEL2]: "LEVEL2",
    [LogLevels.LEVEL3]: "LEVEL3",
    [LogLevels.LEVEL4]: "LEVEL4",
    [LogLevels.LEVEL5]: "LEVEL5",
    [LogLevels.LEVEL6]: "LEVEL6",
    [LogLevels.LEVEL7]: "LEVEL7",
    [LogLevels.LEVEL8]: "LEVEL8",
    [LogLevels.LEVEL9]: "LEVEL9",
    [LogLevels.REPOSITORY]: "REPOSITORY",
    [LogLevels.SYSTEM]: "SYSTEM",
    [LogLevels.EXCEPTION]: "EXCEPTION",
    [LogLevels.ANY]: "ANY"
};
// --------------------------------------------------------------------------

/* === HOIST TO globalThis FOR ESM COMPAT === */
try {
    if (typeof uinfo === 'function') globalThis.uinfo = uinfo;
    if (typeof uinit === 'function') globalThis.uinit = uinit;
    if (typeof uwarn === 'function') globalThis.uwarn = uwarn;
    if (typeof ucrit === 'function') globalThis.ucrit = ucrit;
    if (typeof urepository === 'function') globalThis.urepository = urepository;
    if (typeof usystem === 'function') globalThis.usystem = usystem;
    if (typeof uexception === 'function') globalThis.uexception = uexception;

    if (typeof uniset_log === 'function') globalThis.uniset_log = uniset_log;
    if (typeof uniset_log_create === 'function') globalThis.uniset_log_create = uniset_log_create;
    if (typeof uniset_internal_log === 'function') globalThis.uniset_internal_log = uniset_internal_log;
    if (typeof uniset_internal_log_level === 'function') globalThis.uniset_internal_log_level = uniset_internal_log_level;

    if (typeof LogLevels !== 'undefined') globalThis.LogLevels = LogLevels;
    if (typeof defaultLogger !== 'undefined') globalThis.defaultLogger = defaultLogger;

    if (typeof unisetLogModuleVersion !== 'undefined')
        globalThis.unisetLogModuleVersion = unisetLogModuleVersion;
} catch (_) {}
/* === END HOIST === */