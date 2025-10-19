/*
 * Tests for DelayTimer module
 */

load("uniset2-delaytimer.js");

// Helper function for testing delay timers
function createDelayTimerTestHelper() {
    return {
        // Function for artificial delay
        sleep: function(ms) {
            const start = Date.now();
            while (Date.now() - start < ms) {
                // busy wait
            }
        },

        // Check if value is within acceptable range
        isInRange: function(actual, expected, tolerance) {
            return Math.abs(actual - expected) <= tolerance;
        },

        // Testing delay timer state
        testDelayState: function(timer, expectedState, expectedWaitingOn, expectedWaitingOff, testName) {
            const state = timer.get();
            const waitingOn = timer.isWaitingOn();
            const waitingOff = timer.isWaitingOff();
            const waiting = timer.isWaiting();

            if (state !== expectedState ||
                waitingOn !== expectedWaitingOn ||
                waitingOff !== expectedWaitingOff) {
                throw new Error(testName + " - State: expected state=" + expectedState +
                    " waitingOn=" + expectedWaitingOn + " waitingOff=" + expectedWaitingOff +
                    ", got state=" + state + " waitingOn=" + waitingOn + " waitingOff=" + waitingOff);
            }

            // Check general waiting flag
            const expectedWaiting = expectedWaitingOn || expectedWaitingOff;
            if (waiting !== expectedWaiting) {
                throw new Error(testName + " - isWaiting(): expected " + expectedWaiting +
                    ", got " + waiting);
            }
        }
    };
}

