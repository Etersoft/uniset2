/*
 * Test: SM Exchange via UNetUDP
 *
 * Scenario:
 * 1. Set value on Node1 (TestValue1_AS)
 * 2. Wait for UNet synchronization
 * 3. Check value on Node2 (TestValue1_AS@Node2)
 */
load("uniset2-log.js");

// Busy-wait sleep function (ms)
function sleep(ms) {
    const start = Date.now();
    while (Date.now() - start < ms) {
        // busy wait
    }
}

const log = uniset_log_create("test_sm_exchange", true, true, true);
log.level("info", "warn", "crit");

function runTest() {
    log.info("=== Test: SM Exchange via UNetUDP ===");

    const testValue = 12345;
    const sensorName = "TestValue1_AS";

    try {
        // Step 1: Set value on Node1
        log.info("Setting", sensorName, "@Node1 =", testValue);
        ui.setValue(sensorName + "@Node1", testValue);

        // Step 2: Wait for UNet synchronization
        log.info("Waiting for UNet sync (3 seconds)...");
        sleep(3000);

        // Step 3: Read value from Node2
        log.info("Reading", sensorName, "@Node2...");
        const result = ui.getValue(sensorName + "@Node2");
        log.info("Got value:", result);

        // Step 4: Verify
        if (result === testValue) {
            log.info("SUCCESS: Value propagated correctly through UNet");
            return { success: true };
        } else {
            log.warn("FAIL: Expected", testValue, "got", result);
            return {
                success: false,
                error: "Value mismatch: expected " + testValue + ", got " + result
            };
        }
    } catch (e) {
        log.crit("Exception:", e.message || e);
        return { success: false, error: e.message || String(e) };
    }
}

// Test for bidirectional exchange
function runBidirectionalTest() {
    log.info("=== Test: Bidirectional SM Exchange ===");

    const value1 = 111;
    const value2 = 222;

    try {
        // Set value on Node1
        ui.setValue("TestValue1_AS@Node1", value1);
        // Set value on Node2
        ui.setValue("TestValue2_AS@Node2", value2);

        sleep(3000);

        // Check values propagated
        const result1 = ui.getValue("TestValue1_AS@Node2");
        const result2 = ui.getValue("TestValue2_AS@Node1");

        if (result1 === value1 && result2 === value2) {
            log.info("SUCCESS: Bidirectional exchange works");
            return { success: true };
        } else {
            log.warn("FAIL: Bidirectional mismatch");
            return { success: false, error: "Bidirectional exchange failed" };
        }
    } catch (e) {
        return { success: false, error: e.message || String(e) };
    }
}
