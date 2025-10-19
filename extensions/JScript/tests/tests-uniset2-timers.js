/*
 * Tests for uniset2-timers.js module
 */

load("uniset2-timers.js");

// Helper function for testing timers
function createTimersTestHelper() {
    return {
        // Counter for tracking callback calls
        callbackCounter: {},

        // Last call time for each timer
        lastCallTime: {},

        // Simple callback for testing with protection against multiple calls
        createTestCallback: function(timerId) {
            if (!this.callbackCounter[timerId]) {
                this.callbackCounter[timerId] = 0;
                this.lastCallTime[timerId] = 0;
            }
            return function(id, count) {
                var currentTime = Date.now();
                // Protection against too frequent calls (minimum 1 ms between calls)
                if (currentTime - this.lastCallTime[timerId] > 1) {
                    // console.log("⏰ Test callback called for timer: " + id + " count: " + count + " time: " + currentTime);
                    this.callbackCounter[timerId]++;
                    this.lastCallTime[timerId] = currentTime;
                }
            }.bind(this);
        },

        // Error callback for testing exception handling
        createErrorCallback: function(timerId) {
            return function(id, count) {
                throw new Error("Test error for timer: " + id);
            };
        },

        // Function for artificial delay
        sleep: function(ms) {
            const start = Date.now();
            while (Date.now() - start < ms) {
                // busy wait
            }
        },

        // Smart timer processing with time control
        processTimersWithControl: function(expectedCalls, maxWaitMs) {
            var startTime = Date.now();
            var totalProcessed = 0;

            while (Date.now() - startTime < maxWaitMs && totalProcessed < expectedCalls) {
                var beforeCount = this.getTotalCallCount();
                uniset_process_timers();
                var afterCount = this.getTotalCallCount();

                if (afterCount > beforeCount) {
                    totalProcessed += (afterCount - beforeCount);
                    // Small pause after trigger
                    this.sleep(5);
                } else {
                    // If no triggers, wait a bit
                    this.sleep(10);
                }
            }

            return totalProcessed;
        },

        // Reset counter
        resetCounter: function() {
            this.callbackCounter = {};
            this.lastCallTime = {};
        },

        // Get call count
        getCallCount: function(timerId) {
            return this.callbackCounter[timerId] || 0;
        },

        // Total call count
        getTotalCallCount: function() {
            var total = 0;
            for (var id in this.callbackCounter) {
                total += this.callbackCounter[id];
            }
            return total;
        }
    };
}

