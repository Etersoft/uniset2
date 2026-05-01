/*
 * Tests for uniset2-debug.js -- Debug visualizer runtime module
 *
 * Tests the debug module in isolation (no real HTTP server or step_cb).
 * Uses _debug_internals to access internal state and functions.
 */

load("uniset2-debug.js");

function runDebugTests()
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
            if (typeof print === 'function') print("  + " + name);
            else console.log("  + " + name);
        }
        catch (e)
        {
            testResults.tests.push({name: name, status: 'FAILED', error: e.message || String(e)});
            testResults.failed++;
            var msg = "  x " + name + " - " + (e.message || e);
            if (typeof print === 'function') print(msg);
            else console.log(msg);
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
                " - Expected: " + JSON.stringify(expected) + ", Got: " + JSON.stringify(actual));
    }

    function assertDeepEqual(actual, expected, message)
    {
        if (JSON.stringify(actual) !== JSON.stringify(expected))
            throw new Error((message || "Values not deep-equal") +
                " - Expected: " + JSON.stringify(expected) + ", Got: " + JSON.stringify(actual));
    }

    // Helper to make a fake req object
    function mkReq(opts)
    {
        opts = opts || {};
        var uri = opts.uri || opts.path || "/";
        var path = uri;
        var query = "";
        var qi = uri.indexOf("?");
        if (qi >= 0) { path = uri.slice(0, qi); query = uri.slice(qi + 1); }
        return {
            method: (opts.method || "GET").toUpperCase(),
            uri: uri,
            path: path,
            query: query,
            headers: opts.headers || {},
            body: opts.body || ""
        };
    }

    var internals = globalThis._debug_internals;

    // Reset state before each major test group
    function resetState()
    {
        internals.reset();
        // Clean up any test globals
        delete globalThis.in_TestSensor;
        delete globalThis.out_TestOutput;
        delete globalThis.in_Temp;
        delete globalThis.out_Heater;
        delete globalThis.myFB;
    }

    // ==================================================================
    // Module loading and exports
    // ==================================================================

    test("module version is defined", function ()
    {
        assertEqual(typeof unisetDebugModuleVersion, "string");
        assertEqual(unisetDebugModuleVersion, "v1");
    });

    test("uniset_debug_start is a function on globalThis", function ()
    {
        assertEqual(typeof globalThis.uniset_debug_start, "function");
    });

    test("uniset_debug_watch is a function on globalThis", function ()
    {
        assertEqual(typeof globalThis.uniset_debug_watch, "function");
    });

    test("trace functions are defined on globalThis", function ()
    {
        assertEqual(typeof globalThis._dbg_if, "function");
        assertEqual(typeof globalThis._dbg_case, "function");
        assertEqual(typeof globalThis._dbg_begin_cycle, "function");
        assertEqual(typeof globalThis._dbg_end_cycle, "function");
    });

    test("_debug_internals is exposed for testing", function ()
    {
        assert(internals !== undefined, "internals should be defined");
        assertEqual(typeof internals.collectSnapshot, "function");
        assertEqual(typeof internals.handleDebugRequest, "function");
        assertEqual(typeof internals.debugStepCb, "function");
    });

    // ==================================================================
    // Trace functions
    // ==================================================================

    test("_dbg_if returns condition unchanged (truthy)", function ()
    {
        resetState();
        var result = _dbg_if(42, true);
        assertEqual(result, true);
    });

    test("_dbg_if returns condition unchanged (falsy)", function ()
    {
        resetState();
        var result = _dbg_if(42, false);
        assertEqual(result, false);
    });

    test("_dbg_if records trace entry", function ()
    {
        resetState();
        _dbg_begin_cycle();
        _dbg_if(10, true);
        _dbg_if(20, false);
        var trace = internals.getTrace();
        assertEqual(trace.length, 2);
        assertEqual(trace[0].type, "if");
        assertEqual(trace[0].line, 10);
        assertEqual(trace[0].result, true);
        assertEqual(trace[1].line, 20);
        assertEqual(trace[1].result, false);
    });

    test("_dbg_case returns value unchanged", function ()
    {
        resetState();
        var result = _dbg_case(55, 3);
        assertEqual(result, 3);
    });

    test("_dbg_case records trace entry", function ()
    {
        resetState();
        _dbg_begin_cycle();
        _dbg_case(55, 2);
        var trace = internals.getTrace();
        assertEqual(trace.length, 1);
        assertEqual(trace[0].type, "case");
        assertEqual(trace[0].line, 55);
        assertEqual(trace[0].value, 2);
    });

    test("_dbg_begin_cycle clears trace buffer", function ()
    {
        resetState();
        _dbg_if(1, true);
        _dbg_if(2, false);
        assert(internals.getTrace().length > 0, "trace should have entries");
        _dbg_begin_cycle();
        assertEqual(internals.getTrace().length, 0);
    });

    test("_dbg_end_cycle is a no-op", function ()
    {
        resetState();
        _dbg_begin_cycle();
        _dbg_if(1, true);
        _dbg_end_cycle();
        // trace should still be readable
        assertEqual(internals.getTrace().length, 1);
    });

    // ==================================================================
    // Watch registration
    // ==================================================================

    test("uniset_debug_watch registers a variable", function ()
    {
        resetState();
        var myVal = 42;
        uniset_debug_watch("myVar", function () { return myVal; });
        var watched = internals.getWatched();
        assertEqual(typeof watched.myVar, "function");
        assertEqual(watched.myVar(), 42);
    });

    test("uniset_debug_watch rejects non-string name", function ()
    {
        resetState();
        var threw = false;
        try { uniset_debug_watch(123, function () { return 0; }); }
        catch (e) { threw = true; }
        assert(threw, "should throw for non-string name");
    });

    test("uniset_debug_watch rejects non-function getter", function ()
    {
        resetState();
        var threw = false;
        try { uniset_debug_watch("x", 42); }
        catch (e) { threw = true; }
        assert(threw, "should throw for non-function getter");
    });

    // ==================================================================
    // Variable auto-detection
    // ==================================================================

    test("collectSnapshot detects in_* globals", function ()
    {
        resetState();
        globalThis.in_TestSensor = 100;
        var snap = internals.collectSnapshot();
        assertEqual(snap.vars.in_TestSensor, 100);
        delete globalThis.in_TestSensor;
    });

    test("collectSnapshot detects out_* globals", function ()
    {
        resetState();
        globalThis.out_TestOutput = 50;
        var snap = internals.collectSnapshot();
        assertEqual(snap.vars.out_TestOutput, 50);
        delete globalThis.out_TestOutput;
    });

    test("collectSnapshot includes watched variables", function ()
    {
        resetState();
        var myVal = 99;
        uniset_debug_watch("myWatched", function () { return myVal; });
        var snap = internals.collectSnapshot();
        assertEqual(snap.vars.myWatched, 99);
    });

    test("collectSnapshot detects FB instances with _debug_meta", function ()
    {
        resetState();
        function FakeFB() { this.Q = true; this.ET = 1500; }
        FakeFB._debug_meta = {
            type: "timer",
            fields: { Q: { type: "bool" }, ET: { type: "time" } }
        };
        globalThis.myFB = new FakeFB();
        var snap = internals.collectSnapshot();
        assertEqual(snap.vars["myFB.Q"], true);
        assertEqual(snap.vars["myFB.ET"], 1500);
        delete globalThis.myFB;
    });

    // ==================================================================
    // Ring buffer
    // ==================================================================

    test("ringPush and ringEntries basic operation", function ()
    {
        resetState();
        internals.ringPush({ ts: 1000, cycle: 1, vars: { x: 1 } });
        internals.ringPush({ ts: 2000, cycle: 2, vars: { x: 2 } });
        var entries = internals.ringEntries();
        assertEqual(entries.length, 2);
        assertEqual(entries[0].vars.x, 1);
        assertEqual(entries[1].vars.x, 2);
    });

    test("ring buffer wraps correctly at history_depth", function ()
    {
        resetState();
        // Set small history depth
        internals.ringResize(3);
        internals.ringPush({ ts: 1, cycle: 1, vars: { x: 1 } });
        internals.ringPush({ ts: 2, cycle: 2, vars: { x: 2 } });
        internals.ringPush({ ts: 3, cycle: 3, vars: { x: 3 } });
        internals.ringPush({ ts: 4, cycle: 4, vars: { x: 4 } }); // wraps, oldest (1) removed
        var entries = internals.ringEntries();
        assertEqual(entries.length, 3);
        assertEqual(entries[0].vars.x, 2, "oldest should be x=2");
        assertEqual(entries[1].vars.x, 3);
        assertEqual(entries[2].vars.x, 4, "newest should be x=4");
    });

    test("ringEntries with depth limit", function ()
    {
        resetState();
        internals.ringResize(10);
        for (var i = 0; i < 5; i++) {
            internals.ringPush({ ts: i, cycle: i, vars: { x: i } });
        }
        var entries = internals.ringEntries(3);
        assertEqual(entries.length, 3);
        assertEqual(entries[0].vars.x, 2);
        assertEqual(entries[2].vars.x, 4);
    });

    test("ringResize preserves recent entries", function ()
    {
        resetState();
        internals.ringResize(10);
        for (var i = 0; i < 5; i++) {
            internals.ringPush({ ts: i, cycle: i, vars: { x: i } });
        }
        internals.ringResize(3); // shrink to 3
        var entries = internals.ringEntries();
        assertEqual(entries.length, 3);
        assertEqual(entries[0].vars.x, 2);
        assertEqual(entries[2].vars.x, 4);
    });

    // ==================================================================
    // step_cb (debugStepCb)
    // ==================================================================

    test("debugStepCb increments cycle counter", function ()
    {
        resetState();
        assertEqual(internals.getCycle(), 0);
        internals.debugStepCb();
        assertEqual(internals.getCycle(), 1);
        internals.debugStepCb();
        assertEqual(internals.getCycle(), 2);
    });

    test("debugStepCb applies forced values to globals", function ()
    {
        resetState();
        globalThis.in_Temp = 25;
        // Simulate forcing via internals
        var forced = internals.getForced();
        forced["in_Temp"] = 99;
        internals.debugStepCb();
        assertEqual(globalThis.in_Temp, 99);
        delete globalThis.in_Temp;
    });

    test("debugStepCb captures ring buffer entry after first cycle", function ()
    {
        resetState();
        globalThis.in_Temp = 10;
        internals.debugStepCb(); // cycle 1, no ring entry (cycle > 1 check)
        assertEqual(internals.getRingBuffer().length, 0);
        globalThis.in_Temp = 20;
        internals.debugStepCb(); // cycle 2, captures ring entry
        assertEqual(internals.getRingBuffer().length, 1);
        delete globalThis.in_Temp;
    });

    // ==================================================================
    // HTTP endpoint: GET /debug/snapshot
    // ==================================================================

    test("GET /debug/snapshot returns valid JSON structure", function ()
    {
        resetState();
        globalThis.in_Temp = 42;
        internals.debugStepCb(); // set cycle to 1
        var resp = internals.handleDebugRequest(mkReq({ path: "/debug/snapshot" }));
        assertEqual(resp.status, 200);
        var body = JSON.parse(resp.body);
        assertEqual(body.cycle, 1);
        assert(body.ts > 0, "ts should be positive");
        assertEqual(body.vars.in_Temp, 42);
        assert(Array.isArray(body.trace), "trace should be array");
        assert(Array.isArray(body.forced), "forced should be array");
        assert(typeof body.cycles_missed === 'number', "cycles_missed should be number");
        delete globalThis.in_Temp;
    });

    // ==================================================================
    // HTTP endpoint: GET /debug/history
    // ==================================================================

    test("GET /debug/history returns variable data from ring buffer", function ()
    {
        resetState();
        internals.ringResize(100);
        globalThis.in_Temp = 10;
        internals.debugStepCb(); // cycle 1
        globalThis.in_Temp = 20;
        internals.debugStepCb(); // cycle 2, captures ring for cycle 1 state
        globalThis.in_Temp = 30;
        internals.debugStepCb(); // cycle 3, captures ring for cycle 2 state

        var resp = internals.handleDebugRequest(mkReq({ path: "/debug/history?var=in_Temp" }));
        assertEqual(resp.status, 200);
        var body = JSON.parse(resp.body);
        assertEqual(body['var'], "in_Temp");
        assert(Array.isArray(body.data), "data should be array");
        assert(body.data.length > 0, "should have history entries");
        // Each entry is [timestamp, value]
        assertEqual(body.data[0].length, 2);
        delete globalThis.in_Temp;
    });

    test("GET /debug/history returns 400 without var param", function ()
    {
        resetState();
        var resp = internals.handleDebugRequest(mkReq({ path: "/debug/history" }));
        assertEqual(resp.status, 400);
        var body = JSON.parse(resp.body);
        assert(body.error.indexOf("var") >= 0, "error should mention 'var'");
    });

    // ==================================================================
    // HTTP endpoint: GET /debug/info
    // ==================================================================

    test("GET /debug/info returns server info", function ()
    {
        resetState();
        var resp = internals.handleDebugRequest(mkReq({ path: "/debug/info" }));
        assertEqual(resp.status, 200);
        var body = JSON.parse(resp.body);
        assertEqual(body.version, "v1");
        assertEqual(typeof body.history_depth, "number");
        assertEqual(typeof body.uptime_ms, "number");
        assertEqual(typeof body.vars_count, "number");
    });

    test("GET /debug/info includes program name from _program_meta", function ()
    {
        resetState();
        globalThis._program_meta = { name: "TestProgram" };
        var resp = internals.handleDebugRequest(mkReq({ path: "/debug/info" }));
        var body = JSON.parse(resp.body);
        assertEqual(body.program, "TestProgram");
        delete globalThis._program_meta;
    });

    // ==================================================================
    // HTTP endpoint: POST /debug/force
    // ==================================================================

    test("POST /debug/force sets a forced variable", function ()
    {
        resetState();
        var resp = internals.handleDebugRequest(mkReq({
            method: "POST",
            path: "/debug/force",
            body: JSON.stringify({ "var": "in_Temp", value: 99 })
        }));
        assertEqual(resp.status, 200);
        var body = JSON.parse(resp.body);
        assertEqual(body.ok, true);
        var forced = internals.getForced();
        assertEqual(forced["in_Temp"], 99);
    });

    test("POST /debug/force returns 400 for missing var", function ()
    {
        resetState();
        var resp = internals.handleDebugRequest(mkReq({
            method: "POST",
            path: "/debug/force",
            body: JSON.stringify({ value: 99 })
        }));
        assertEqual(resp.status, 400);
    });

    test("POST /debug/force returns 400 for missing value", function ()
    {
        resetState();
        var resp = internals.handleDebugRequest(mkReq({
            method: "POST",
            path: "/debug/force",
            body: JSON.stringify({ "var": "in_Temp" })
        }));
        assertEqual(resp.status, 400);
    });

    // ==================================================================
    // HTTP endpoint: POST /debug/unforce
    // ==================================================================

    test("POST /debug/unforce releases a forced variable", function ()
    {
        resetState();
        internals.getForced()["in_Temp"] = 99;
        var resp = internals.handleDebugRequest(mkReq({
            method: "POST",
            path: "/debug/unforce",
            body: JSON.stringify({ "var": "in_Temp" })
        }));
        assertEqual(resp.status, 200);
        assertEqual(internals.getForced()["in_Temp"], undefined);
    });

    test("POST /debug/unforce returns 400 for missing var", function ()
    {
        resetState();
        var resp = internals.handleDebugRequest(mkReq({
            method: "POST",
            path: "/debug/unforce",
            body: JSON.stringify({})
        }));
        assertEqual(resp.status, 400);
    });

    // ==================================================================
    // HTTP endpoint: POST /debug/config
    // ==================================================================

    test("POST /debug/config changes history_depth", function ()
    {
        resetState();
        var resp = internals.handleDebugRequest(mkReq({
            method: "POST",
            path: "/debug/config",
            body: JSON.stringify({ history_depth: 500 })
        }));
        assertEqual(resp.status, 200);
        assertEqual(internals.getConfig().history_depth, 500);
    });

    test("POST /debug/config changes trace_enabled", function ()
    {
        resetState();
        var resp = internals.handleDebugRequest(mkReq({
            method: "POST",
            path: "/debug/config",
            body: JSON.stringify({ trace_enabled: false })
        }));
        assertEqual(resp.status, 200);
        assertEqual(internals.getConfig().trace_enabled, false);
    });

    test("POST /debug/config rejects invalid history_depth", function ()
    {
        resetState();
        var resp = internals.handleDebugRequest(mkReq({
            method: "POST",
            path: "/debug/config",
            body: JSON.stringify({ history_depth: -1 })
        }));
        assertEqual(resp.status, 400);
    });

    // ==================================================================
    // HTTP endpoint: GET /debug/ui
    // ==================================================================

    test("GET /debug/ui returns HTML placeholder", function ()
    {
        resetState();
        var resp = internals.handleDebugRequest(mkReq({ path: "/debug/ui" }));
        assertEqual(resp.status, 200);
        assert(resp.headers['Content-Type'].indexOf('text/html') >= 0, "should be HTML");
        assert(resp.body.indexOf('Debug UI placeholder') >= 0, "should contain placeholder text");
    });

    // ==================================================================
    // HTTP 404 for unknown debug paths
    // ==================================================================

    test("unknown /debug/ path returns 404", function ()
    {
        resetState();
        var resp = internals.handleDebugRequest(mkReq({ path: "/debug/unknown" }));
        assertEqual(resp.status, 404);
    });

    test("non-debug path returns null (pass-through)", function ()
    {
        resetState();
        var resp = internals.handleDebugRequest(mkReq({ path: "/api/data" }));
        assertEqual(resp, null);
    });

    // ==================================================================
    // Force mechanism integration with step_cb
    // ==================================================================

    test("force + step_cb writes forced value to global before on_step", function ()
    {
        resetState();
        globalThis.out_Heater = 0;

        // Force the variable via HTTP
        internals.handleDebugRequest(mkReq({
            method: "POST",
            path: "/debug/force",
            body: JSON.stringify({ "var": "out_Heater", value: 1 })
        }));

        // Run step_cb (simulates before on_step)
        internals.debugStepCb();

        // The forced value should be applied
        assertEqual(globalThis.out_Heater, 1);

        // Unforce
        internals.handleDebugRequest(mkReq({
            method: "POST",
            path: "/debug/unforce",
            body: JSON.stringify({ "var": "out_Heater" })
        }));

        // Next step_cb should not change it
        globalThis.out_Heater = 0;
        internals.debugStepCb();
        assertEqual(globalThis.out_Heater, 0);

        delete globalThis.out_Heater;
    });

    // ==================================================================
    // Snapshot forced list
    // ==================================================================

    test("snapshot includes forced variable names", function ()
    {
        resetState();
        globalThis.in_Temp = 10;
        internals.getForced()["in_Temp"] = 99;
        internals.debugStepCb();
        var snap = internals.collectSnapshot();
        assert(snap.forced.indexOf("in_Temp") >= 0, "forced list should contain in_Temp");
        delete globalThis.in_Temp;
    });

    // ==================================================================
    // Trace disabled
    // ==================================================================

    test("trace not collected when trace_enabled is false", function ()
    {
        resetState();
        internals.getConfig().trace_enabled = false;
        _dbg_begin_cycle();
        _dbg_if(1, true);
        _dbg_case(2, 5);
        assertEqual(internals.getTrace().length, 0);
    });

    // ==================================================================
    // Snapshot cycles_missed counter
    // ==================================================================

    test("cycles_missed counts cycles since last snapshot request", function ()
    {
        resetState();
        internals.debugStepCb(); // cycle 1
        internals.debugStepCb(); // cycle 2
        internals.debugStepCb(); // cycle 3

        var snap1 = internals.collectSnapshot();
        assertEqual(snap1.cycles_missed, 3);

        internals.debugStepCb(); // cycle 4
        var snap2 = internals.collectSnapshot();
        assertEqual(snap2.cycles_missed, 1);
    });

    // ==================================================================
    // Summary
    // ==================================================================

    var msg = "Debug tests: " + testResults.passed + " passed, " + testResults.failed + " failed";
    if (typeof print === 'function') print("\n=== " + msg + " ===");
    else console.log("\n=== " + msg + " ===");

    return testResults;
}
