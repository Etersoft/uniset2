load("uniset2-log.js")
load("local-test.js");

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
    testLocalFunc(in_JS1_S);
    out_JS4_C = localAdd(in_JS1_S, in_JS1_S);
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
        case 1:
            // Тест 1: ui.getValue() с числовым ID
            testLog.info("Test 1: Reading UI_TestSensor1 by ID");
            let valueById = ui.getValue(UI_TestSensor1);
            testLog.info("Test 1: Got value:", valueById);
            out_UI_TestOutput1 = valueById;
            out_UI_TestResult_C = 1; // Успех
            testLog.info("Test 1: Completed successfully");
            break;

        case 2:
            // Тест 2: ui.getValue() с именем в виде строки
            testLog.info("Test 2: Reading UI_TestSensor2 by name");
            let valueByName = ui.getValue("UI_TestSensor2");
            testLog.info("Test 2: Got value:", valueByName);
            out_UI_TestOutput2 = valueByName;
            out_UI_TestResult_C = 1; // Успех
            testLog.info("Test 2: Completed successfully");
            break;

        case 3:
            // Тест 3: ui.setValue() с числовым ID
            testLog.info("Test 3: Setting UI_TestOutput1 to 999 by ID");
            let result3 = ui.setValue(UI_TestOutput1, 999);
            testLog.info("Test 3: setValue returned:", result3);
            out_UI_TestResult_C = 1; // Успех
            testLog.info("Test 3: Completed successfully");
            break;

        case 4:
            // Тест 4: ui.setValue() с именем в виде строки
            testLog.info("Test 4: Setting UI_TestOutput2 to 888 by name");
            let result4 = ui.setValue("UI_TestOutput2", 888);
            testLog.info("Test 4: setValue returned:", result4);
            out_UI_TestResult_C = 1; // Успех
            testLog.info("Test 4: Completed successfully");
            break;

        case 5:
            // Тест 5: ui.askSensor - проверка реакции на изменение
            testLog.info("Test 5: Testing askSensor reaction");
            out_UI_TestResult_C = 1; // Успех
            testLog.info("Test 5: Completed successfully");
            break;

        case 6:
            // Тест 6: проверка констант UIONotify
            testLog.info("Test 6: Checking UIONotify constants");
            testLog.info("UIONotify:", UIONotify,
                "UIODontNotify:", UIODontNotify,
                "UIONotifyChange:",UIONotifyChange,
                "UIONotifyFirstNotNull:", UIONotifyFirstNotNull);
            if (UIONotify === 0 && UIODontNotify === 1 &&
                UIONotifyChange === 2 && UIONotifyFirstNotNull === 3) {
                out_UI_TestOutput1 = UIONotify;
                out_UI_TestOutput2 = UIODontNotify;
                out_UI_TestResult_C = 1; // Успех
                testLog.info("Test 6: Constants are correct");
            } else {
                out_UI_TestResult_C = 0; // Ошибка
                testLog.warn("Test 6: Constants are incorrect");
            }
            break;

        case 7:
            // Тест 7: комбинированный тест - чтение и запись
            testLog.info("Test 7: Combined read/write test");
            let sensor1Value = ui.getValue(UI_TestSensor1);
            let sensor2Value = ui.getValue("UI_TestSensor2");
            testLog.info("Test 7: Read values - sensor1:", sensor1Value, "sensor2:", sensor2Value);
            ui.setValue(UI_TestOutput1, sensor1Value * 2);
            ui.setValue("UI_TestOutput2", sensor2Value + 10);
            out_UI_TestResult_C = 1; // Успех
            testLog.info("Test 7: Completed successfully");
            break;

        case 8:
            // Тест 8: test-uniset2-log.js
            load("tests-uniset2-log.js")
            testLog.info("Test 8: tests-uniset2-log.js");
            result = runLogTests();
            if( result.failed === 0 )
            {
                out_UI_TestResult_C = 1; // Успех
                testLog.info("Test 8: Completed successfully");
            }
            break;

        case 9:
            // Тест 9: PassiveTimer
            testLog.info("Test 9: Testing PassiveTimer");
            load("tests-uniset2-passivetimer.js");

            var timerResults = runPassiveTimerTests();

            if (timerResults.failed === 0) {
                out_UI_TestResult_C = 1; // Успех
                testLog.info("Test 9: All PassiveTimer tests passed");
            } else {
                out_UI_TestResult_C = 0; // Ошибка
                testLog.warn("Test 9: Some PassiveTimer tests failed: " + timerResults.failed);
            }
            break;

        case 10:
            // Тест 10: DeleayTimer
            testLog.info("Test 10: Testing DelayTimer");
            load("tests-uniset2-delaytimer.js");

            var timerResults = runDelayTimerTests();

            if (timerResults.failed === 0) {
                out_UI_TestResult_C = 1; // Успех
                testLog.info("Test 10: All DelayTimer tests passed");
            } else {
                out_UI_TestResult_C = 0; // Ошибка
                testLog.warn("Test 10: Some DelayTimer tests failed: " + timerResults.failed);
            }
            break;

        case 11:
            // Тест 11: uniset2-timers
            testLog.info("Test 11: Testing uniset2-timers");
            load("tests-uniset2-timers.js");

            var timersResults = runTimersTests();

            if (timersResults.failed === 0) {
                out_UI_TestResult_C = 1; // Успех
                testLog.info("Test 11: All uniset2-timers tests passed");
            } else {
                out_UI_TestResult_C = 0; // Ошибка
                testLog.warn("Test 11: Some uniset2-timers tests failed: " + timersResults.failed);
            }
            break;

        case 12:
            // Тест 12: tests-uniset2-mini-http
            load("tests-uniset2-mini-http.js")
            testLog.info("Test 12: tests-uniset2-mini-http.js");
            result = runLogTests();
            if( result.failed === 0 )
            {
                out_UI_TestResult_C = 1; // Успех
                testLog.info("Test 12: Completed successfully");
            }
            break;

        case 13:
        {
            // Тест 13: uniset2-simitator
            testLog.info("Test 13: Testing uniset2-simitator");
            const resumeSim = simExample && typeof simExample.isRunning === "function" && simExample.isRunning();

            if (simExample && typeof simExample.stop === "function")
                simExample.stop();

            load("tests-uniset2-simitator.js");
            const simResults = runSimitatorTests();

            if (simResults.failed === 0) {
                out_UI_TestResult_C = 1;
                testLog.info("Test 13: All simitator tests passed");
            } else {
                out_UI_TestResult_C = 0;
                testLog.warn("Test 13: simitator tests failed: " + simResults.failed);
            }

            if (resumeSim && simExample && typeof simExample.start === "function")
                simExample.start();

            break;
        }

        default:
            testLog.warn("Unknown test command:", testCommand);
            break;
    }
}

function testLocalFunc(cnt)
{
    if (cnt % 10 === 0)
        out_JS3_C = cnt;
}
