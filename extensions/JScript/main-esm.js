import * as Timers from "uniset2-timers.esm.js";
import * as Log from "uniset2-log.esm.js";
import { testLocalFunc } from "local.esm.js";
import * as dtimer from "uniset2-delaytimer.esm.js";
import * as ptimer from "uniset2-passivetimer.esm.js";
// ----------------------------------------------------------------------------
globalThis.u = globalThis;
// ----------------------------------------------------------------------------
u.uniset_inputs = [
    { name: "JS_AI1_AS" },
    { name: "JS_AI2_AS" }
];
// ----------------------------------------------------------------------------
u.uniset_outputs = [
    { name: "JS_AO1_C" },
    { name: "JS_AO2_C" }
];
// ----------------------------------------------------------------------------
const mylog = Log.uniset_log_create("main.esm.js", true, true);
mylog.level("info","warn","crit", "level5")
// ----------------------------------------------------------------------------
const dt = new dtimer.DelayTimer(200, 50);
const pt = new ptimer.PassiveTimer(100);
var MyGlobal = 0;
var MyCounter = 0;
// ----------------------------------------------------------------------------
u.uniset_on_sensor = function uniset_on_sensor( id, value )
{
    mylog.info("OnSensor:", id, value)
}
// ----------------------------------------------------------------------------
u.uniset_on_step = function uniset_on_step()
{
    MyCounter++;
    testLocalFunc(MyCounter);

    u.out_JS_AO1_C = u.in_JS_AI1_AS + 1;
    MyGlobal = u.in_JS_AI1_AS;
}
// ----------------------------------------------------------------------------
u.uniset_on_stop = function uniset_on_stop()
{
    mylog.info("=== STOP ===");
    clearAllTimers();
}
// ----------------------------------------------------------------------------
u.uniset_on_start = function uniset_on_start() {
    mylog.info("=== START ===");

    mylog.info("getValue: ", ui.getValue(9));
    mylog.info("setValue: ", ui.setValue(9, 100));
    mylog.info("askSensor: ", ui.askSensor(9,UIONotify));
    mylog.info("getValue: ", ui.getValue("JS_AI1_AS"));

    // Таймер с несколькими повторениями
    askTimer("timer1", 3000, 3, function(id, count) {
        mylog.info("✅ Test timer 1 triggered! id:", id, "count:", count);
        u.out_JS_AO2_C = 100;
    });
}
// ----------------------------------------------------------------------------
function main() {
    mylog.info("pt.check", pt.checkTime())
    mylog.info("dt.check", dt.check(true))
}
// ----------------------------------------------------------------------------
main();
