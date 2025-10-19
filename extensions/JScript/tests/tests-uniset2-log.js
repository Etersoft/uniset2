/*
 * Tests for uniset2-log.js logging module
 */

load("uniset2-log.js");

// Helper function for intercepting both console.log and uniset_internal_log
function createLogInterceptor() {
    var logs = [];
    var originalConsoleLog = console.log;
    var originalInternalLog = typeof uniset_internal_log !== 'undefined' ? uniset_internal_log : null;
    var originalInternalLogLevel = typeof uniset_internal_log_level !== 'undefined' ? uniset_internal_log_level : null;

    // Intercept console.log
    console.log = function() {
        var message = Array.prototype.slice.call(arguments).join(' ');
        logs.push({type: 'console', message: message});
        // originalConsoleLog.apply(console, arguments); // uncomment for debugging
    };

    // Intercept uniset_internal_log if it exists
    if (originalInternalLog) {
        uniset_internal_log = function(message, level) {
            logs.push({type: 'internal', message: message, level: level});
            // originalInternalLog(message, level); // uncomment for debugging
        };
    }

    // Intercept uniset_internal_log_level if it exists
    if (originalInternalLogLevel) {
        uniset_internal_log_level = function(levelMask) {
            logs.push({type: 'level_change', levelMask: levelMask});
            // originalInternalLogLevel(levelMask); // uncomment for debugging
        };
    }

    return {
        logs: logs,
        restore: function() {
            console.log = originalConsoleLog;
            if (originalInternalLog) {
                uniset_internal_log = originalInternalLog;
            }
            if (originalInternalLogLevel) {
                uniset_internal_log_level = originalInternalLogLevel;
            }
        },
        clear: function() {
            logs.length = 0;
        },
        contains: function(text) {
            return logs.some(function(log) {
                return log.message && log.message.indexOf(text) !== -1;
            });
        },
        getByLevel: function(levelName) {
            return logs.filter(function(log) {
                return log.message && log.message.indexOf('[' + levelName + ']') !== -1;
            });
        },
        getByText: function(text) {
            return logs.filter(function(log) {
                return log.message && log.message.indexOf(text) !== -1;
            });
        },
        // Get all messages (both console and internal)
        getAllMessages: function() {
            return logs.filter(function(log) {
                return log.message !== undefined;
            }).map(function(log) {
                return log.message;
            });
        },
        // Check if any log contains text
        anyContains: function(text) {
            return logs.some(function(log) {
                return log.message && log.message.indexOf(text) !== -1;
            });
        }
    };
}

