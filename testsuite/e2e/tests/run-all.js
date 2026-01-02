/*
 * E2E Test Runner for UniSet2
 * Async version using timers
 */
load("uniset2-log.js");
load("uniset2-timers.js");

// Required for JScript
uniset_inputs = [
    { name: "TestMode_S" }
];
uniset_outputs = [
    { name: "TestCounter_AS" }
];

// Create logger
const log = uniset_log_create("e2e-runner", true, true, true);
log.level("info", "warn", "crit");

// Test state machine
let testState = {
    currentTest: 0,
    phase: "init",  // init, running, verify, done
    testValue: 0,
    passed: 0,
    failed: 0,
    results: []
};

// Tests to run
const TESTS = [
    { name: "SM Exchange via UNet", run: runSmExchangeTest }
];

// Main entry point
function uniset_on_start() {
    log.info("=== E2E Test Suite ===");
    log.info("Running", TESTS.length, "tests");
    log.info("");

    // Start first test after 2 seconds (let nodes stabilize)
    askTimer("start_tests", 2000, 1, function() {
        startNextTest();
    });
}

// Start next test in queue
function startNextTest() {
    if (testState.currentTest >= TESTS.length) {
        printResults();
        return;
    }

    const test = TESTS[testState.currentTest];
    log.info("--- Running:", test.name, "---");
    testState.phase = "running";

    try {
        test.run();
    } catch (e) {
        log.crit("ERROR in", test.name, ":", e.message || e);
        recordResult(test.name, false, e.message || String(e));
        testState.currentTest++;
        askTimer("next_test", 500, 1, startNextTest);
    }
}

// Record test result
function recordResult(name, success, error) {
    testState.results.push({ name: name, success: success, error: error });
    if (success) {
        testState.passed++;
        log.info("PASS:", name);
    } else {
        testState.failed++;
        log.warn("FAIL:", name, error || "");
    }
}

// Print final results and exit
function printResults() {
    log.info("");
    log.info("=== Test Results ===");
    log.info("Passed:", testState.passed);
    log.info("Failed:", testState.failed);
    log.info("");

    for (let i = 0; i < testState.results.length; i++) {
        const t = testState.results[i];
        const status = t.success ? "[PASS]" : "[FAIL]";
        log.info(status, t.name, t.error ? "- " + t.error : "");
    }

    if (testState.failed > 0) {
        log.crit("Tests failed!");
        std.exit(1);
    } else {
        log.info("All tests passed!");
        std.exit(0);
    }
}

// ============== SM Exchange Test ==============
function runSmExchangeTest() {
    const testValue = 12345 + Math.floor(Math.random() * 1000);
    const sensorName = "TestValue1_AS";

    log.info("Setting", sensorName, "@Node1 =", testValue);

    try {
        ui.setValue(sensorName + "@Node1", testValue);
    } catch (e) {
        recordResult("SM Exchange via UNet", false, "setValue failed: " + e.message);
        testState.currentTest++;
        askTimer("next_test", 500, 1, startNextTest);
        return;
    }

    // Store expected value
    testState.testValue = testValue;

    // Check result after 3 seconds (UNet sync time)
    log.info("Waiting 3 seconds for UNet sync...");
    askTimer("verify_sm_exchange", 3000, 1, function() {
        verifySmExchange(sensorName, testValue);
    });
}

function verifySmExchange(sensorName, expectedValue) {
    log.info("Reading", sensorName, "@Node2...");

    try {
        const result = ui.getValue(sensorName + "@Node2");
        log.info("Got value:", result);

        if (result === expectedValue) {
            log.info("Value propagated correctly through UNet");
            recordResult("SM Exchange via UNet", true);
        } else {
            recordResult("SM Exchange via UNet", false,
                "Value mismatch: expected " + expectedValue + ", got " + result);
        }
    } catch (e) {
        recordResult("SM Exchange via UNet", false, "getValue failed: " + e.message);
    }

    // Move to next test
    testState.currentTest++;
    askTimer("next_test", 500, 1, startNextTest);
}

// Callbacks
function uniset_on_step() {}
function uniset_on_stop() {}
