load("uniset2-timers.js");
load("uniset2-log.js");
load("local.js");
load("uniset2-delaytimer.js");
load("my-http-api.js");
// ----------------------------------------------------------------------------
uniset_inputs = [
    { name: "JS_AI1_AS" },
    { name: "JS_AI2_AS" }
];
// ----------------------------------------------------------------------------
uniset_outputs = [
    { name: "JS_AO1_C" },
    { name: "JS_AO2_C" }
];
// ----------------------------------------------------------------------------
MyGlobal = 0;
MyCounter = 0;
mylog = uniset_log_create("main.js", true, true, true);
mylog.level("info","warn","crit", "level5")
// ----------------------------------------------------------------------------
const dt = new DelayTimer(200, 50);
const pt = new PassiveTimer(200);
// ----------------------------------------------------------------------------
startMyHttpServer();
// ----------------------------------------------------------------------------
function uniset_on_sensor( id, value )
{
    mylog.info("OnSensor:", id, value)
}
// ----------------------------------------------------------------------------
function uniset_on_step()
{
    MyCounter++;
    testLocalFunc(MyCounter);

    out_JS_AO1_C = in_JS_AI1_AS + 1;
    MyGlobal = in_JS_AI1_AS;
}
// ----------------------------------------------------------------------------
function uniset_on_stop()
{
    mylog.info("=== STOP ===");
}
// ----------------------------------------------------------------------------
function uniset_on_start() {

    mylog.info("=== START ===");

    mylog.info("getValue by id: ", ui.getValue(9));
    mylog.info("getValue by name: ", ui.getValue("JS_AI1_AS"));
    mylog.info("setValue: ", ui.setValue(9, 100));
    mylog.info("askSensor: ", ui.askSensor(9,UIONotify));

    // Таймер с бесконечными повторениями
    askTimer("test2", 1000, TimerInfinity, function(id, count) {
        mylog.info("🔄 Test timer 2 triggered! id:", id, "count:", count);
        out_JS_AO2_C = out_JS_AO2_C + 1;
    });
}
// ----------------------------------------------------------------------------