// Main tests
function runLogTests() {
    var testResults = {
        passed: 0,
        failed: 0,
        tests: []
    };

    function test(name, fn) {
        var logInterceptor = createLogInterceptor();
        try {
            fn(logInterceptor);
            testResults.tests.push({name: name, status: 'PASSED'});
            testResults.passed++;
            console.log("✓ " + name);
        } catch (e) {
            testResults.tests.push({name: name, status: 'FAILED', error: e.message});
            testResults.failed++;
            console.log("✗ " + name + " - " + e.message);
        } finally {
            logInterceptor.restore();
        }
    }

    function assert(condition, message) {
        if (!condition) {
            throw new Error(message || "Assertion failed");
        }
    }

    function assertEqual(actual, expected, message) {
        if (actual !== expected) {
            throw new Error((message || "Values not equal") +
                " - Expected: " + expected + ", Got: " + actual);
        }
    }

    console.log("\n=== Running uniset2-log.js Tests ===\n");

    // Test 1: Logger creation
    test("Creating logger with default parameters", function(interceptor) {
        var log = uniset_log_create("test-logger");
        assert(log !== undefined, "Logger not created");
        assert(log.info !== undefined, "No info method");
        assert(log.warn !== undefined, "No warn method");
        assert(log.crit !== undefined, "No crit method");
        assert(log.init !== undefined, "No init method");
        assert(log.repository !== undefined, "No repository method");
        assert(log.system !== undefined, "No system method");
        assert(log.exception !== undefined, "No exception method");
    });

    test("Creating logger with custom parameters", function(interceptor) {
        var log = uniset_log_create("custom", false, false, false);
        assert(log !== undefined, "Logger not created");
    });

    // Test 2: Logging levels
    test("Setting levels via string names", function(interceptor) {
        var log = uniset_log_create("levels-test", false, false, false, false);
        interceptor.clear();

        log.level("crit", "info", "init"); // Enable only crit, info and init

        log.crit("Critical message");
        log.warn("Warning"); // Should not output
        log.info("Informational message");
        log.init("Initialization message");
        log.log1("Level 1"); // Should not output
        log.system("System message"); // Should not output

        assert(interceptor.anyContains("Critical message"), "Crit message not output");
        assert(interceptor.anyContains("Informational message"), "Info message not output");
        assert(interceptor.anyContains("Initialization message"), "Init message not output");
        assert(!interceptor.anyContains("Warning"), "Warn message output when it shouldn't");
        assert(!interceptor.anyContains("Level 1"), "Level1 message output when it shouldn't");
        assert(!interceptor.anyContains("System message"), "System message output when it shouldn't");
    });

    test("Setting levels via bitmask", function(interceptor) {
        var log = uniset_log_create("mask-test", false, false, false, false);
        interceptor.clear();

        log.setLevelMask(LogLevels.WARN | LogLevels.LEVEL1 | LogLevels.SYSTEM);

        log.crit("Critical"); // Should not output
        log.warn("Warning");
        log.log1("Level 1");
        log.info("Information"); // Should not output
        log.system("System info"); // Should output
        log.repository("Repository"); // Should not output

        assert(!interceptor.anyContains("Critical"), "Crit output with mask without CRIT");
        assert(interceptor.anyContains("Warning"), "Warn not output");
        assert(interceptor.anyContains("Level 1"), "Level1 not output");
        assert(interceptor.anyContains("System info"), "System not output");
        assert(!interceptor.anyContains("Information"), "Info output with mask without INFO");
        assert(!interceptor.anyContains("Repository"), "Repository output with mask without REPOSITORY");
    });

    test("Enabling all levels", function(interceptor) {
        var log = uniset_log_create("all-levels", false, false, false, false);
        interceptor.clear();

        log.enableAll();

        log.crit("crit");
        log.warn("warn");
        log.info("info");
        log.init("init");
        log.log9("level9");
        log.system("system");
        log.repository("repository");
        log.exception("exception");

        assert(interceptor.anyContains("crit"), "Crit not output with enableAll");
        assert(interceptor.anyContains("warn"), "Warn not output with enableAll");
        assert(interceptor.anyContains("info"), "Info not output with enableAll");
        assert(interceptor.anyContains("init"), "Init not output with enableAll");
        assert(interceptor.anyContains("level9"), "Level9 not output with enableAll");
        assert(interceptor.anyContains("system"), "System not output with enableAll");
        assert(interceptor.anyContains("repository"), "Repository not output with enableAll");
        assert(interceptor.anyContains("exception"), "Exception not output with enableAll");
    });

    test("Disabling all levels", function(interceptor) {
        var log = uniset_log_create("no-levels", false, false, false, false);
        interceptor.clear();

        log.disableAll();

        log.crit("crit");
        log.warn("warn");
        log.info("info");
        log.init("init");
        log.system("system");

        assert(!interceptor.anyContains("crit"), "Crit output with disableAll");
        assert(!interceptor.anyContains("warn"), "Warn output with disableAll");
        assert(!interceptor.anyContains("info"), "Info output with disableAll");
        assert(!interceptor.anyContains("init"), "Init output with disableAll");
        assert(!interceptor.anyContains("system"), "System output with disableAll");
    });

    // Test 3: Checking level enabled status
    test("Checking isEnabled for levels", function(interceptor) {
        var log = uniset_log_create("enabled-test");
        log.level("warn", "level2", "system", "exception");

        assert(log.isEnabled("warn") === true, "Warn should be enabled");
        assert(log.isEnabled("level2") === true, "Level2 should be enabled");
        assert(log.isEnabled("system") === true, "System should be enabled");
        assert(log.isEnabled("exception") === true, "Exception should be enabled");
        assert(log.isEnabled("crit") === false, "Crit should be disabled");
        assert(log.isEnabled("info") === false, "Info should be disabled");
        assert(log.isEnabled("init") === false, "Init should be disabled");
        assert(log.isEnabled("repository") === false, "Repository should be disabled");
        assert(log.isEnabled("unknown") === false, "Unknown level should be false");
    });

    // Test 4: Message formatting
    test("Formatting with different data types", function(interceptor) {
        var log = uniset_log_create("format-test", false, false, false, false);
        interceptor.clear();
        log.level("info");

        log.info("String:", "test", "Number:", 42, "Object:", {key: "value"}, "Array:", [1, 2, 3]);

        var messages = interceptor.getAllMessages();
        var found = messages.some(function(msg) {
            return msg.indexOf("String: test") !== -1 &&
                msg.indexOf("Number: 42") !== -1;
        });
        assert(found, "Message doesn't contain formatted data");
    });

    test("Formatting with null and undefined", function(interceptor) {
        var log = uniset_log_create("null-test", false, false, false, false);
        interceptor.clear();
        log.level("info");

        log.info("Null:", null, "Undefined:", undefined);

        var messages = interceptor.getAllMessages();
        var found = messages.some(function(msg) {
            return msg.indexOf("Null: null") !== -1 &&
                msg.indexOf("Undefined: undefined") !== -1;
        });
        assert(found, "Message doesn't contain null/undefined");
    });

    // Test 5: Display settings
    test("Enabling/disabling date", function(interceptor) {
        var log = uniset_log_create("date-test", true, false, false, false);
        interceptor.clear();
        log.info("With date");

        var messages = interceptor.getAllMessages();
        assert(messages.length > 0, "No messages logged");
        var hasDate = messages[0].match(/\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}/);
        assert(hasDate !== null, "Date not displayed");

        log.showDate(false);
        interceptor.clear();
        log.info("Without date");

        messages = interceptor.getAllMessages();
        assert(messages.length > 0, "No messages logged after disabling date");
        hasDate = messages[0].match(/\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}/);
        assert(hasDate === null, "Date displayed after disabling");
    });

    test("Enabling/disabling logger name", function(interceptor) {
        var log = uniset_log_create("name-test", false, true, false, false);
        interceptor.clear();
        log.info("With name");

        assert(interceptor.anyContains("name-test"), "Logger name not displayed");

        log.showLogName(false);
        interceptor.clear();
        log.info("Without name");

        assert(!interceptor.anyContains("name-test"), "Logger name displayed after disabling");
    });

    test("Enabling/disabling emoji", function(interceptor) {
        var log = uniset_log_create("emoji-test", false, false, true, false);
        interceptor.clear();
        log.level("info", "init", "system", "exception");

        log.info("With emoji info");
        log.init("With emoji init");
        log.system("With emoji system");
        log.exception("With emoji exception");

        assert(interceptor.anyContains("🔵"), "Info emoji not displayed");
        assert(interceptor.anyContains("⚙️"), "Init emoji not displayed");
        assert(interceptor.anyContains("💻"), "System emoji not displayed");
        assert(interceptor.anyContains("🚨"), "Exception emoji not displayed");

        log.showEmoji(false);
        interceptor.clear();
        log.info("Without emoji");

        assert(!interceptor.anyContains("🔵"), "Emoji displayed after disabling");
    });

    // Test 6: Universal log function with new levels
    test("Universal log function with different levels", function(interceptor) {
        var log = uniset_log_create("universal-test", false, false, false, false);
        interceptor.clear();
        log.level("crit", "warn", "info", "init", "system");

        log.log("crit", "Critical via log");
        log.log("warn", "Warning via log");
        log.log("info", "Information via log");
        log.log("init", "Initialization via log");
        log.log("system", "System via log");
        log.log("level1", "Level1 via log"); // Should not output
        log.log("repository", "Repository via log"); // Should not output

        // Check that required messages exist and unwanted ones don't
        var hasCrit = interceptor.anyContains("Critical via log");
        var hasWarn = interceptor.anyContains("Warning via log");
        var hasInfo = interceptor.anyContains("Information via log");
        var hasInit = interceptor.anyContains("Initialization via log");
        var hasSystem = interceptor.anyContains("System via log");
        var hasLevel1 = interceptor.anyContains("Level1 via log");
        var hasRepository = interceptor.anyContains("Repository via log");

        assert(hasCrit, "Crit via log not working");
        assert(hasWarn, "Warn via log not working");
        assert(hasInfo, "Info via log not working");
        assert(hasInit, "Init via log not working");
        assert(hasSystem, "System via log not working");
        assert(!hasLevel1, "Level1 via log output when it shouldn't");
        assert(!hasRepository, "Repository via log output when it shouldn't");
    });

    // Test 7: Global functions with new levels
    test("Global logging functions", function(interceptor) {
        interceptor.clear();

        // Configure global logger - disable internal log for global logger during test
        defaultLogger.internallog(false);
        uniset_log("crit", "warn", "info", "init", "system");

        ucrit("Global critical");
        uwarn("Global warning");
        uinfo("Global information");
        uinit("Global initialization");
        usystem("Global system");
        ulog1("Global level 1"); // Should not output
        urepository("Global repository"); // Should not output

        assert(interceptor.anyContains("Global critical"), "Global crit not working");
        assert(interceptor.anyContains("Global warning"), "Global warn not working");
        assert(interceptor.anyContains("Global information"), "Global info not working");
        assert(interceptor.anyContains("Global initialization"), "Global init not working");
        assert(interceptor.anyContains("Global system"), "Global system not working");
        assert(!interceptor.anyContains("Global level 1"), "Global level1 output when it shouldn't");
        assert(!interceptor.anyContains("Global repository"), "Global repository output when it shouldn't");

        // Restore internal log for global logger
        defaultLogger.internallog(true);
    });

    test("Global uniset_log function without arguments", function(interceptor) {
        var currentLevel = uniset_log();
        assert(typeof currentLevel === 'number', "uniset_log() without arguments should return a number");
    });

    test("Managing emoji in global logger", function(interceptor) {
        uniset_log_show_emoji(true);
        assert(defaultLogger.showEmoji() === true, "Global emoji enable not working");

        uniset_log_show_emoji(false);
        assert(defaultLogger.showEmoji() === false, "Global emoji disable not working");
    });

    // Test 8: Alignment with new level names
    test("Setting width for alignment", function(interceptor) {
        var log = uniset_log_create("width-test", false, false, false, false);
        interceptor.clear();

        log.setWidths(12, 15); // Increase width for longer level names
        log.level("info", "repository", "exception");
        log.info("Alignment test");
        log.repository("Repository test");
        log.exception("Exception test");

        var messages = interceptor.getAllMessages();
        assert(messages.length > 0, "No messages logged");

        // Check that levels are properly aligned
        for (var i = 0; i < messages.length; i++) {
            var message = messages[i];
            var levelMatch = message.match(/\[(\w+)\]/);
            if (levelMatch) {
                var levelName = levelMatch[1];
                // Check that level name is properly formatted within brackets
                assert(levelName.length <= 10, "Level name too long: " + levelName);
            }
        }
    });

    // Test 9: Level constants - updated for new levels
    test("Logging level constants", function(interceptor) {
        assert(LogLevels.NONE === 0, "NONE should be 0");
        assert(LogLevels.INFO === 1, "INFO should be 1");
        assert(LogLevels.INIT === 2, "INIT should be 2");
        assert(LogLevels.WARN === 4, "WARN should be 4");
        assert(LogLevels.CRIT === 8, "CRIT should be 8");
        assert(LogLevels.LEVEL1 === 16, "LEVEL1 should be 16");
        assert(LogLevels.LEVEL2 === 32, "LEVEL2 should be 32");
        assert(LogLevels.LEVEL3 === 64, "LEVEL3 should be 64");
        assert(LogLevels.LEVEL4 === 128, "LEVEL4 should be 128");
        assert(LogLevels.LEVEL5 === 256, "LEVEL5 should be 256");
        assert(LogLevels.LEVEL6 === 512, "LEVEL6 should be 512");
        assert(LogLevels.LEVEL7 === 1024, "LEVEL7 should be 1024");
        assert(LogLevels.LEVEL8 === 2048, "LEVEL8 should be 2048");
        assert(LogLevels.LEVEL9 === 4096, "LEVEL9 should be 4096");
        assert(LogLevels.REPOSITORY === 8192, "REPOSITORY should be 8192");
        assert(LogLevels.SYSTEM === 16384, "SYSTEM should be 16384");
        assert(LogLevels.EXCEPTION === 32768, "EXCEPTION should be 32768");
        assert(LogLevels.ANY === 0xFFFF, "ANY should be 0xFFFF");
        assert(LogLevels.ALL === 0xFFFF, "ALL should be 0xFFFF (backward compatibility)");
    });

    // Test 10: Parallel usage of multiple loggers with new levels
    test("Independence of multiple loggers", function(interceptor) {
        var log1 = uniset_log_create("logger1", false, false, false, false);
        var log2 = uniset_log_create("logger2", false, false, false, false);
        interceptor.clear();

        log1.level("crit", "system");
        log2.level("info", "init", "repository");

        log1.info("Logger1 info"); // Should not output
        log1.system("Logger1 system"); // Should output
        log2.info("Logger2 info"); // Should output
        log2.init("Logger2 init"); // Should output
        log2.repository("Logger2 repository"); // Should output
        log2.system("Logger2 system"); // Should not output

        assert(!interceptor.anyContains("Logger1 info"), "Logger1 info output with disabled info");
        assert(interceptor.anyContains("Logger1 system"), "Logger1 system not output with enabled system");
        assert(interceptor.anyContains("Logger2 info"), "Logger2 info not output with enabled info");
        assert(interceptor.anyContains("Logger2 init"), "Logger2 init not output with enabled init");
        assert(interceptor.anyContains("Logger2 repository"), "Logger2 repository not output with enabled repository");
        assert(!interceptor.anyContains("Logger2 system"), "Logger2 system output with disabled system");
    });

    // Test 11: New level-specific methods
    test("New level-specific methods", function(interceptor) {
        var log = uniset_log_create("new-methods-test", false, false, false, false);
        interceptor.clear();
        log.level("info", "init", "repository", "system", "exception");

        log.info("Info message");
        log.init("Init message");
        log.repository("Repository message");
        log.system("System message");
        log.exception("Exception message");

        assert(interceptor.anyContains("Info message"), "info() method not working");
        assert(interceptor.anyContains("Init message"), "init() method not working");
        assert(interceptor.anyContains("Repository message"), "repository() method not working");
        assert(interceptor.anyContains("System message"), "system() method not working");
        assert(interceptor.anyContains("Exception message"), "exception() method not working");
    });

    // Output results
    console.log("\n=== Test Results ===");
    console.log("Passed: " + testResults.passed);
    console.log("Failed: " + testResults.failed);
    console.log("Total: " + (testResults.passed + testResults.failed));

    if (testResults.failed > 0) {
        console.log("\nFailed tests:");
        testResults.tests.forEach(function(t) {
            if (t.status === 'FAILED') {
                console.log("  ✗ " + t.name + " - " + t.error);
            }
        });
    }

    return testResults;
}

// Run tests on load
if (typeof module === 'undefined') {
    // If running directly in QuickJS
    var results = runLogTests();

    if (results.failed === 0) {
        console.log("\n🎉 All tests passed successfully!");
    } else {
        console.log("\n❌ There are failed tests!");
    }
}

// Export for use in other modules
if (typeof module !== 'undefined') {
    module.exports = {
        runLogTests: runLogTests,
        LogLevels: LogLevels,
        uniset_log_create: uniset_log_create
    };
}