// Main tests
function runDelayTimerTests() {
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

    console.log("\n=== Running DelayTimer Tests ===\n");

    var helper = createDelayTimerTestHelper();

    // Test 1: Timer creation
    test("Creating timer with default parameters", function() {
        var timer = new DelayTimer();
        assert(timer !== undefined, "Timer not created");
        assertEqual(timer.getOnDelay(), 0, "Default activation delay should be 0");
        assertEqual(timer.getOffDelay(), 0, "Default deactivation delay should be 0");
        assertEqual(timer.get(), false, "Initial state should be false");
    });

    test("Creating timer with specified delays", function() {
        var timer = new DelayTimer(100, 50);
        assertEqual(timer.getOnDelay(), 100, "Activation delay should be 100 ms");
        assertEqual(timer.getOffDelay(), 50, "Deactivation delay should be 50 ms");
    });

    // Test 2: Setting parameters
    test("Setting parameters via set()", function() {
        var timer = new DelayTimer(100, 50);
        timer.set(200, 150);
        assertEqual(timer.getOnDelay(), 200, "Activation delay should be changed to 200 ms");
        assertEqual(timer.getOffDelay(), 150, "Deactivation delay should be changed to 150 ms");

        // Check that state is reset
        helper.testDelayState(timer, false, false, false, "After set()");
    });

    // Test 3: Timer reset
    test("Resetting timer via reset()", function() {
        var timer = new DelayTimer(50, 50);

        // Set to waiting state
        timer.check(true);
        helper.testDelayState(timer, false, true, false, "After check(true)");

        // Reset
        timer.reset();
        helper.testDelayState(timer, false, false, false, "After reset()");

        // Additional check: call get() after reset()
        var stateAfterGet = timer.get();
        assertEqual(stateAfterGet, false, "After reset() get() should return false");
        helper.testDelayState(timer, false, false, false, "After reset() and get()");

        // Check that timer is really reset and not trying to activate
        helper.sleep(60);
        var finalState = timer.get();
        assertEqual(finalState, false, "After reset() and waiting, state should remain false");
        helper.testDelayState(timer, false, false, false, "After reset() and waiting");
    });

    // Test 4: Activation delay
    test("Activation delay - normal operation", function() {
        var timer = new DelayTimer(100, 0);

        // Activate - should go to activation waiting state
        var result1 = timer.check(true);
        assertEqual(result1, false, "Immediately after activation state should be false");
        helper.testDelayState(timer, false, true, false, "After activation");

        // Wait less than delay time
        helper.sleep(50);
        var result2 = timer.check(true);
        assertEqual(result2, false, "Before delay expiration state should remain false");
        helper.testDelayState(timer, false, true, false, "During activation delay");

        // Wait full delay time
        helper.sleep(60);
        var result3 = timer.check(true);
        assertEqual(result3, true, "After delay expiration state should become true");
        helper.testDelayState(timer, true, false, false, "After activation delay completion");
    });

    // Test 5: Deactivation delay
    test("Deactivation delay - normal operation", function() {
        var timer = new DelayTimer(0, 100); // Only deactivation delay

        // Activate immediately (since activation delay is 0)
        var result1 = timer.check(true);
        assertEqual(result1, true, "Immediately after activation state should be true");
        helper.testDelayState(timer, true, false, false, "After activation");

        // Deactivate - should go to deactivation waiting state
        var result2 = timer.check(false);
        assertEqual(result2, true, "Immediately after deactivation state should remain true");
        helper.testDelayState(timer, true, false, true, "After deactivation");

        // Wait less than delay time
        helper.sleep(50);
        var result3 = timer.check(false);
        assertEqual(result3, true, "Before delay expiration state should remain true");
        helper.testDelayState(timer, true, false, true, "During deactivation delay");

        // Wait full delay time
        helper.sleep(60);
        var result4 = timer.check(false);
        assertEqual(result4, false, "After delay expiration state should become false");
        helper.testDelayState(timer, false, false, false, "After deactivation delay completion");
    });

    // Test 6: Interrupting activation delay
    test("Interrupting activation delay", function() {
        var timer = new DelayTimer(100, 0);

        // Start activation
        timer.check(true);
        helper.testDelayState(timer, false, true, false, "After starting activation");

        // Interrupt before time expiration
        helper.sleep(50);
        var result = timer.check(false); // Interrupt signal
        assertEqual(result, false, "After interruption state should remain false");
        helper.testDelayState(timer, false, false, false, "After interrupting activation");

        // Check that timer really reset
        helper.sleep(60);
        var finalResult = timer.check(false);
        assertEqual(finalResult, false, "After interruption state should not change");
    });

    // Test 7: Interrupting deactivation delay
    test("Interrupting deactivation delay", function() {
        var timer = new DelayTimer(0, 100);

        // Activate
        timer.check(true);
        helper.testDelayState(timer, true, false, false, "After activation");

        // Start deactivation
        timer.check(false);
        helper.testDelayState(timer, true, false, true, "After starting deactivation");

        // Interrupt before time expiration
        helper.sleep(50);
        var result = timer.check(true); // Activate again
        assertEqual(result, true, "After interruption state should remain true");
        helper.testDelayState(timer, true, false, false, "After interrupting deactivation");

        // Check that timer really reset
        helper.sleep(60);
        var finalResult = timer.check(true);
        assertEqual(finalResult, true, "After interruption state should not change");
    });

    // Test 8: Combined operation of both delays
    test("Combined operation of both delays", function() {
        var timer = new DelayTimer(80, 60);

        // Activation test
        timer.check(true);
        helper.sleep(90);
        var resultOn = timer.check(true);
        assertEqual(resultOn, true, "After activation delay state should become true");

        // Deactivation test
        timer.check(false);
        helper.sleep(70);
        var resultOff = timer.check(false);
        assertEqual(resultOff, false, "After deactivation delay state should become false");
    });

    // Test 9: Zero delays
    test("Operation with zero delays", function() {
        var timer = new DelayTimer(0, 0);

        // Activation without delay
        var resultOn = timer.check(true);
        assertEqual(resultOn, true, "With zero delay activation should be instant");

        // Deactivation without delay
        var resultOff = timer.check(false);
        assertEqual(resultOff, false, "With zero delay deactivation should be instant");
    });

    // Test 10: get() method with previous state
    test("get() method uses previous state", function() {
        var timer = new DelayTimer(100, 100);

        // Set state
        timer.check(true);
        helper.testDelayState(timer, false, true, false, "After check(true)");

        // get() method should use previous state (true)
        var result = timer.get();
        assertEqual(result, false, "get() should return current state considering delay");
        helper.testDelayState(timer, false, true, false, "After get()");
    });

    // Test 11: Getting current time
    test("Getting current timer operation time", function() {
        var timer = new DelayTimer(100, 100);

        // Start activation delay
        timer.check(true);

        var initialTime = timer.getCurrent();
        assert(initialTime >= 0 && initialTime < 10, "Initial time should be close to 0");

        helper.sleep(50);
        var midTime = timer.getCurrent();
        assert(midTime >= 45 && midTime <= 55, "Time should be around 50 ms");
    });

    // Test 12: Multiple switching
    test("Multiple rapid switching", function() {
        var timer = new DelayTimer(50, 50);

        // Rapid switching back and forth
        timer.check(true);
        helper.sleep(20);
        timer.check(false); // Interrupt
        helper.sleep(10);
        timer.check(true); // Activate again
        helper.sleep(20);
        timer.check(false); // Deactivate again

        // Should be in deactivation waiting state (if managed to activate)
        // or simply deactivated (if didn't manage)
        var state = timer.get();
        var waiting = timer.isWaiting();

        // Check that state is correct (specific state depends on timing)
        assert(!state || (state && waiting), "State should be either false, or true with waiting");
    });

    // Test 13: Long-term state holding
    test("Long-term state holding after triggering", function() {
        var timer = new DelayTimer(50, 50);

        // Activate and wait for triggering
        timer.check(true);
        helper.sleep(60);
        var result1 = timer.check(true);
        assertEqual(result1, true, "After triggering state should be true");

        // Continue holding - state should remain true
        helper.sleep(100);
        var result2 = timer.check(true);
        assertEqual(result2, true, "When continuing to hold state should remain true");
        helper.testDelayState(timer, true, false, false, "During long-term holding");
    });

    // Test 14: Edge cases
    test("Edge cases with negative delays", function() {
        // Negative delays should work as zero
        var timer = new DelayTimer(-100, -50);
        assertEqual(timer.getOnDelay(), -100, "Negative activation delay should be preserved");
        assertEqual(timer.getOffDelay(), -50, "Negative deactivation delay should be preserved");

        // Check operation (should be instant due to negative delays)
        var resultOn = timer.check(true);
        assertEqual(resultOn, true, "With negative delay activation should be instant");

        var resultOff = timer.check(false);
        assertEqual(resultOff, false, "With negative delay deactivation should be instant");
    });

    // Test 15: Simultaneous operation of multiple timers
    test("Multiple DelayTimers operating simultaneously", function() {
        var timer1 = new DelayTimer(50, 30);
        var timer2 = new DelayTimer(30, 50);
        var timer3 = new DelayTimer(0, 0);

        // Activate all simultaneously
        var result1 = timer1.check(true);
        var result2 = timer2.check(true);
        var result3 = timer3.check(true);

        assertEqual(result1, false, "First timer should be in delay");
        assertEqual(result2, false, "Second timer should be in delay");
        assertEqual(result3, true, "Third timer should trigger instantly");

        // Wait for first two to trigger
        helper.sleep(60);

        var final1 = timer1.check(true);
        var final2 = timer2.check(true);
        var final3 = timer3.check(true);

        assertEqual(final1, true, "First timer should trigger");
        assertEqual(final2, true, "Second timer should trigger");
        assertEqual(final3, true, "Third timer should remain activated");
    });

    // Output results
    console.log("\n=== DelayTimer Test Results ===");
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
    var results = runDelayTimerTests();

    if (results.failed === 0) {
        console.log("\n🎉 All DelayTimer tests passed successfully!");
    } else {
        console.log("\n❌ There are failed DelayTimer tests!");
    }
}

// Export for use in other modules
if (typeof module !== 'undefined') {
    module.exports = {
        runDelayTimerTests: runDelayTimerTests,
        DelayTimer: DelayTimer
    };
}
