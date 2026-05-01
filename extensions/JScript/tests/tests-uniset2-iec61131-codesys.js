/*
 * Tests for Codesys Util function blocks (uniset2-iec61131-codesys.js)
 * Verifies exact Codesys specification compliance.
 */

load("uniset2-iec61131-codesys.js");

function sleep(ms)
{
    const start = Date.now();
    while (Date.now() - start < ms) { /* busy wait */ }
}

function runCodesysTests()
{
    var testResults = { passed: 0, failed: 0, tests: [] };

    function test(name, fn)
    {
        try
        {
            fn();
            testResults.tests.push({name: name, status: 'PASSED'});
            testResults.passed++;
            console.log("  + " + name);
        }
        catch (e)
        {
            testResults.tests.push({name: name, status: 'FAILED', error: e.message});
            testResults.failed++;
            console.log("  x " + name + " - " + e.message);
        }
    }

    function assert(condition, message) { if (!condition) throw new Error(message || "Assertion failed"); }
    function assertEqual(a, b, msg) { if (a !== b) throw new Error((msg||"")+" - Expected: "+b+", Got: "+a); }
    function assertNear(a, b, tol, msg) { if (Math.abs(a-b) > tol) throw new Error((msg||"")+" - Expected: "+b+" +/-"+tol+", Got: "+a); }

    // =====================================================================
    console.log("\n=== BLINK ===\n");
    // =====================================================================

    test("BLINK: starts TRUE when first enabled", function() {
        var b = new BLINK(100, 100);
        b.update(true);
        assertEqual(b.OUT, true);
    });

    test("BLINK: toggles after TIMEHIGH", function() {
        var b = new BLINK(60, 60);
        b.update(true);
        assertEqual(b.OUT, true);
        sleep(70);
        b.update(true);
        assertEqual(b.OUT, false, "toggled to false");
        sleep(70);
        b.update(true);
        assertEqual(b.OUT, true, "toggled back");
    });

    test("BLINK: ENABLE=false holds last value TRUE", function() {
        var b = new BLINK(200, 200);
        b.update(true);
        assertEqual(b.OUT, true, "starts true");
        b.update(false);
        assertEqual(b.OUT, true, "holds true on disable");
    });

    test("BLINK: ENABLE=false holds last value FALSE", function() {
        var b = new BLINK(50, 200);
        b.update(true);
        sleep(60); // past TIMEHIGH
        b.update(true);
        assertEqual(b.OUT, false, "toggled to false");
        b.update(false);
        assertEqual(b.OUT, false, "holds false on disable");
    });

    test("BLINK: re-enable starts fresh from TRUE", function() {
        var b = new BLINK(50, 50);
        b.update(true);
        sleep(60);
        b.update(true);
        assertEqual(b.OUT, false, "toggled");
        b.update(false); // holds false
        b.update(true);  // re-enable: starts fresh
        assertEqual(b.OUT, true, "re-enable starts from true");
    });

    test("BLINK: asymmetric timing", function() {
        var b = new BLINK(80, 40);
        b.update(true);
        sleep(50);
        b.update(true);
        assertEqual(b.OUT, true, "still high (50 < 80)");
        sleep(40);
        b.update(true);
        assertEqual(b.OUT, false, "low (90 > 80)");
        sleep(50);
        b.update(true);
        assertEqual(b.OUT, true, "high again (50 > 40)");
    });

    // =====================================================================
    console.log("\n=== HYSTERESIS ===\n");
    // =====================================================================

    test("HYSTERESIS: initial state false", function() {
        var h = new HYSTERESIS();
        assertEqual(h.OUT, false);
    });

    test("HYSTERESIS: below LOW (strict) turns ON", function() {
        var h = new HYSTERESIS();
        h.update(15, 25, 20);
        assertEqual(h.OUT, true);
    });

    test("HYSTERESIS: above HIGH (strict) turns OFF", function() {
        var h = new HYSTERESIS();
        h.update(15, 25, 20);
        h.update(30, 25, 20);
        assertEqual(h.OUT, false);
    });

    test("HYSTERESIS: exactly at LOW — no change (dead band)", function() {
        var h = new HYSTERESIS();
        // starts false, IN==LOW should NOT trigger ON
        h.update(20, 25, 20);
        assertEqual(h.OUT, false, "at LOW boundary, was false, stays false");
    });

    test("HYSTERESIS: exactly at HIGH — no change (dead band)", function() {
        var h = new HYSTERESIS();
        h.update(15, 25, 20); // ON
        h.update(25, 25, 20); // at HIGH boundary
        assertEqual(h.OUT, true, "at HIGH boundary, was true, stays true");
    });

    test("HYSTERESIS: in dead band holds state", function() {
        var h = new HYSTERESIS();
        h.update(15, 25, 20); // ON
        h.update(22, 25, 20); // in dead band
        assertEqual(h.OUT, true, "holds ON");

        var h2 = new HYSTERESIS();
        h2.update(30, 25, 20); // OFF
        h2.update(22, 25, 20); // in dead band
        assertEqual(h2.OUT, false, "holds OFF");
    });

    // =====================================================================
    console.log("\n=== LIMITALARM ===\n");
    // =====================================================================

    test("LIMITALARM: in limits", function() {
        var la = new LIMITALARM();
        la.update(50, 100, 0);
        assertEqual(la.IL, true); assertEqual(la.O, false); assertEqual(la.U, false);
    });

    test("LIMITALARM: over limit", function() {
        var la = new LIMITALARM();
        la.update(150, 100, 0);
        assertEqual(la.O, true); assertEqual(la.IL, false);
    });

    test("LIMITALARM: under limit", function() {
        var la = new LIMITALARM();
        la.update(-10, 100, 0);
        assertEqual(la.U, true); assertEqual(la.IL, false);
    });

    test("LIMITALARM: at boundaries is in limits", function() {
        var la = new LIMITALARM();
        la.update(100, 100, 0);
        assertEqual(la.IL, true, "at HIGH");
        la.update(0, 100, 0);
        assertEqual(la.IL, true, "at LOW");
    });

    // =====================================================================
    console.log("\n=== LIN_TRAFO ===\n");
    // =====================================================================

    test("LIN_TRAFO: midpoint", function() {
        var lt = new LIN_TRAFO(0, 100, 0, 1000);
        lt.update(50);
        assertEqual(lt.OUT, 500);
        assertEqual(lt.ERROR, false);
    });

    test("LIN_TRAFO: 4-20mA to 0-100%", function() {
        var lt = new LIN_TRAFO(4, 20, 0, 100);
        lt.update(12);
        assertEqual(lt.OUT, 50);
    });

    test("LIN_TRAFO: ERROR on zero range", function() {
        var lt = new LIN_TRAFO(50, 50, 0, 100);
        lt.update(50);
        assertEqual(lt.ERROR, true);
    });

    test("LIN_TRAFO: ERROR on out-of-range input", function() {
        var lt = new LIN_TRAFO(0, 100, 0, 1000);
        lt.update(150);
        assertEqual(lt.ERROR, true, "above range");
        assert(lt.OUT > 1000, "extrapolated output");
        lt.update(50);
        assertEqual(lt.ERROR, false, "back in range");
    });

    test("LIN_TRAFO: inverted range", function() {
        var lt = new LIN_TRAFO(0, 100, 100, 0);
        lt.update(25);
        assertEqual(lt.OUT, 75);
    });

    // =====================================================================
    console.log("\n=== RAMP_REAL ===\n");
    // =====================================================================

    test("RAMP_REAL: first call initializes to IN", function() {
        var r = new RAMP_REAL(10, 10, 1000);
        r.update(50, false);
        assertEqual(r.OUT, 50);
    });

    test("RAMP_REAL: RESET freezes OUT (does NOT snap to IN)", function() {
        var r = new RAMP_REAL(100, 100, 1000);
        r.update(50, false); // init to 50
        sleep(50);
        r.update(100, true); // RESET: should hold 50, NOT jump to 100
        assertEqual(r.OUT, 50, "RESET freezes at last value");
    });

    test("RAMP_REAL: TIMEBASE=0 means per-call step", function() {
        var r = new RAMP_REAL(5, 5, 0); // 5 units per call
        r.update(0, false); // init
        r.update(100, false);
        assertEqual(r.OUT, 5, "one step up = 5");
        r.update(100, false);
        assertEqual(r.OUT, 10, "two steps = 10");
    });

    test("RAMP_REAL: rate-limited ascend with TIMEBASE", function() {
        var r = new RAMP_REAL(100, 100, 1000); // 100 units per 1000ms
        r.update(0, false);
        sleep(100);
        r.update(1000, false);
        assert(r.OUT > 0, "moved up");
        assert(r.OUT < 50, "rate-limited, got " + r.OUT);
    });

    test("RAMP_REAL: rate-limited descend", function() {
        var r = new RAMP_REAL(100, 100, 1000);
        r.update(100, false);
        sleep(100);
        r.update(0, false);
        assert(r.OUT < 100, "moved down");
        assert(r.OUT > 50, "rate-limited, got " + r.OUT);
    });

    // =====================================================================
    console.log("\n=== PID ===\n");
    // =====================================================================

    test("PID: reset returns Y_OFFSET", function() {
        var pid = new PID(1.0, 0, 0, -100, 100);
        pid.Y_OFFSET = 10;
        pid.update(50, 50, true);
        assertEqual(pid.Y, 10);
        assertEqual(pid.OVERFLOW, false);
    });

    test("PID: P-only responds to error", function() {
        var pid = new PID(2.0, 0, 0, -100, 100);
        pid.update(0, 0, true);
        sleep(50);
        pid.update(40, 50, false);
        assertNear(pid.Y, 20, 1, "KP=2, error=10");
    });

    test("PID: output clamped with LIMITS_ACTIVE", function() {
        var pid = new PID(10.0, 0, 0, -50, 50);
        pid.update(0, 0, true);
        sleep(50);
        pid.update(0, 100, false);
        assertEqual(pid.Y, 50, "clamped to Y_MAX");
        assertEqual(pid.LIMITS_ACTIVE, true);
    });

    test("PID: MANUAL mode bypasses controller", function() {
        var pid = new PID(10.0, 0, 0, -100, 100);
        pid.update(0, 0, true);
        pid.MANUAL = true;
        pid.Y_MANUAL = 42;
        sleep(50);
        pid.update(0, 100, false);
        assertEqual(pid.Y, 42, "manual override");
        assertEqual(pid.LIMITS_ACTIVE, false);
    });

    test("PID: integral accumulates", function() {
        var pid = new PID(1.0, 1.0, 0, -1000, 1000);
        pid.update(0, 0, true);
        for (var i = 0; i < 5; i++) { sleep(50); pid.update(0, 10, false); }
        assert(pid.Y > 10, "integral increases Y beyond P-only, got " + pid.Y);
    });

    test("PID: anti-windup prevents integral overshoot", function() {
        var pid = new PID(10.0, 1.0, 0, -50, 50);
        pid.update(0, 0, true);
        // Drive to saturation
        for (var i = 0; i < 10; i++) { sleep(30); pid.update(0, 100, false); }
        assertEqual(pid.Y, 50, "clamped");
        // Now reverse — should respond quickly if anti-windup works
        sleep(30);
        pid.update(100, 0, false); // error = -100
        assert(pid.Y < 50, "responds to reverse after saturation, got " + pid.Y);
    });

    // =====================================================================
    console.log("\n=== DERIVATIVE ===\n");
    // =====================================================================

    test("DERIVATIVE: reset gives zero", function() {
        var d = new DERIVATIVE();
        d.update(100, 0, true);
        assertEqual(d.OUT, 0);
    });

    test("DERIVATIVE: detects rising signal (auto TM)", function() {
        var d = new DERIVATIVE();
        d.update(0, 0, true);
        sleep(50); d.update(10, 0, false);
        sleep(50); d.update(20, 0, false);
        sleep(50); d.update(30, 0, false);
        assert(d.OUT > 0, "positive for rising, got " + d.OUT);
    });

    test("DERIVATIVE: detects falling signal", function() {
        var d = new DERIVATIVE();
        d.update(100, 0, true);
        sleep(50); d.update(90, 0, false);
        sleep(50); d.update(80, 0, false);
        sleep(50); d.update(70, 0, false);
        assert(d.OUT < 0, "negative for falling, got " + d.OUT);
    });

    test("DERIVATIVE: constant signal gives ~zero", function() {
        var d = new DERIVATIVE();
        d.update(50, 0, true);
        sleep(50); d.update(50, 0, false);
        sleep(50); d.update(50, 0, false);
        sleep(50); d.update(50, 0, false);
        assertNear(d.OUT, 0, 1, "constant -> ~0");
    });

    test("DERIVATIVE: explicit TM parameter", function() {
        var d = new DERIVATIVE();
        d.update(0, 100, true); // reset, TM=100ms
        d.update(10, 100, false);
        d.update(20, 100, false);
        d.update(30, 100, false);
        // 4-point: (30 + 3*20 - 3*10 - 0) / (6 * 0.1) = (30+60-30-0)/0.6 = 60/0.6 = 100
        assertNear(d.OUT, 100, 5, "100 units/sec with TM=100ms");
    });

    test("DERIVATIVE: uses 4-point algorithm after 3 samples", function() {
        var d = new DERIVATIVE();
        d.update(0, 100, true);
        d.update(100, 100, false); // 1st sample: simple diff
        d.update(200, 100, false); // 2nd: simple diff
        d.update(300, 100, false); // 3rd+: 4-point
        // 4-point: (300 + 3*200 - 3*100 - 0) / (6*0.1) = (300+600-300-0)/0.6 = 600/0.6 = 1000
        assertNear(d.OUT, 1000, 10, "4-point on linear ramp");
    });

    // =====================================================================
    console.log("\n=== INTEGRAL ===\n");
    // =====================================================================

    test("INTEGRAL: reset to zero", function() {
        var ig = new INTEGRAL();
        ig.update(100, 0, false);
        sleep(50);
        ig.update(100, 0, false);
        ig.update(0, 0, true);
        assertEqual(ig.OUT, 0);
        assertEqual(ig.OVERFLOW, false);
    });

    test("INTEGRAL: accumulates with auto TM", function() {
        var ig = new INTEGRAL();
        ig.update(0, 0, true);
        for (var i = 0; i < 5; i++) { sleep(50); ig.update(100, 0, false); }
        assert(ig.OUT > 0, "accumulated, got " + ig.OUT);
    });

    test("INTEGRAL: explicit TM parameter", function() {
        var ig = new INTEGRAL();
        ig.update(0, 100, true);     // reset
        ig.update(100, 100, false);  // 100 * 0.1s = 10
        ig.update(100, 100, false);  // + 10 = 20
        ig.update(100, 100, false);  // + 10 = 30
        assertNear(ig.OUT, 30, 1, "100 * 0.1s * 3 = 30");
    });

    test("INTEGRAL: zero input holds value", function() {
        var ig = new INTEGRAL();
        ig.update(0, 100, true);
        ig.update(100, 100, false); // accumulate 10
        var val = ig.OUT;
        ig.update(0, 100, false);   // zero input
        assertNear(ig.OUT, val, 0.01, "zero input preserves");
    });

    test("INTEGRAL: OVERFLOW flag on Infinity", function() {
        var ig = new INTEGRAL();
        ig.update(0, 1000, true);
        ig.OUT = Number.MAX_VALUE;
        ig.update(Number.MAX_VALUE, 1000, false); // MAX_VALUE + MAX_VALUE*1 = Infinity
        assertEqual(ig.OVERFLOW, true, "overflow detected");
        assertEqual(ig.OUT, 0, "reset on overflow");
    });

    // =====================================================================
    console.log("\n=== Codesys Util Test Results ===");
    console.log("Passed: " + testResults.passed);
    console.log("Failed: " + testResults.failed);
    console.log("Total: " + (testResults.passed + testResults.failed));

    if (testResults.failed > 0)
    {
        console.log("\nFailed tests:");
        testResults.tests.forEach(function(t) {
            if (t.status === 'FAILED')
                console.log("  x " + t.name + " - " + t.error);
        });
    }

    return testResults;
}

if (typeof module === 'undefined')
{
    var results = runCodesysTests();
    if (results.failed === 0)
        console.log("\nAll Codesys Util tests passed!");
    else
        console.log("\nThere are failed Codesys Util tests!");
}

if (typeof module !== 'undefined')
    module.exports = { runCodesysTests: runCodesysTests };
