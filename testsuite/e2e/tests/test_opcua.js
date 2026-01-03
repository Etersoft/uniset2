/*
 * Test: OPC UA Server + Exchange
 *
 * Scenario:
 * 1. Set value on Node5 (OPCUA_Value1_AS)
 * 2. Read via OPC UA Exchange
 * 3. Verify value matches
 */
load("uniset2-log.js");

const log = uniset_log_create("test_opcua", true, true, true);
log.level("info", "warn", "crit");

function runTest() {
    log.info("=== Test: OPC UA Server + Exchange ===");

    const testValue = 555;

    try {
        // Step 1: Set value on Node5
        log.info("Setting OPCUA_Value1_AS@Node5 =", testValue);
        ui.setValue("OPCUA_Value1_AS@Node5", testValue);

        // Step 2: Wait for OPC UA sync
        log.info("Waiting for OPC UA sync (2 seconds)...");
        uniset.sleep(2000);

        // Step 3: Read value back
        log.info("Reading OPCUA_Value1_AS@Node5...");
        const result = ui.getValue("OPCUA_Value1_AS@Node5");
        log.info("Got value:", result);

        // Step 4: Verify
        if (result === testValue) {
            log.info("SUCCESS: OPC UA value set/get works");
            return { success: true };
        } else {
            log.warn("FAIL: Expected", testValue, "got", result);
            return {
                success: false,
                error: "OPC UA value mismatch: expected " + testValue + ", got " + result
            };
        }
    } catch (e) {
        log.crit("Exception:", e.message || e);
        return { success: false, error: e.message || String(e) };
    }
}
