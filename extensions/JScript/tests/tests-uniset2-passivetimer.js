/*
 * Tests for PassiveTimer module
 */

load("uniset2-passivetimer.js");

// Helper function for testing timers
function createTimerTestHelper() {
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
        }
    };
}

// Main tests
function runPassiveTimerTests() {
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

    console.log("\n=== Running PassiveTimer Tests ===\n");

    var helper = createTimerTestHelper();

    // Test 1: Timer creation
    test("Creating timer with default interval", function() {
        var timer = new PassiveTimer();
        assert(timer !== undefined, "Timer not created");
        assertEqual(timer.getInterval(), 0, "Default interval should be 0");
        assert(timer.checkTime() === true, "Timer with interval 0 should trigger immediately");
    });

    test("Creating timer with specified interval", function() {
        var timer = new PassiveTimer(1000);
        assertEqual(timer.getInterval(), 1000, "Interval should be 1000 ms");
        assert(timer.checkTime() === false, "Timer should not trigger immediately");
    });

    // Test 2: Timer operation check
    test("Timer triggers after time expiration", function() {
        var timer = new PassiveTimer(50); // Short interval for test
        var initialCheck = timer.checkTime();
        assert(initialCheck === false, "Timer should not trigger immediately");

        helper.sleep(60); // Wait more than interval

        var finalCheck = timer.checkTime();
        assert(finalCheck === true, "Timer should trigger after time expiration");
    });

    test("Timer does not trigger before time expiration", function() {
        var timer = new PassiveTimer(100);
        var check1 = timer.checkTime();
        assert(check1 === false, "Timer should not trigger immediately");

        helper.sleep(50); // Wait less than interval

        var check2 = timer.checkTime();
        assert(check2 === false, "Timer should not trigger before time expiration");
    });

    // Test 3: Setting interval
    test("Setting interval via setTiming", function() {
        var timer = new PassiveTimer();
        var newInterval = timer.setTiming(500);

        assertEqual(newInterval, 500, "setTiming should return the set interval");
        assertEqual(timer.getInterval(), 500, "Interval should be set to 500 ms");
        assert(timer.checkTime() === false, "Timer should not trigger immediately after setting");
    });

    test("Reconfiguring interval", function() {
        var timer = new PassiveTimer(1000);
        timer.setTiming(200);

        assertEqual(timer.getInterval(), 200, "Interval should be changed to 200 ms");

        helper.sleep(250);
        assert(timer.checkTime() === true, "Timer should trigger after reconfiguration");
    });

    // Test 4: Timer reset
    test("Resetting timer", function() {
        var timer = new PassiveTimer(100);

        helper.sleep(80);
        var beforeReset = timer.checkTime();
        assert(beforeReset === false, "Timer should not trigger before reset");

        timer.reset();

        helper.sleep(50);
        var afterReset = timer.checkTime();
        assert(afterReset === false, "Timer should not trigger immediately after reset");

        helper.sleep(60);
        var finalCheck = timer.checkTime();
        assert(finalCheck === true, "Timer should trigger after reset and waiting");
    });

    // Test 5: Getting current time
    test("Getting current timer operation time", function() {
        var timer = new PassiveTimer(1000);

        var initialTime = timer.getCurrent();
        assert(initialTime >= 0 && initialTime < 10, "Initial time should be close to 0");

        helper.sleep(50);
        var midTime = timer.getCurrent();
        assert(midTime >= 45 && midTime <= 55, "Time should be around 50 ms");

        helper.sleep(60);
        var finalTime = timer.getCurrent();
        assert(finalTime >= 105 && finalTime <= 115, "Time should be around 110 ms");
    });

    // Test 6: Getting remaining time
    test("Getting remaining time", function() {
        var timer = new PassiveTimer(100);

        var initialLeft = timer.getLeft();
        assert(initialLeft <= 100 && initialLeft > 90, "Initially remaining time should be around 100 ms");

        helper.sleep(30);
        var midLeft = timer.getLeft();
        assert(midLeft <= 70 && midLeft > 60, "After 30 ms remaining time should be around 70 ms");

        helper.sleep(80);
        var finalLeft = timer.getLeft();
        assert(finalLeft === 0, "After time expiration remaining time should be 0");
    });

    // Test 7: Timer termination
    test("Terminating timer", function() {
        var timer = new PassiveTimer(100);

        helper.sleep(50);
        timer.terminate();

        // After termination current time should be reset
        var currentAfterTerminate = timer.getCurrent();
        assert(currentAfterTerminate < 10, "After termination time should be reset");

        // Check that checkTime works correctly after termination
        var checkAfterTerminate = timer.checkTime();
        assert(checkAfterTerminate === false, "After termination timer should not trigger");
    });

    // Test 8: Multiple timers simultaneously
    test("Multiple timers operating simultaneously", function() {
        var timer1 = new PassiveTimer(50);
        var timer2 = new PassiveTimer(100);
        var timer3 = new PassiveTimer(150);

        helper.sleep(60);

        var check1 = timer1.checkTime();
        var check2 = timer2.checkTime();
        var check3 = timer3.checkTime();

        assert(check1 === true, "First timer should trigger");
        assert(check2 === false, "Second timer should not trigger");
        assert(check3 === false, "Third timer should not trigger");

        helper.sleep(50);

        var check2_final = timer2.checkTime();
        var check3_final = timer3.checkTime();

        assert(check2_final === true, "Second timer should trigger");
        assert(check3_final === false, "Third timer should not trigger");

        helper.sleep(50);

        var check3_final2 = timer3.checkTime();
        assert(check3_final2 === true, "Third timer should trigger");
    });

    // Test 9: Edge cases
    test("Edge cases", function() {
        // Timer with zero interval
        var zeroTimer = new PassiveTimer(0);
        assert(zeroTimer.checkTime() === true, "Timer with interval 0 should always be triggered");

        // Negative interval (should work like 0)
        var negativeTimer = new PassiveTimer(-100);
        assert(negativeTimer.checkTime() === true, "Timer with negative interval should behave like with 0");

        // Very large interval
        var largeTimer = new PassiveTimer(1000000);
        assert(largeTimer.checkTime() === false, "Timer with large interval should not trigger immediately");
    });

    // Test 10: Accuracy check
    test("Checking time interval accuracy", function() {
        var testIntervals = [10, 50, 100, 200];

        for (var i = 0; i < testIntervals.length; i++) {
            var interval = testIntervals[i];
            var timer = new PassiveTimer(interval);

            var startTime = Date.now();
            while (!timer.checkTime()) {
                // Wait for trigger
            }
            var endTime = Date.now();
            var actualInterval = endTime - startTime;

            // Allow 10 ms tolerance due to JavaScript timer inaccuracy
            var tolerance = 10;
            var isAccurate = Math.abs(actualInterval - interval) <= tolerance;

            assert(isAccurate, "Interval " + interval + " ms: expected " + interval +
                ", got " + actualInterval + " (deviation: " + Math.abs(actualInterval - interval) + " ms)");
        }
    });

    // Output results
    console.log("\n=== PassiveTimer Test Results ===");
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
    var results = runPassiveTimerTests();

    if (results.failed === 0) {
        console.log("\n🎉 All PassiveTimer tests passed successfully!");
    } else {
        console.log("\n❌ There are failed PassiveTimer tests!");
    }
}

// Export for use in other modules
if (typeof module !== 'undefined') {
    module.exports = {
        runPassiveTimerTests: runPassiveTimerTests,
        PassiveTimer: PassiveTimer
    };
}
