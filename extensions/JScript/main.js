load("uniset2-timers.js");
load("uniset2-log.js");
load("local.js");
load("uniset2-delaytimer.js");
load("my-http-api.js");
load("uniset2-simitator.js");
load("uniset2-modbus-client.js");
load("uniset2-opcua-client.js");
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
const simitator = createSimitator({
    sensors: ["JS_AI1_AS"],
    min: 0,
    max: 100,
    step: 5,
    pause: 500,
    func: (v) => Math.round(v * Math.sin(v))
});
// ----------------------------------------------------------------------------
const dt = new DelayTimer(200, 50);
const pt = new PassiveTimer(200);
const mbclient = uniset_createModbusClient();
const opcua = uniset_createOpcuaClient();
// ----------------------------------------------------------------------------
// HTTP server
startMyHttpServer();
// ----------------------------------------------------------------------------
function uniset_on_sensor( id, value, name )
{
    mylog.info("OnSensor:", id, value, name)
}
// ----------------------------------------------------------------------------
function uniset_on_step()
{
    MyCounter++;
    testLocalFunc(MyCounter);

    out_JS_AO1_C = in_JS_AI1_AS + 1;
    MyGlobal = in_JS_AI1_AS;


    mylog.info("Modbus demo read03...");
    const reply = mbclient.read03({ slave: 1, reg: 0, count: 2 });
    mylog.info("Modbus demo read03:", reply);
    const reply4 = mbclient.read04({ slave: 1, reg: 0, count: 2 });
    mylog.info("Modbus demo read04:", reply4);
    const replyF = mbclient.write0F({ slave: 1, reg: 0, values: [1, 0, 1, 0] });
    mylog.info("Modbus demo write0F:", replyF);

    try
    {
        const opcValues = opcua.readValues(["ns=0;i=1001", "ns=0;i=1002"]);
        mylog.info("OPC UA demo readValues:", opcValues);
    }
    catch( e )
    {
        mylog.warn("OPC UA read failed:", e.message || e);
    }

}
// ----------------------------------------------------------------------------
function uniset_on_stop()
{
    mylog.info("=== STOP ===");
    simitator.stop();
    mbclient.disconnect();

    if( opcua )
        opcua.disconnect();
}
// ----------------------------------------------------------------------------
function uniset_on_start() {

    mylog.info("=== START ===");

    simitator.start();
    mylog.info("simitator example started for JS_AI1_AS");

    mylog.info("getValue by id: ", ui.getValue(9));
    mylog.info("getValue by name: ", ui.getValue("JS_AI1_AS"));
    mylog.info("setValue: ", ui.setValue(9, 100));
    mylog.info("askSensor: ", ui.askSensor(9,UIONotify));

    // Таймер с бесконечными повторениями
    askTimer("test2", 1000, TimerInfinity, function(id, count) {
        mylog.info("🔄 Test timer 2 triggered! id:", id, "count:", count);
        out_JS_AO2_C = out_JS_AO2_C + 1;
    });


    try
    {
        mylog.info("Modbus client try connect..");
        mbclient.connectTCP({ host: "127.0.0.1", port: 2048, timeout: 2000});
    }
    catch( e )
    {
        mylog.crit("Modbus demo failed:", e.message);
    }

    try
    {
        if( opcua )
        {
            mylog.info("OPC UA client try connect..");
            opcua.connect({endpoint: "opc.tcp://127.0.0.1:4840"});
            opcua.write({"ns=0;i=1001": 42});
            mylog.info("OPC UA demo write ok");
        }
    }
    catch( e )
    {
        mylog.warn("OPC UA demo failed:", e.message || e);
    }

}
// ----------------------------------------------------------------------------
