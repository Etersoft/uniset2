/*
 * Test: Modbus Master -> Slave Chain
 *
 * Scenario:
 * 1. Set value on Slave (Node4: MB_SlaveValue1_AS)
 * 2. Wait for Master to poll
 * 3. Check Master has read the value (Node3: MB_Register1_AS)
 */
load("uniset2-log.js");

const log = uniset_log_create("test_modbus", true, true, true);
log.level("info", "warn", "crit");

function runTest() {
    log.info("=== Test: Modbus Master -> Slave Chain ===");

    const testValue = 777;

    try {
        // Step 1: Set value on Slave (Node4)
        log.info("Setting MB_SlaveValue1_AS@Node4 =", testValue);
        ui.setValue("MB_SlaveValue1_AS@Node4", testValue);

        // Step 2: Wait for Master to poll
        log.info("Waiting for Modbus poll cycle (2 seconds)...");
        uniset.sleep(2000);

        // Step 3: Read from Master (Node3)
        log.info("Reading MB_Register1_AS@Node3...");
        const result = ui.getValue("MB_Register1_AS@Node3");
        log.info("Got value:", result);

        // Step 4: Check respond sensor
        const respond = ui.getValue("MB_Respond_S@Node3");
        log.info("Modbus respond:", respond);

        // Step 5: Verify
        if (result === testValue) {
            log.info("SUCCESS: Modbus chain works correctly");
            return { success: true };
        } else {
            log.warn("FAIL: Expected", testValue, "got", result);
            return {
                success: false,
                error: "Modbus value mismatch: expected " + testValue + ", got " + result
            };
        }
    } catch (e) {
        log.crit("Exception:", e.message || e);
        return { success: false, error: e.message || String(e) };
    }
}

// Additional test: multiple registers
function runMultiRegisterTest() {
    log.info("=== Test: Modbus Multi-Register ===");

    const value1 = 100;
    const value2 = 200;

    try {
        // Set values on Slave
        ui.setValue("MB_SlaveValue1_AS@Node4", value1);
        ui.setValue("MB_SlaveValue2_AS@Node4", value2);

        uniset.sleep(2000);

        // Read from Master
        const result1 = ui.getValue("MB_Register1_AS@Node3");
        const result2 = ui.getValue("MB_Register2_AS@Node3");

        if (result1 === value1 && result2 === value2) {
            log.info("SUCCESS: Multi-register Modbus works");
            return { success: true };
        } else {
            log.warn("FAIL: Multi-register mismatch");
            return {
                success: false,
                error: "Expected " + value1 + "," + value2 + " got " + result1 + "," + result2
            };
        }
    } catch (e) {
        return { success: false, error: e.message || String(e) };
    }
}
