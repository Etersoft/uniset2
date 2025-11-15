/*
 * Tests for uniset2-simitator.js module
 */

load("uniset2-simitator.js");

(function(global) {
    function createTestHarness() {
        var results = { passed: 0, failed: 0, tests: [] };

        function log(msg) {
            if (typeof print === "function")
                print(msg);
            else
                console.log(msg);
        }

        function addResult(name, ok, err) {
            if (ok) {
                results.passed++;
                results.tests.push({ name: name, status: "PASSED" });
                log("✓ " + name);
            } else {
                results.failed++;
                results.tests.push({ name: name, status: "FAILED", error: err });
                log("✗ " + name + " - " + err);
            }
        }

        function test(name, fn) {
            try {
                fn();
                addResult(name, true);
            } catch (e) {
                addResult(name, false, e && e.message ? e.message : String(e));
            }
        }

        function assert(cond, msg) {
            if (!cond)
                throw new Error(msg || "Assertion failed");
        }

        function assertEqual(actual, expected, msg) {
            if (actual !== expected)
                throw new Error((msg || "Values not equal") + " - expected: " + expected + " got: " + actual);
        }

        return {
            results: results,
            test: test,
            assert: assert,
            assertEqual: assertEqual
        };
    }

    function runSimitatorTests() {
        var harness = createTestHarness();
        var test = harness.test;
        var assert = harness.assert;
        var assertEqual = harness.assertEqual;

        if (!global.ui || typeof global.ui.setValue !== "function")
            throw new Error("uniset2-simitator tests require global ui");

        var realUi = global.ui;
        if (!realUi || typeof realUi.setValue !== "function")
            throw new Error("uniset2-simitator tests require global ui");

        if (typeof global.JS1_S === "undefined" || typeof global.JS2_C === "undefined")
            throw new Error("uniset2-simitator tests require JS1_S and JS2_C IDs in main-test.js");

        var sidJS1_S = global.JS1_S;
        var sidJS2_C = global.JS2_C;
        var savedSetValue = realUi.setValue;
        var savedDateNow = Date.now;
        var savedMathRandom = Math.random;

        var recordedCalls = [];
        var trackedSensors = null;
        var fakeNow = 0;

        function advance(sim, ms) {
            fakeNow += ms;

            if (sim && typeof sim.stepHandler === "function")
                sim.stepHandler();
        }

        function resetEnv(watchList) {
            recordedCalls = [];
            trackedSensors = watchList ? watchList.slice() : null;
            fakeNow = 0;
        }

        function matchesSensor(sensor) {
            if (!trackedSensors)
                return true;

            for (var i = 0; i < trackedSensors.length; i++)
            {
                if (trackedSensors[i] === sensor)
                    return true;
            }

            return false;
        }

        function getRecorded() {
            return recordedCalls.slice();
        }

        try {
            realUi.setValue = function(sensor, value) {
                if (matchesSensor(sensor))
                    recordedCalls.push({ sensor: sensor, value: value });

                return savedSetValue.call(realUi, sensor, value);
            };

            Date.now = function() {
                return fakeNow;
            };
            Math.random = function() {
                return 0.42;
            };

            test("basic waveform with bouncing", function() {
                resetEnv([sidJS1_S]);
                var sim = createSimitator({
                    sensors: [sidJS1_S],
                    min: 0,
                    max: 20,
                    step: 10,
                    pause: 100,
                    autoStart: false
                });

                sim.start();
                assert(sim.isRunning(), "Simulator must be running after start()");

                advance(sim, 50);
                assertEqual(getRecorded().length, 0, "Should not tick before pause");

                advance(sim, 120);
                advance(sim, 120);
                advance(sim, 120);

                var values = getRecorded().map(function(r) {
                    return r.value;
                });

                assertEqual(values.length, 3, "Should produce three values");
                assertEqual(values[0], 10, "First tick must step to min+step");
                assertEqual(values[1], 20, "Second tick must reach max");
                assertEqual(values[2], 10, "Third tick must bounce back");
                assertEqual(realUi.getValue(sidJS1_S), values[values.length - 1],
                    "Real UI value must match last simulated value");

                sim.stop();
                advance(sim, 150);
                assertEqual(getRecorded().length, 3, "Stop must prevent further ticks");
            });

            test("supports sin transform", function() {
                resetEnv([sidJS2_C]);

                var sim = createSimitator({
                    sensors: [sidJS2_C],
                    min: 0,
                    max: 10,
                    step: 10,
                    pause: 50,
                    func: "sin"
                });

                sim.start();
                advance(sim, 60);
                sim.stop();
                assertEqual(getRecorded().length, 1, "Should produce one value");
                var expected = Math.round(10 * Math.sin(10));
                assertEqual(getRecorded()[0].value, expected, "sin transform must be applied");
                assertEqual(realUi.getValue(sidJS2_C), expected,
                    "Real UI value must equal sin-transformed value");
            });

            test("supports custom function callback", function() {
                resetEnv([sidJS2_C]);

                var sim = createSimitator({
                    sensors: [sidJS2_C],
                    min: 0,
                    max: 10,
                    step: 10,
                    pause: 40,
                    func: function(value) {
                        return value + 123;
                    }
                });

                sim.start();
                advance(sim, 50);
                sim.stop();

                assertEqual(getRecorded().length, 1, "Should emit value with custom callback");
                var customExpected = 10 + 123;
                assertEqual(getRecorded()[0].value, customExpected, "Custom callback must be applied");
                assertEqual(realUi.getValue(sidJS2_C), customExpected,
                    "Real UI value must match custom callback result");
            });

            test("sid parsing creates entries for every sensor", function() {
                resetEnv([sidJS1_S, sidJS2_C]);

                var sim = createSimitator({
                    sid: sidJS1_S + "," + sidJS2_C,
                    min: 0,
                    max: 5,
                    step: 5,
                    pause: 30,
                    autoStart: false
                });

                sim.start();
                advance(sim, 40);
                sim.stop();
                var records = getRecorded();
                var seen = records.map(function(r) { return r.sensor; });

                assertEqual(seen.length, 2, "Must emit value for each sensor in SID list");
                assert(seen.indexOf(sidJS1_S) !== -1, "Numeric SID must be preserved");
                assert(seen.indexOf(sidJS2_C) !== -1, "String SID must be preserved");

                var latest = {};
                for (var i = 0; i < records.length; i++)
                    latest[records[i].sensor] = records[i].value;

                assertEqual(realUi.getValue(sidJS1_S), latest[sidJS1_S],
                    "JS1_S must hold last simulated value");
                assertEqual(realUi.getValue(sidJS2_C), latest[sidJS2_C],
                    "JS2_C must hold last simulated value");
            });

            test("invalid configuration throws error", function() {
                resetEnv();
                var threw = false;

                try
                {
                    createSimitator({
                        sensors: [sidJS1_S],
                        step: 0
                    });
                }
                catch(e)
                {
                    threw = true;
                }

                assert(threw, "Creating simulator with step=0 must throw");
            });
        }
        finally
        {
            realUi.setValue = savedSetValue;
            Date.now = savedDateNow;
            Math.random = savedMathRandom;
        }

        return harness.results;
    }

    global.runSimitatorTests = runSimitatorTests;
})(typeof globalThis !== "undefined" ? globalThis : this);
