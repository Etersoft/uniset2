load("uniset2-log.js")

ModulesTestsRunning = false

uniset_inputs = [
    { name: "JS1_S" },
    { name: "JS_StartModulesTests_S" },
    { name: "UI_TestSensor1" },
    { name: "UI_TestSensor2" },
    { name: "UI_TestCommand_S" }
];

uniset_outputs = [
    { name: "JS_Start_C" },
    { name: "JS_Stop_C" },
    { name: "JS2_C" },
    { name: "JS3_C" },
    { name: "JS4_C" },
    { name: "JS_ModulesTestsResult_C" },
    { name: "UI_TestOutput1" },
    { name: "UI_TestOutput2" },
    { name: "UI_TestResult_C" }
];

// Переменные для тестов ui объекта
let uiTestInitialized = false;
let uiTestSensor1Value = 0;
let uiTestSensor2Value = 0;
let currentTestCommand = 0;
let testCompleted = false;
let simExample = null;

// Логгер для отладки
let testLog = uniset_log_create("ui-tests", true, true);
testLog.level("info", "warn", "crit");

function uniset_on_start() {
    out_JS_Start_C = 10;
    initUiTests();
}

function initUiTests() {
    ui.askSensor(UI_TestSensor1, UIONotify);
    ui.askSensor(UI_TestSensor2, UIONotify);
    ui.askSensor(UI_TestCommand_S, UIONotify);
    uiTestInitialized = true;
    testLog.info("UI tests initialized");
}

function uniset_stop()
{
    out_JS_Stop_C = 15;
}

function uniset_on_step()
{
    out_JS2_C = in_JS1_S;
}

function uniset_on_sensor(id, value)
{
    if (id === JS_StartModulesTests_S && value > 0 && !ModulesTestsRunning) {
        ModulesTestsRunning = true;
        // out_JS_ModulesTestsResult_C = runAllUniset2Tests();
        out_JS_ModulesTestsResult_C = true;
    }

    // Обработка тестовых датчиков для ui объекта
    if (id === UI_TestSensor1 ) {
        uiTestSensor1Value = value;
        testLog.info("UI_TestSensor1 changed to:", value);
        // Если активен тест 5 (askSensor reaction), обновляем выход
        if (currentTestCommand === 5 ) {
            out_UI_TestOutput1 = value;
            testLog.info("Test 5: set UI_TestOutput1 to:", value);
        }
    }

    if (id === UI_TestSensor2) {
        uiTestSensor2Value = value;
        testLog.info("UI_TestSensor2 changed to:", value);
    }

    if (id === UI_TestCommand_S && value !== currentTestCommand) {
        // Новая команда теста
        currentTestCommand = value;
        testCompleted = false;
        out_UI_TestResult_C = 0; // Сбрасываем результат перед новым тестом
        testLog.info("Received test command:", value);

        // Запускаем тест только если команда не 0
        if (value > 0) {
            runUiTest(value);
            testCompleted = true;
        }
    }
}

function runUiTest(testCommand) {
    testLog.info("Running UI test:", testCommand);

    switch(testCommand) {
        case 15:
        {
            testLog.info("Test 15: OPC UA client integration");
            load("tests-uniset2-opcua.js")
            const opcRes = runOpcuaIntegrationTest(OPCUA_TEST_ENDPOINT);

            if( opcRes.success )
            {
                out_UI_TestOutput1 = opcRes.intValue;
                out_UI_TestOutput2 = opcRes.boolValue;
                out_UI_TestResult_C = 1;
                testLog.info("Test 15: OPC UA values int=", opcRes.intValue,
                             " bool=", opcRes.boolValue);
            }
            else
            {
                out_UI_TestOutput1 = 0;
                out_UI_TestOutput2 = 0;
                out_UI_TestResult_C = -1;
                testLog.warn("Test 15: OPC UA error", opcRes.error);
            }
            break;
        }

        default:
            testLog.warn("Unknown test command:", testCommand);
            break;
    }
}
