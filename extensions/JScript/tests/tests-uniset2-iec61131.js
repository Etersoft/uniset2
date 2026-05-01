/*
 * Tests for IEC 61131-3 function blocks (uniset2-iec61131.js)
 */

load("uniset2-iec61131.js");

function sleep(ms)
{
    const start = Date.now();
    while (Date.now() - start < ms) { /* busy wait */ }
}

function runIEC61131Tests()
{
    var testResults = {
        passed: 0,
        failed: 0,
        tests: []
    };

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

    function assert(condition, message)
    {
        if (!condition)
            throw new Error(message || "Assertion failed");
    }

    function assertEqual(actual, expected, message)
    {
        if (actual !== expected)
            throw new Error((message || "Values not equal") +
                " - Expected: " + expected + ", Got: " + actual);
    }

    // =====================================================================
    console.log("\n=== RS (Reset-dominant bistable) ===\n");
    // =====================================================================

    test("RS: initial state is false", function() {
        var rs = new RS();
        assertEqual(rs.Q1, false);
    });

    test("RS: SET=1, RESET1=0 -> Q1=1", function() {
        var rs = new RS();
        rs.update(true, false);
        assertEqual(rs.Q1, true);
    });

    test("RS: SET=0, RESET1=1 -> Q1=0", function() {
        var rs = new RS();
        rs.update(true, false);
        assertEqual(rs.Q1, true);
        rs.update(false, true);
        assertEqual(rs.Q1, false);
    });

    test("RS: SET=0, RESET1=0 -> holds previous state", function() {
        var rs = new RS();
        rs.update(true, false);
        assertEqual(rs.Q1, true);
        rs.update(false, false);
        assertEqual(rs.Q1, true, "should hold true");

        var rs2 = new RS();
        rs2.update(false, false);
        assertEqual(rs2.Q1, false, "should hold false");
    });

    test("RS: SET=1, RESET1=1 -> Q1=0 (reset dominant)", function() {
        var rs = new RS();
        rs.update(true, false);
        assertEqual(rs.Q1, true);
        rs.update(true, true);
        assertEqual(rs.Q1, false, "reset must dominate");
    });

    // =====================================================================
    console.log("\n=== SR (Set-dominant bistable) ===\n");
    // =====================================================================

    test("SR: initial state is false", function() {
        var sr = new SR();
        assertEqual(sr.Q1, false);
    });

    test("SR: SET1=1, RESET=0 -> Q1=1", function() {
        var sr = new SR();
        sr.update(true, false);
        assertEqual(sr.Q1, true);
    });

    test("SR: SET1=0, RESET=1 -> Q1=0", function() {
        var sr = new SR();
        sr.update(true, false);
        sr.update(false, true);
        assertEqual(sr.Q1, false);
    });

    test("SR: SET1=0, RESET=0 -> holds previous state", function() {
        var sr = new SR();
        sr.update(true, false);
        sr.update(false, false);
        assertEqual(sr.Q1, true, "should hold true");
    });

    test("SR: SET1=1, RESET=1 -> Q1=1 (set dominant)", function() {
        var sr = new SR();
        sr.update(false, true);
        assertEqual(sr.Q1, false);
        sr.update(true, true);
        assertEqual(sr.Q1, true, "set must dominate");
    });

    // =====================================================================
    console.log("\n=== R_TRIG (Rising edge detector) ===\n");
    // =====================================================================

    test("R_TRIG: initial state is false", function() {
        var rt = new R_TRIG();
        assertEqual(rt.Q, false);
    });

    test("R_TRIG: false->true gives one pulse", function() {
        var rt = new R_TRIG();
        rt.update(false);
        assertEqual(rt.Q, false, "no edge yet");
        rt.update(true);
        assertEqual(rt.Q, true, "rising edge detected");
        rt.update(true);
        assertEqual(rt.Q, false, "pulse lasts only one cycle");
    });

    test("R_TRIG: true->false->true gives another pulse", function() {
        var rt = new R_TRIG();
        rt.update(true);
        assertEqual(rt.Q, true, "first rising edge");
        rt.update(true);
        assertEqual(rt.Q, false);
        rt.update(false);
        assertEqual(rt.Q, false);
        rt.update(true);
        assertEqual(rt.Q, true, "second rising edge");
    });

    test("R_TRIG: repeated false gives no pulse", function() {
        var rt = new R_TRIG();
        for (var i = 0; i < 5; i++)
        {
            rt.update(false);
            assertEqual(rt.Q, false, "cycle " + i);
        }
    });

    // =====================================================================
    console.log("\n=== F_TRIG (Falling edge detector) ===\n");
    // =====================================================================

    test("F_TRIG: initial state is false", function() {
        var ft = new F_TRIG();
        assertEqual(ft.Q, false);
    });

    test("F_TRIG: true->false gives one pulse", function() {
        var ft = new F_TRIG();
        ft.update(true);
        assertEqual(ft.Q, false, "no falling edge yet");
        ft.update(false);
        assertEqual(ft.Q, true, "falling edge detected");
        ft.update(false);
        assertEqual(ft.Q, false, "pulse lasts only one cycle");
    });

    test("F_TRIG: false->true->false gives another pulse", function() {
        var ft = new F_TRIG();
        ft.update(true);
        ft.update(false);
        assertEqual(ft.Q, true, "first falling edge");
        ft.update(false);
        assertEqual(ft.Q, false);
        ft.update(true);
        assertEqual(ft.Q, false);
        ft.update(false);
        assertEqual(ft.Q, true, "second falling edge");
    });

    test("F_TRIG: first call with false gives no pulse (no previous true)", function() {
        var ft = new F_TRIG();
        ft.update(false);
        assertEqual(ft.Q, false, "CLK_prev was false, no edge");
    });

    // =====================================================================
    console.log("\n=== CTU (Up counter) ===\n");
    // =====================================================================

    test("CTU: initial state CV=0, Q=false", function() {
        var ctu = new CTU(3);
        assertEqual(ctu.CV, 0);
        assertEqual(ctu.Q, false);
    });

    test("CTU: counts rising edges", function() {
        var ctu = new CTU(3);
        ctu.update(true, false);   // edge 1
        assertEqual(ctu.CV, 1);
        ctu.update(false, false);
        ctu.update(true, false);   // edge 2
        assertEqual(ctu.CV, 2);
        assertEqual(ctu.Q, false, "not yet reached PV");
        ctu.update(false, false);
        ctu.update(true, false);   // edge 3
        assertEqual(ctu.CV, 3);
        assertEqual(ctu.Q, true, "CV >= PV");
    });

    test("CTU: does not count while CU stays true", function() {
        var ctu = new CTU(5);
        ctu.update(true, false);
        assertEqual(ctu.CV, 1);
        ctu.update(true, false);
        assertEqual(ctu.CV, 1, "no edge, no count");
        ctu.update(true, false);
        assertEqual(ctu.CV, 1, "still no edge");
    });

    test("CTU: reset sets CV=0", function() {
        var ctu = new CTU(3);
        ctu.update(true, false);
        ctu.update(false, false);
        ctu.update(true, false);
        assertEqual(ctu.CV, 2);
        ctu.update(false, true);
        assertEqual(ctu.CV, 0, "reset");
        assertEqual(ctu.Q, false);
    });

    test("CTU: continues counting past PV", function() {
        var ctu = new CTU(2);
        ctu.update(true, false);
        ctu.update(false, false);
        ctu.update(true, false);
        assertEqual(ctu.Q, true, "reached PV");
        ctu.update(false, false);
        ctu.update(true, false);
        assertEqual(ctu.CV, 3, "counts beyond PV");
        assertEqual(ctu.Q, true);
    });

    // =====================================================================
    console.log("\n=== CTD (Down counter) ===\n");
    // =====================================================================

    test("CTD: initial state CV=0, Q=true", function() {
        var ctd = new CTD(3);
        assertEqual(ctd.CV, 0);
        assertEqual(ctd.Q, true, "CV <= 0 initially");
    });

    test("CTD: load sets CV=PV", function() {
        var ctd = new CTD(5);
        ctd.update(false, true);
        assertEqual(ctd.CV, 5);
        assertEqual(ctd.Q, false, "CV > 0");
    });

    test("CTD: counts down on rising edges", function() {
        var ctd = new CTD(3);
        ctd.update(false, true);  // load
        assertEqual(ctd.CV, 3);

        ctd.update(true, false);  // edge 1
        assertEqual(ctd.CV, 2);
        ctd.update(false, false);
        ctd.update(true, false);  // edge 2
        assertEqual(ctd.CV, 1);
        assertEqual(ctd.Q, false);
        ctd.update(false, false);
        ctd.update(true, false);  // edge 3
        assertEqual(ctd.CV, 0);
        assertEqual(ctd.Q, true, "CV <= 0");
    });

    test("CTD: does not count below 0", function() {
        var ctd = new CTD(1);
        ctd.update(false, true);  // load CV=1
        ctd.update(true, false);  // count down to 0
        assertEqual(ctd.CV, 0);
        ctd.update(false, false);
        ctd.update(true, false);  // try to count below 0
        assertEqual(ctd.CV, 0, "should not go below 0");
    });

    test("CTD: simultaneous LOAD and CD — LOAD wins", function() {
        var ctd = new CTD(5);
        ctd.update(true, true); // CD rising edge + LOAD simultaneously
        assertEqual(ctd.CV, 5, "LOAD takes priority, CV=PV");
    });

    // =====================================================================
    console.log("\n=== CTUD (Up/Down counter) ===\n");
    // =====================================================================

    test("CTUD: initial state CV=0, QU=false, QD=true", function() {
        var ctud = new CTUD(3);
        assertEqual(ctud.CV, 0);
        assertEqual(ctud.QU, false);
        assertEqual(ctud.QD, true);
    });

    test("CTUD: counts up", function() {
        var ctud = new CTUD(3);
        ctud.update(true, false, false, false);
        assertEqual(ctud.CV, 1);
        ctud.update(false, false, false, false);
        ctud.update(true, false, false, false);
        assertEqual(ctud.CV, 2);
        ctud.update(false, false, false, false);
        ctud.update(true, false, false, false);
        assertEqual(ctud.CV, 3);
        assertEqual(ctud.QU, true, "CV >= PV");
        assertEqual(ctud.QD, false);
    });

    test("CTUD: counts down", function() {
        var ctud = new CTUD(3);
        ctud.update(false, false, false, true); // load PV=3
        assertEqual(ctud.CV, 3);
        ctud.update(false, true, false, false);  // edge down
        assertEqual(ctud.CV, 2);
        ctud.update(false, false, false, false);
        ctud.update(false, true, false, false);
        assertEqual(ctud.CV, 1);
    });

    test("CTUD: simultaneous up and down", function() {
        var ctud = new CTUD(5);
        ctud.update(false, false, false, true); // load PV=5
        assertEqual(ctud.CV, 5);
        // Both edges at the same time: +1 -1 = net 0
        ctud.update(true, true, false, false);
        assertEqual(ctud.CV, 5, "up and down cancel out");
    });

    test("CTUD: reset takes priority over load", function() {
        var ctud = new CTUD(5);
        ctud.update(false, false, false, true); // load
        assertEqual(ctud.CV, 5);
        ctud.update(false, false, true, true); // reset + load
        assertEqual(ctud.CV, 0, "reset takes priority");
    });

    test("CTUD: does not count below 0", function() {
        var ctud = new CTUD(5);
        ctud.update(false, true, false, false);
        assertEqual(ctud.CV, 0, "should not go below 0");
    });

    // =====================================================================
    console.log("\n=== TON (On-delay timer) ===\n");
    // =====================================================================

    test("TON: initial state Q=false, ET=0", function() {
        var ton = new TON(100);
        assertEqual(ton.Q, false);
        assertEqual(ton.ET, 0);
    });

    test("TON: Q stays false before PT", function() {
        var ton = new TON(100);
        ton.update(true);
        sleep(50);
        ton.update(true);
        assertEqual(ton.Q, false, "not yet");
        assert(ton.ET >= 40 && ton.ET <= 60, "ET should be ~50ms, got " + ton.ET);
    });

    test("TON: Q becomes true after PT", function() {
        var ton = new TON(80);
        ton.update(true);
        sleep(100);
        ton.update(true);
        assertEqual(ton.Q, true, "should be true after PT");
        assertEqual(ton.ET, 80, "ET capped at PT");
    });

    test("TON: IN=false resets Q and ET immediately", function() {
        var ton = new TON(80);
        ton.update(true);
        sleep(100);
        ton.update(true);
        assertEqual(ton.Q, true);
        ton.update(false);
        assertEqual(ton.Q, false, "Q resets on IN=false");
        assertEqual(ton.ET, 0, "ET resets on IN=false");
    });

    test("TON: IN=false during timing resets ET", function() {
        var ton = new TON(100);
        ton.update(true);
        sleep(50);
        ton.update(true);
        assert(ton.ET > 0, "ET should be > 0");
        ton.update(false);
        assertEqual(ton.ET, 0, "ET resets");
        assertEqual(ton.Q, false);
    });

    test("TON: re-trigger starts fresh timing", function() {
        var ton = new TON(80);
        ton.update(true);
        sleep(50);
        ton.update(false); // reset
        ton.update(true);  // re-trigger
        sleep(50);
        ton.update(true);
        assertEqual(ton.Q, false, "fresh timer not expired yet");
        sleep(40);
        ton.update(true);
        assertEqual(ton.Q, true, "now expired");
    });

    // =====================================================================
    console.log("\n=== TOF (Off-delay timer) ===\n");
    // =====================================================================

    test("TOF: initial state Q=false, ET=0", function() {
        var tof = new TOF(100);
        assertEqual(tof.Q, false);
        assertEqual(tof.ET, 0);
    });

    test("TOF: IN=true sets Q immediately", function() {
        var tof = new TOF(100);
        tof.update(true);
        assertEqual(tof.Q, true, "Q immediately true");
        assertEqual(tof.ET, 0);
    });

    test("TOF: Q stays true for PT after IN goes false", function() {
        var tof = new TOF(100);
        tof.update(true);
        tof.update(false);
        sleep(50);
        tof.update(false);
        assertEqual(tof.Q, true, "still true during delay");
        assert(tof.ET >= 40 && tof.ET <= 60, "ET should be ~50ms, got " + tof.ET);
    });

    test("TOF: Q becomes false after PT", function() {
        var tof = new TOF(80);
        tof.update(true);
        tof.update(false);
        sleep(100);
        tof.update(false);
        assertEqual(tof.Q, false, "should be false after PT");
        assertEqual(tof.ET, 80, "ET capped at PT");
    });

    test("TOF: IN=true during delay resets delay", function() {
        var tof = new TOF(100);
        tof.update(true);
        tof.update(false);
        sleep(50);
        tof.update(true); // re-activate during delay
        assertEqual(tof.Q, true);
        assertEqual(tof.ET, 0, "ET reset");
        tof.update(false); // start delay again
        sleep(50);
        tof.update(false);
        assertEqual(tof.Q, true, "new delay still running");
    });

    test("TOF: Q stays false if IN was never true", function() {
        var tof = new TOF(100);
        tof.update(false);
        assertEqual(tof.Q, false);
        sleep(50);
        tof.update(false);
        assertEqual(tof.Q, false, "never activated");
    });

    // =====================================================================
    console.log("\n=== TP (Pulse timer) ===\n");
    // =====================================================================

    test("TP: initial state Q=false, ET=0", function() {
        var tp = new TP(100);
        assertEqual(tp.Q, false);
        assertEqual(tp.ET, 0);
    });

    test("TP: rising edge starts pulse", function() {
        var tp = new TP(100);
        tp.update(true);
        assertEqual(tp.Q, true, "pulse started");
        assert(tp.ET >= 0 && tp.ET <= 5, "ET near 0");
    });

    test("TP: pulse lasts exactly PT", function() {
        var tp = new TP(80);
        tp.update(true);
        sleep(50);
        tp.update(true);
        assertEqual(tp.Q, true, "still pulsing");
        sleep(40);
        tp.update(true);
        assertEqual(tp.Q, false, "pulse ended");
        assertEqual(tp.ET, 80, "ET capped at PT");
    });

    test("TP: pulse runs to completion even if IN goes false", function() {
        var tp = new TP(100);
        tp.update(true);
        assertEqual(tp.Q, true);
        tp.update(false); // IN goes false during pulse
        sleep(50);
        tp.update(false);
        assertEqual(tp.Q, true, "pulse continues");
        sleep(60);
        tp.update(false);
        assertEqual(tp.Q, false, "pulse completed");
    });

    test("TP: ET continues to increase during pulse when IN=false", function() {
        var tp = new TP(200);
        tp.update(true);  // start pulse
        tp.update(false); // IN goes false during pulse
        sleep(80);
        tp.update(false);
        assertEqual(tp.Q, true, "pulse still active");
        assert(tp.ET >= 60 && tp.ET <= 100, "ET should track elapsed time (~80ms), got " + tp.ET);
    });

    test("TP: no re-trigger during active pulse", function() {
        var tp = new TP(100);
        tp.update(true);  // start pulse
        sleep(30);
        tp.update(false);
        tp.update(true);  // attempt re-trigger during pulse
        sleep(30);
        tp.update(true);
        assertEqual(tp.Q, true, "original pulse still running");
    });

    test("TP: no re-trigger when IN stays high after pulse ends", function() {
        var tp = new TP(60);
        tp.update(true);  // start pulse
        sleep(70);
        tp.update(true);  // pulse ended but IN still high
        assertEqual(tp.Q, false, "pulse ended");
        tp.update(true);  // IN still high - must NOT re-trigger
        assertEqual(tp.Q, false, "no re-trigger while IN stays high");
    });

    test("TP: can re-trigger after pulse ends and IN returns to false", function() {
        var tp = new TP(60);
        tp.update(true);
        sleep(70);
        tp.update(false); // pulse ended, IN=false
        assertEqual(tp.Q, false);
        tp.update(true);  // new rising edge
        assertEqual(tp.Q, true, "new pulse started");
    });

    test("TP: ET resets to 0 when IN goes false after pulse", function() {
        var tp = new TP(60);
        tp.update(true);
        sleep(70);
        tp.update(true);  // pulse ended, ET=PT
        assertEqual(tp.ET, 60, "ET capped at PT");
        tp.update(false); // IN goes false
        assertEqual(tp.ET, 0, "ET resets when IN goes false after pulse");
    });

    // =====================================================================
    console.log("\n=== IEC 61508 edge cases: Timers ===\n");
    // =====================================================================

    // --- TON boundary cases ---

    test("TON: PT=0 — Q immediately true on first update", function() {
        var ton = new TON(0);
        ton.update(true);
        assertEqual(ton.Q, true, "PT=0: Q should be true immediately");
        assertEqual(ton.ET, 0, "ET capped at PT=0");
    });

    test("TON: pulse exactly equal to PT", function() {
        var ton = new TON(50);
        ton.update(true);
        sleep(50);
        ton.update(true);
        assertEqual(ton.Q, true, "Q true at exactly PT");
        ton.update(false); // release
        assertEqual(ton.Q, false, "Q false after release");
        assertEqual(ton.ET, 0);
    });

    test("TON: very short pulse (shorter than PT) does not trigger", function() {
        var ton = new TON(100);
        ton.update(true);
        ton.update(false); // immediate release, ~0ms
        assertEqual(ton.Q, false, "not enough time");
        assertEqual(ton.ET, 0);
    });

    test("TON: multiple retriggers — each starts fresh", function() {
        var ton = new TON(80);
        // First attempt — abort
        ton.update(true);
        sleep(30);
        ton.update(false);
        // Second attempt — abort
        ton.update(true);
        sleep(30);
        ton.update(false);
        // Third attempt — let it complete
        ton.update(true);
        sleep(90);
        ton.update(true);
        assertEqual(ton.Q, true, "third attempt should succeed");
    });

    test("TON: IN held true long after PT — Q stays true, ET stays at PT", function() {
        var ton = new TON(50);
        ton.update(true);
        sleep(60);
        ton.update(true);
        assertEqual(ton.Q, true);
        assertEqual(ton.ET, 50, "ET capped at PT");
        sleep(50);
        ton.update(true);
        assertEqual(ton.Q, true, "still true");
        assertEqual(ton.ET, 50, "ET still capped");
    });

    // --- TOF boundary cases ---

    test("TOF: PT=0 — Q immediately false when IN goes false", function() {
        var tof = new TOF(0);
        tof.update(true);
        assertEqual(tof.Q, true);
        tof.update(false);
        assertEqual(tof.Q, false, "PT=0: Q immediately false");
        assertEqual(tof.ET, 0, "ET capped at PT=0");
    });

    test("TOF: very short IN pulse — delay still runs full PT", function() {
        var tof = new TOF(80);
        tof.update(true);
        tof.update(false); // immediate release
        sleep(40);
        tof.update(false);
        assertEqual(tof.Q, true, "delay still running");
        sleep(50);
        tof.update(false);
        assertEqual(tof.Q, false, "delay expired");
    });

    test("TOF: multiple rapid IN toggles during delay", function() {
        var tof = new TOF(100);
        tof.update(true);
        tof.update(false); // start delay
        sleep(30);
        tof.update(true);  // re-activate: resets delay
        assertEqual(tof.Q, true);
        assertEqual(tof.ET, 0, "ET reset on re-activation");
        tof.update(false); // restart delay
        sleep(30);
        tof.update(false);
        assertEqual(tof.Q, true, "new delay still running");
    });

    // --- TP boundary cases ---

    test("TP: PT=0 — pulse starts and ends in same cycle", function() {
        var tp = new TP(0);
        tp.update(true);
        // PT=0: pulse should complete immediately
        assertEqual(tp.ET, 0, "ET capped at PT=0");
        // Q may be true for one cycle then false, or false immediately
        // Standard allows either; key is no infinite pulse
        tp.update(false);
        tp.update(true); // should be able to retrigger
    });

    test("TP: rapid retrigger after completion", function() {
        var tp = new TP(50);
        tp.update(true);  // start pulse 1
        sleep(60);
        tp.update(false); // pulse done, IN=false
        assertEqual(tp.Q, false);
        tp.update(true);  // pulse 2
        assertEqual(tp.Q, true, "pulse 2 started");
        sleep(60);
        tp.update(false); // pulse 2 done
        assertEqual(tp.Q, false);
        tp.update(true);  // pulse 3
        assertEqual(tp.Q, true, "pulse 3 started");
    });

    test("TP: IN stays true across multiple PT periods — only one pulse", function() {
        var tp = new TP(40);
        tp.update(true);
        assertEqual(tp.Q, true, "pulse started");
        sleep(50);
        tp.update(true); // pulse ended but IN still true
        assertEqual(tp.Q, false, "pulse ended");
        sleep(50);
        tp.update(true); // still no retrigger
        assertEqual(tp.Q, false, "no retrigger while IN stays high");
    });

    // =====================================================================
    console.log("\n=== IEC 61508 edge cases: Counters ===\n");
    // =====================================================================

    test("CTU: PV=0 — Q true after first update", function() {
        var ctu = new CTU(0);
        ctu.update(false, false); // no count, just evaluate Q
        assertEqual(ctu.Q, true, "CV=0 >= PV=0");
    });

    test("CTU: PV=1 — single edge triggers Q", function() {
        var ctu = new CTU(1);
        assertEqual(ctu.Q, false);
        ctu.update(true, false);
        assertEqual(ctu.Q, true, "one edge reaches PV=1");
    });

    test("CTU: RESET while CU=true — RESET wins, edge consumed", function() {
        var ctu = new CTU(5);
        ctu.update(true, false); // count to 1
        ctu.update(false, false);
        ctu.update(true, true);  // CU rising + RESET simultaneously
        assertEqual(ctu.CV, 0, "RESET wins");
        ctu.update(false, false);
        ctu.update(true, false); // new edge
        assertEqual(ctu.CV, 1, "edge after RESET counts normally");
    });

    test("CTU: rapid toggling counts correctly", function() {
        var ctu = new CTU(10);
        for (var i = 0; i < 5; i++) {
            ctu.update(true, false);
            ctu.update(false, false);
        }
        assertEqual(ctu.CV, 5, "5 toggles = 5 counts");
    });

    test("CTD: PV=0 — Q true from start, LOAD sets CV=0", function() {
        var ctd = new CTD(0);
        assertEqual(ctd.Q, true, "CV=0 <= 0");
        ctd.update(false, true); // LOAD: CV=PV=0
        assertEqual(ctd.CV, 0);
        assertEqual(ctd.Q, true);
    });

    test("CTD: PV=1 — one count reaches zero", function() {
        var ctd = new CTD(1);
        ctd.update(false, true); // LOAD: CV=1
        assertEqual(ctd.Q, false);
        ctd.update(true, false); // count down
        assertEqual(ctd.CV, 0);
        assertEqual(ctd.Q, true);
    });

    test("CTUD: PV=0 — QU and QD true after first update", function() {
        var ctud = new CTUD(0);
        ctud.update(false, false, false, false);
        assertEqual(ctud.QU, true, "CV=0 >= PV=0");
        assertEqual(ctud.QD, true, "CV=0 <= 0");
    });

    test("CTUD: RESET during active counting", function() {
        var ctud = new CTUD(10);
        // Count up to 5
        for (var i = 0; i < 5; i++) {
            ctud.update(true, false, false, false);
            ctud.update(false, false, false, false);
        }
        assertEqual(ctud.CV, 5);
        // RESET
        ctud.update(false, false, true, false);
        assertEqual(ctud.CV, 0);
        assertEqual(ctud.QD, true);
    });

    test("CTUD: LOAD during active counting", function() {
        var ctud = new CTUD(10);
        ctud.update(true, false, false, false);
        ctud.update(false, false, false, false);
        assertEqual(ctud.CV, 1);
        ctud.update(false, false, false, true); // LOAD
        assertEqual(ctud.CV, 10);
        assertEqual(ctud.QU, true);
    });

    // =====================================================================
    console.log("\n=== IEC61131_STRICT mode ===\n");
    // =====================================================================

    test("STRICT: rejects non-boolean input to RS", function() {
        IEC61131_STRICT = true;
        var rs = new RS();
        var caught = false;
        try { rs.update(1, 0); } catch (e) { caught = true; }
        IEC61131_STRICT = false;
        assert(caught, "should throw TypeError for non-boolean");
    });

    test("STRICT: rejects NaN PT in TON constructor", function() {
        IEC61131_STRICT = true;
        var caught = false;
        try { new TON(NaN); } catch (e) { caught = true; }
        IEC61131_STRICT = false;
        assert(caught, "should throw RangeError for NaN");
    });

    test("STRICT: rejects negative PT in TON constructor", function() {
        IEC61131_STRICT = true;
        var caught = false;
        try { new TON(-100); } catch (e) { caught = true; }
        IEC61131_STRICT = false;
        assert(caught, "should throw RangeError for negative PT");
    });

    test("STRICT: rejects non-integer PV in CTU constructor", function() {
        IEC61131_STRICT = true;
        var caught = false;
        try { new CTU(3.5); } catch (e) { caught = true; }
        IEC61131_STRICT = false;
        assert(caught, "should throw RangeError for non-integer PV");
    });

    test("STRICT: rejects undefined args in CTUD.update", function() {
        IEC61131_STRICT = true;
        var ctud = new CTUD(5);
        var caught = false;
        try { ctud.update(true); } catch (e) { caught = true; }
        IEC61131_STRICT = false;
        assert(caught, "should throw TypeError for missing args");
    });

    test("STRICT: accepts correct types without error", function() {
        IEC61131_STRICT = true;
        var ton = new TON(100);
        ton.update(true);
        ton.update(false);
        var ctu = new CTU(5);
        ctu.update(true, false);
        var rs = new RS();
        rs.update(true, false);
        IEC61131_STRICT = false;
        assert(true, "no exceptions with correct types");
    });

    // =====================================================================
    // Results
    // =====================================================================
    console.log("\n=== IEC 61131-3 Test Results ===");
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

// Run tests on load
if (typeof module === 'undefined')
{
    var results = runIEC61131Tests();

    if (results.failed === 0)
        console.log("\nAll IEC 61131-3 tests passed!");
    else
        console.log("\nThere are failed IEC 61131-3 tests!");
}

// Export for use in other modules
if (typeof module !== 'undefined')
{
    module.exports = {
        runIEC61131Tests: runIEC61131Tests
    };
}