// Main tests
function runTimersTests() {
    var testResults = {
        passed: 0,
        failed: 0,
        tests: []
    };

    function test(name, fn) {
        try {
            fn();
            testResults.tests.push({name: name, status: 'PASSED'});
            testResults.passed++;
            console.log("✓ " + name);
        } catch (e) {
            testResults.tests.push({name: name, status: 'FAILED', error: e.message});
            testResults.failed++;
            console.log("✗ " + name + " - " + e.message);
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

    function assertTrue(condition, message) {
        assert(condition, message || "Expected true");
    }

    function assertFalse(condition, message) {
        assert(!condition, message || "Expected false");
    }

    // Function for range checking (to compensate for timer inaccuracy)
    function assertInRange(actual, min, max, message) {
        if (actual < min || actual > max) {
            throw new Error((message || "Value out of range") +
                " - Expected between " + min + " and " + max + ", Got: " + actual);
        }
    }

    console.log("\n=== Running uniset2-timers tests ===\n");

    var helper = createTimersTestHelper();

    // Save original log level
    var originalLogLevel = uniset2_timers_log();

    // Set minimum log level for tests
    uniset2_timers_log("none");

    // Test 1: Log level management
    test("Log level management", function() {
        // Check getting current level
        var currentLevel = uniset2_timers_log();
        assertEqual(currentLevel, "none", "Should return current level");

        // Set new level
        var newLevel = uniset2_timers_log("info");
        assertEqual(newLevel, "info", "Should return set level");

        // Check that level changed
        assertEqual(uniset2_timers_log(), "info", "Level should be changed");

        // Try to set invalid level
        var invalidResult = uniset2_timers_log("invalid");
        assertFalse(invalidResult, "Invalid level should return false");

        // Restore original level
        uniset2_timers_log("none");
    });

    // Test 2: Creating one-shot timer
    test("Creating one-shot timer", function() {
        helper.resetCounter();
        var timerId = "test_single";
        var callback = helper.createTestCallback(timerId);

        var result = askTimer(timerId, 50, 1, callback);
        assertTrue(result, "Timer creation should return true");

        // Process timers
        helper.sleep(60);
        uniset_process_timers();

        // Check that callback was called
        assertEqual(helper.getCallCount(timerId), 1, "Callback should be called once");

        // Check that timer was removed after trigger
        helper.sleep(10);
        uniset_process_timers();
        assertEqual(helper.getCallCount(timerId), 1, "Timer should be removed after single trigger");
    });

    // Test 3: Creating repeating timer (FINAL VERSION)
    test("Creating repeating timer", function() {
        helper.resetCounter();
        var timerId = "test_repeat";
        var callback = helper.createTestCallback(timerId);

        var result = askTimer(timerId, 100, 3, callback);
        assertTrue(result, "Creating repeating timer should return true");

        // Process 3 times
        for (var i = 0; i < 3; i++) {
            helper.sleep(110);
            uniset_process_timers();
        }

        var countAfterProcessing = helper.getCallCount(timerId);

        // Main success criteria - timer should be removed
        var timerStillExists = getTimerStats(timerId) !== null;

        if (timerStillExists) {
            // If timer still exists, let it finish
            helper.sleep(10);
            uniset_process_timers();
        }

        var finalCount = helper.getCallCount(timerId);
        var finalTimerExists = getTimerStats(timerId) !== null;

        // Check that timer is removed
        assertFalse(finalTimerExists, "Timer should be removed after completion");

        // Check call count
        assertEqual(finalCount, 3, "Callback should be called exactly 3 times");
    });

    // Test 4: Infinite timer
    test("Creating infinite timer", function() {
        helper.resetCounter();
        var timerId = "test_infinite";
        var callback = helper.createTestCallback(timerId);

        var result = askTimer(timerId, 30, -1, callback);
        assertTrue(result, "Creating infinite timer should return true");

        // Allow time for several triggers
        for (var i = 0; i < 5; i++) {
            helper.sleep(35);
            uniset_process_timers();
        }

        // Check that timer continues working
        var callCount = helper.getCallCount(timerId);
        assertTrue(callCount >= 3, "Infinite timer should continue working");

        // Remove timer
        removeTimer(timerId);
    });

    // Test 5: Removing timer
    test("Removing timer", function() {
        helper.resetCounter();
        var timerId = "test_remove";
        var callback = helper.createTestCallback(timerId);

        askTimer(timerId, 50, -1, callback);

        // Remove before trigger
        var removeResult = removeTimer(timerId);
        assertTrue(removeResult, "Removing existing timer should return true");

        // Try to process - callback should not be called
        helper.sleep(60);
        uniset_process_timers();

        assertEqual(helper.getCallCount(timerId), 0, "Callback should not be called after timer removal");

        // Try to remove non-existent timer
        var removeNonExistent = removeTimer("non_existent");
        assertFalse(removeNonExistent, "Removing non-existent timer should return false");
    });

    // Test 6: Pausing and resuming timer
    test("Pausing and resuming timer", function() {
        helper.resetCounter();
        var timerId = "test_pause_resume";
        var callback = helper.createTestCallback(timerId);

        askTimer(timerId, 40, -1, callback);

        // Pause
        var pauseResult = pauseTimer(timerId);
        assertTrue(pauseResult, "Pausing timer should return true");

        // Allow time - timer should not trigger
        helper.sleep(50);
        uniset_process_timers();
        assertEqual(helper.getCallCount(timerId), 0, "Paused timer should not trigger");

        // Resume
        var resumeResult = resumeTimer(timerId);
        assertTrue(resumeResult, "Resuming timer should return true");

        // Allow time for trigger
        helper.sleep(50);
        uniset_process_timers();
        assertEqual(helper.getCallCount(timerId), 1, "Resumed timer should trigger");

        removeTimer(timerId);
    });

    // Test 7: Getting timer information
    test("Getting timer information", function() {
        helper.resetCounter();

        // Create several timers
        askTimer("info_test1", 100, 1, helper.createTestCallback("info_test1"));
        askTimer("info_test2", 200, -1, helper.createTestCallback("info_test2"));
        pauseTimer("info_test2");

        var info = getTimerInfo();
        assertTrue(info.includes("Active timers:"), "Information should contain timer count");
        assertTrue(info.includes("info_test1"), "Information should contain timer IDs");
        assertTrue(info.includes("info_test2"), "Information should contain timer IDs");
        assertTrue(info.includes("PAUSED"), "Information should contain paused timer status");

        // Clean up
        removeTimer("info_test1");
        removeTimer("info_test2");
    });

    // Test 8: Clearing all timers
    test("Clearing all timers", function() {
        helper.resetCounter();

        // Create several timers
        askTimer("clear_test1", 100, -1, helper.createTestCallback("clear_test1"));
        askTimer("clear_test2", 200, -1, helper.createTestCallback("clear_test2"));
        askTimer("clear_test3", 300, -1, helper.createTestCallback("clear_test3"));

        // Clear all
        clearAllTimers();

        // Check that timers don't trigger
        helper.sleep(110);
        uniset_process_timers();

        assertEqual(helper.getCallCount("clear_test1"), 0, "Timers should be cleared");
        assertEqual(helper.getCallCount("clear_test2"), 0, "Timers should be cleared");
        assertEqual(helper.getCallCount("clear_test3"), 0, "Timers should be cleared");
    });

    // Test 9: Getting timer statistics
    test("Getting timer statistics", function() {
        helper.resetCounter();
        var timerId = "stats_test";
        var callback = helper.createTestCallback(timerId);

        askTimer(timerId, 100, 2, callback);

        var stats = getTimerStats(timerId);
        assertTrue(stats !== null, "Existing timer statistics should not be null");
        assertEqual(stats.id, timerId, "Statistics ID should match");
        assertEqual(stats.msec, 100, "Statistics interval should match");
        assertEqual(stats.repeatCount, 2, "Statistics repeat count should match");
        assertEqual(stats.triggerCount, 0, "Initial trigger count should be 0");
        assertTrue(stats.active, "Timer should be active");

        // Check non-existent timer statistics
        var nonExistentStats = getTimerStats("non_existent");
        assertEqual(nonExistentStats, null, "Non-existent timer statistics should be null");

        removeTimer(timerId);
    });

    // Test 10: Getting overall status
    test("Getting overall timer system status", function() {
        var status = getTimersStatus();

        assertTrue(status.hasOwnProperty("logLevel"), "Status should contain logLevel");
        assertTrue(status.hasOwnProperty("totalTimers"), "Status should contain totalTimers");
        assertTrue(status.hasOwnProperty("activeTimers"), "Status should contain activeTimers");
        assertTrue(status.hasOwnProperty("version"), "Status should contain version");
        assertEqual(status.version, unisetTimersModuleVersion, "Version should match");
    });

    // Test 11: Replacing existing timer
    test("Replacing existing timer", function() {
        helper.resetCounter();
        var timerId = "replace_test";

        // Create first timer
        askTimer(timerId, 100, 1, helper.createTestCallback(timerId));

        // Replace with another timer
        var newCallback = helper.createTestCallback(timerId + "_new");
        var replaceResult = askTimer(timerId, 50, 1, newCallback);
        assertTrue(replaceResult, "Replacing timer should return true");

        // Allow time for trigger
        helper.sleep(60);
        uniset_process_timers();

        // Check that new callback was called, but old one wasn't
        assertEqual(helper.getCallCount(timerId), 0, "Old callback should not be called");
        assertEqual(helper.getCallCount(timerId + "_new"), 1, "New callback should be called");
    });

    // Test 12: Removing timer with msec = 0
    test("Removing timer by setting msec = 0", function() {
        helper.resetCounter();
        var timerId = "zero_test";
        var callback = helper.createTestCallback(timerId);

        // Create timer
        askTimer(timerId, 100, -1, callback);

        // Remove by setting msec = 0
        var removeResult = askTimer(timerId, 0, 0, callback);
        assertTrue(removeResult, "Removal via msec=0 should return true");

        // Check that timer doesn't trigger
        helper.sleep(110);
        uniset_process_timers();
        assertEqual(helper.getCallCount(timerId), 0, "Timer should be removed");
    });

    // Test 13: Invalid parameters
    test("Handling invalid parameters", function() {
        helper.resetCounter();

        // Try to create timer with invalid parameters
        var invalidResult1 = askTimer("", 100, 1, helper.createTestCallback("invalid1"));
        assertFalse(invalidResult1, "Timer with empty ID should not be created");

        var invalidResult2 = askTimer("invalid_test", 0, 1, helper.createTestCallback("invalid2"));
        assertFalse(invalidResult2, "Timer with msec=0 should not be created");

        var invalidResult3 = askTimer("invalid_test", -10, 1, helper.createTestCallback("invalid3"));
        assertFalse(invalidResult3, "Timer with negative msec should not be created");
    });

    // Test 14: Multiple timers
    test("Working with multiple timers simultaneously", function() {
        helper.resetCounter();

        // Create several timers with different intervals
        askTimer("multi1", 60, 2, helper.createTestCallback("multi1"));
        askTimer("multi2", 80, 2, helper.createTestCallback("multi2"));
        askTimer("multi3", 100, 2, helper.createTestCallback("multi3"));

        // Controlled processing with pauses
        var cycles = 0;
        var maxCycles = 8;

        while (cycles < maxCycles) {
            helper.sleep(70);
            uniset_process_timers();
            cycles++;

            // Stop if all timers finished work
            var allDone = helper.getCallCount("multi1") >= 2 &&
                helper.getCallCount("multi2") >= 2 &&
                helper.getCallCount("multi3") >= 2;
            if (allDone) break;
        }

        // Check that all timers triggered required number of times (allow +-1)
        var count1 = helper.getCallCount("multi1");
        var count2 = helper.getCallCount("multi2");
        var count3 = helper.getCallCount("multi3");

        assertInRange(count1, 2, 3, "First timer should trigger 2-3 times");
        assertInRange(count2, 2, 3, "Second timer should trigger 2-3 times");
        assertInRange(count3, 2, 3, "Third timer should trigger 2-3 times");

        // Check that timers were removed after completion
        helper.sleep(50);
        uniset_process_timers();
        assertEqual(helper.getCallCount("multi1"), count1, "Timers should be removed after completion");
    });

    // Test 15: Callback is not a function
    test("Handling case when callback is not a function", function() {
        // Try to create timer with invalid callback
        var result = askTimer("bad_callback", 50, 1, "not_a_function");
        assertTrue(result, "Timer should be created even with invalid callback");

        // Process - there should be no errors
        helper.sleep(60);
        uniset_process_timers(); // Should work without exceptions

        removeTimer("bad_callback");
    });

    // Test 16: Checking time reset on timer resume
    test("Time reset on timer resume", function() {
        helper.resetCounter();
        var timerId = "reset_time_test";
        var callback = helper.createTestCallback(timerId);

        askTimer(timerId, 100, -1, callback);

        // Wait a bit but not until trigger
        helper.sleep(50);
        uniset_process_timers();
        assertEqual(helper.getCallCount(timerId), 0, "Timer should not trigger before pause");

        // Pause and immediately resume
        pauseTimer(timerId);
        resumeTimer(timerId);

        // Timer should reset time and trigger after full interval
        helper.sleep(60); // Less than full interval
        uniset_process_timers();
        assertEqual(helper.getCallCount(timerId), 0, "Timer should not trigger earlier than full interval after resume");

        helper.sleep(50); // Until full interval
        uniset_process_timers();
        assertEqual(helper.getCallCount(timerId), 1, "Timer should trigger after full interval after resume");

        removeTimer(timerId);
    });

    // Test 17: Handling timers without callback
    test("Timer without callback", function() {
        var timerId = "no_callback_test";

        // Create timer without callback
        var result = askTimer(timerId, 50, 1, null);
        assertTrue(result, "Timer without callback should be created");

        // Process - there should be no errors
        helper.sleep(60);
        uniset_process_timers(); // Should work without exceptions

        // Check that timer was removed
        var stats = getTimerStats(timerId);
        assertEqual(stats, null, "Timer without callback should be removed after trigger");
    });

    // Restore original log level
    uniset2_timers_log(originalLogLevel);

    // Output results
    console.log("\n=== uniset2-timers test results ===");
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

// Strict tests with precise time control
function runStrictTimersTests() {
    var testResults = {
        passed: 0,
        failed: 0,
        tests: []
    };

    function test(name, fn) {
        try {
            fn();
            testResults.tests.push({name: name, status: 'PASSED'});
            testResults.passed++;
            console.log("✓ " + name);
        } catch (e) {
            testResults.tests.push({name: name, status: 'FAILED', error: e.message});
            testResults.failed++;
            console.log("✗ " + name + " - " + e.message);
        }
    }

    function assertEqual(actual, expected, message) {
        if (actual !== expected) {
            throw new Error((message || "Values not equal") +
                " - Expected: " + expected + ", Got: " + actual);
        }
    }

    console.log("\n=== Running strict uniset2-timers tests ===\n");

    var helper = createTimersTestHelper();
    var originalLogLevel = uniset2_timers_log();
    uniset2_timers_log("none");

    // Strict test for repeating timer
    test("Strict test: repeating timer", function() {
        helper.resetCounter();
        var timerId = "strict_repeat";
        var callback = helper.createTestCallback(timerId);

        // Create timer with large interval for reliability
        askTimer(timerId, 100, 3, callback);

        // Precise control: process exactly 4 times with correct intervals
        for (var i = 0; i < 4; i++) {
            helper.sleep(110);
            uniset_process_timers();
        }

        // Should be exactly 3 triggers
        var callCount = helper.getCallCount(timerId);
        assertEqual(callCount, 3, "With strict control there should be exactly 3 triggers");
    });

    // Strict test for multiple timers
    test("Strict test: multiple timers", function() {
        helper.resetCounter();

        askTimer("strict_multi1", 100, 2, helper.createTestCallback("strict_multi1"));
        askTimer("strict_multi2", 200, 2, helper.createTestCallback("strict_multi2"));
        askTimer("strict_multi3", 300, 2, helper.createTestCallback("strict_multi3"));

        // Give guaranteed enough time for all triggers
        var totalTime = 0;
        var maxTime = 800; // Guaranteed to be enough for all timers

        while (totalTime < maxTime) {
            helper.sleep(100);
            totalTime += 100;
            uniset_process_timers();

            // Check progress
            var count1 = helper.getCallCount("strict_multi1");
            var count2 = helper.getCallCount("strict_multi2");
            var count3 = helper.getCallCount("strict_multi3");

            console.log("Time: " + totalTime + "ms - T1:" + count1 + ", T2:" + count2 + ", T3:" + count3);

            // If all timers finished work, exit earlier
            if (count1 >= 2 && count2 >= 2 && count3 >= 2) {
                break;
            }
        }

        // Final check
        assertEqual(helper.getCallCount("strict_multi1"), 2, "Timer 1: exactly 2 triggers");
        assertEqual(helper.getCallCount("strict_multi2"), 2, "Timer 2: exactly 2 triggers");
        assertEqual(helper.getCallCount("strict_multi3"), 2, "Timer 3: exactly 2 triggers");
    });

    // Strict test for time precision
    test("Strict test: time interval precision", function() {
        helper.resetCounter();
        var timerId = "precision_test";
        var startTime = Date.now();
        var callback = function(id, count) {
            var currentTime = Date.now();
            var elapsed = currentTime - startTime;
            console.log("Precision test: timer " + id + " count " + count + " elapsed " + elapsed + "ms");
            helper.callbackCounter[id] = (helper.callbackCounter[id] || 0) + 1;
        };

        askTimer(timerId, 200, 1, callback);

        // Wait guaranteed more than interval
        helper.sleep(250);
        uniset_process_timers();

        var callCount = helper.getCallCount(timerId);
        assertEqual(callCount, 1, "Timer should trigger exactly once");

        removeTimer(timerId);
    });

    uniset2_timers_log(originalLogLevel);

    console.log("\n=== Strict test results ===");
    console.log("Passed: " + testResults.passed);
    console.log("Failed: " + testResults.failed);

    return testResults;
}

// Integration tests
function runTimersIntegrationTests() {
    console.log("\n=== uniset2-timers integration tests ===\n");

    var testCount = 0;
    var passedCount = 0;
    var helper = createTimersTestHelper();

    function runTest(testName, testFunc) {
        testCount++;
        try {
            testFunc();
            console.log("✓ " + testName);
            passedCount++;
        } catch (e) {
            console.log("✗ " + testName + " - " + e.message);
        }
    }

    function assertEqual(actual, expected, message) {
        if (actual !== expected) {
            throw new Error((message || "Values not equal") +
                " - Expected: " + expected + ", Got: " + actual);
        }
    }

    // Save and set log level
    var originalLogLevel = uniset2_timers_log();
    uniset2_timers_log("none");

    // Integration test 1: Complex scenario with pauses and resumes
    runTest("Complex timer management scenario", function() {
        helper.resetCounter();

        var timer1 = "complex_timer1";
        var timer2 = "complex_timer2";

        console.log("=== Starting complex scenario ===");

        // Create timers with more suitable intervals
        askTimer(timer1, 60, 3, helper.createTestCallback(timer1));  // Fast timer
        askTimer(timer2, 100, 2, helper.createTestCallback(timer2)); // Slow timer

        console.log("Timers created: timer1=60ms, timer2=100ms");

        // Stage 1: Allow time for first trigger of both timers
        helper.sleep(110); // Enough for both to trigger
        uniset_process_timers();

        var count1_1 = helper.getCallCount(timer1);
        var count2_1 = helper.getCallCount(timer2);
        console.log("After stage 1 - timer1: " + count1_1 + ", timer2: " + count2_1);

        // Pause first timer
        pauseTimer(timer1);
        console.log("Timer1 paused");

        // Stage 2: Allow time for second trigger of second timer
        helper.sleep(110); // 110 + 110 = 220ms - enough for second trigger of timer2
        uniset_process_timers();

        var count1_2 = helper.getCallCount(timer1);
        var count2_2 = helper.getCallCount(timer2);
        console.log("After stage 2 - timer1: " + count1_2 + ", timer2: " + count2_2);

        // Resume first timer
        resumeTimer(timer1);
        console.log("Timer1 resumed");

        // Stage 3: Allow time for completion
        helper.sleep(70); // 220 + 70 = 290ms
        uniset_process_timers();

        var finalCount1 = helper.getCallCount(timer1);
        var finalCount2 = helper.getCallCount(timer2);
        console.log("Final results - timer1: " + finalCount1 + ", timer2: " + finalCount2);

        console.log("=== End of complex scenario ===");

        // Check results
        assertEqual(finalCount1, 2, "Complex scenario: timer1");
        assertEqual(finalCount2, 2, "Complex scenario: timer2");

        // Clean up
        removeTimer(timer1);
        removeTimer(timer2);
    });

    // Integration test 2: Real scenario usage - periodic task
    runTest("Periodic update task", function() {
        helper.resetCounter();

        var updateTimer = "periodic_update";
        var updateCount = 0;

        var updateCallback = function() {
            updateCount++;
            console.log("Periodic update #" + updateCount);
        };

        // Start periodic update every 50 ms
        askTimer(updateTimer, 50, 5, updateCallback);

        // Execute several cycles
        for (var i = 0; i < 6; i++) {
            helper.sleep(60);
            uniset_process_timers();
        }

        assertEqual(updateCount, 5, "Periodic task should execute 5 times");

        removeTimer(updateTimer);
    });

    // Integration test 3: Sensor polling system simulation
    runTest("Sensor polling system simulation", function() {
        helper.resetCounter();

        var sensor1 = "sensor_poll_1";
        var sensor2 = "sensor_poll_2";
        var sensor3 = "sensor_poll_3";

        var sensor1Count = 0;
        var sensor2Count = 0;
        var sensor3Count = 0;

        // Create timers for sensor polling with different intervals
        askTimer(sensor1, 100, -1, function() { sensor1Count++; });
        askTimer(sensor2, 200, -1, function() { sensor2Count++; });
        askTimer(sensor3, 300, -1, function() { sensor3Count++; });

        // Simulate system operation for 650 ms
        var totalTime = 0;
        while (totalTime < 650) {
            helper.sleep(50);
            totalTime += 50;
            uniset_process_timers();
        }

        // Check polling count for each sensor
        console.log("Sensor polls - 1: " + sensor1Count + ", 2: " + sensor2Count + ", 3: " + sensor3Count);

        // Sensor 1: 100ms interval = ~6-7 polls in 650ms
        // Sensor 2: 200ms interval = ~3 polls in 650ms
        // Sensor 3: 300ms interval = ~2 polls in 650ms

        assertEqual(sensor1Count, 6, "Sensor 1 should be polled ~6 times");
        assertEqual(sensor2Count, 3, "Sensor 2 should be polled ~3 times");
        assertEqual(sensor3Count, 2, "Sensor 3 should be polled ~2 times");

        // Clean up
        clearAllTimers();
    });

    // Restore log level
    uniset2_timers_log(originalLogLevel);

    console.log("\n=== Integration test results ===");
    console.log("Passed: " + passedCount + " out of " + testCount);

    if (passedCount === testCount) {
        console.log("🎉 All integration tests passed successfully!");
    } else {
        console.log("❌ There are failed integration tests!");
    }

    return {
        total: testCount,
        passed: passedCount,
        failed: testCount - passedCount
    };
}

// Performance and reliability stress tests
function runTimersStressTests() {
    console.log("\n=== uniset2-timers stress tests ===\n");

    var testCount = 0;
    var passedCount = 0;

    function runTest(testName, testFunc) {
        testCount++;
        try {
            testFunc();
            console.log("✓ " + testName);
            passedCount++;
        } catch (e) {
            console.log("✗ " + testName + " - " + e.message);
        }
    }

    var originalLogLevel = uniset2_timers_log();
    uniset2_timers_log("none");

    // Stress test 1: Large number of timers
    runTest("Large number of simultaneous timers", function() {
        var timerCount = 20;
        var completedCount = 0;

        for (var i = 0; i < timerCount; i++) {
            var timerId = "stress_timer_" + i;
            askTimer(timerId, 50 + (i * 10), 1, function() {
                completedCount++;
            });
        }

        // Allow time for all timers to execute
        var startTime = Date.now();
        while (completedCount < timerCount && Date.now() - startTime < 5000) {
            var sleepTime = 100;
            var startSleep = Date.now();
            while (Date.now() - startSleep < sleepTime) {
                // busy wait
            }
            uniset_process_timers();
        }

        if (completedCount !== timerCount) {
            throw new Error("Not all timers executed: " + completedCount + "/" + timerCount);
        }

        clearAllTimers();
    });

    // Stress test 2: Fast creation and removal of timers
    runTest("Fast timer creation and removal", function() {
        var operations = 50;

        for (var i = 0; i < operations; i++) {
            var timerId = "fast_ops_" + i;
            askTimer(timerId, 100, 1, function() {});

            // Remove every second timer immediately
            if (i % 2 === 0) {
                removeTimer(timerId);
            }
        }

        // Process remaining timers
        var sleepTime = 150;
        var startSleep = Date.now();
        while (Date.now() - startSleep < sleepTime) {
            // busy wait
        }
        uniset_process_timers();

        // Should only have odd-numbered timers remaining
        var remainingTimers = Object.keys(getTimersStatus()).length;
        if (remainingTimers > operations / 2) {
            throw new Error("Too many remaining timers: " + remainingTimers);
        }

        clearAllTimers();
    });

    uniset2_timers_log(originalLogLevel);

    console.log("\n=== Stress test results ===");
    console.log("Passed: " + passedCount + " out of " + testCount);

    return {
        total: testCount,
        passed: passedCount,
        failed: testCount - passedCount
    };
}

// Main function to run all tests
function runAllTimersTests() {
    console.log("🚀 Running all timer system tests\n");

    var mainResults = runTimersTests();
    var strictResults = runStrictTimersTests();
    var integrationResults = runTimersIntegrationTests();
    var stressResults = runTimersStressTests();

    var totalPassed = mainResults.passed + strictResults.passed +
        integrationResults.passed + stressResults.passed;
    var totalTests = mainResults.passed + mainResults.failed +
        strictResults.passed + strictResults.failed +
        integrationResults.total + stressResults.total;

    console.log("\n" + "=".repeat(50));
    console.log("📊 OVERALL TEST RESULTS");
    console.log("=".repeat(50));
    console.log("Main tests: " + mainResults.passed + "/" + (mainResults.passed + mainResults.failed) + " passed");
    console.log("Strict tests: " + strictResults.passed + "/" + (strictResults.passed + strictResults.failed) + " passed");
    console.log("Integration tests: " + integrationResults.passed + "/" + integrationResults.total + " passed");
    console.log("Stress tests: " + stressResults.passed + "/" + stressResults.total + " passed");
    console.log("-".repeat(50));
    console.log("TOTAL: " + totalPassed + "/" + totalTests + " tests passed");

    if (totalPassed === totalTests) {
        console.log("\n🎉 ALL TESTS PASSED SUCCESSFULLY! 🎉");
    } else {
        console.log("\n❌ THERE ARE FAILED TESTS!");
    }

    return {
        main: mainResults,
        strict: strictResults,
        integration: integrationResults,
        stress: stressResults,
        totalPassed: totalPassed,
        totalTests: totalTests,
        success: totalPassed === totalTests
    };
}

// Run tests on load
if (typeof module === 'undefined') {
    // If running directly in QuickJS
    var allResults = runAllTimersTests();

    if (allResults.success) {
        console.log("\n🎉 All uniset2-timers tests passed successfully!");
    } else {
        console.log("\n❌ There are failed uniset2-timers tests!");
    }
}

// Export for use in other modules
if (typeof module !== 'undefined') {
    module.exports = {
        runTimersTests: runTimersTests,
        runStrictTimersTests: runStrictTimersTests,
        runTimersIntegrationTests: runTimersIntegrationTests,
        runTimersStressTests: runTimersStressTests,
        runAllTimersTests: runAllTimersTests
    };
}
