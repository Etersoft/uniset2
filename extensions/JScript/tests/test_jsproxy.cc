#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <memory>
#include <unordered_set>
#include <limits>
#include <iostream>
#include "JSProxy.h"
#include "SMInterface.h"
#include "SharedMemory.h"
// -----------------------------------------------------------------------------
// #define debuglog 1
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// -----------------------------------------------------------------------------
static shared_ptr<UInterface> ui;
static std::shared_ptr<SMInterface> smi;
// -----------------------------------------------------------------------------
extern std::shared_ptr<SharedMemory> shm;
extern std::shared_ptr<JSProxy> js;
// -----------------------------------------------------------------------------
const ObjectId sidStart_C = 1000;
const ObjectId sidStart_S = 1001;
const ObjectId sidStop_C = 1002;
const ObjectId sidJS1_S = 1003;
const ObjectId sidJS2_C = 1004;
const ObjectId sidJS3_C = 1005;
const ObjectId sidJS4_C = 1006;
const ObjectId sidStartModuleTests = 1007;
const ObjectId sidModuleTestsResult = 1008;
const ObjectId sidUI_TestSensor1 = 1009;
const ObjectId sidUI_TestSensor2 = 1010;
const ObjectId sidUI_TestOutput1 = 1011;
const ObjectId sidUI_TestOutput2 = 1012;
const ObjectId sidUI_TestCommand_S = 1013;
const ObjectId sidUI_TestResult_C = 1014;
// -----------------------------------------------------------------------------
static void InitTest()
{
    auto conf = uniset_conf();
    CHECK( conf != nullptr );

    if( !ui )
    {
        ui = make_shared<UInterface>(uniset::AdminID);
        CHECK( ui->getObjectIndex() != nullptr );
        CHECK( ui->getConf() == conf );
    }

    if( !smi )
    {
        if( shm == nullptr )
            throw SystemError("SharedMemory don`t initialize..");

        if( ui == nullptr )
            throw SystemError("UInterface don`t initialize..");

        smi = make_shared<SMInterface>(ui->getId(), ui, ui->getId(), shm );
    }

    CHECK(js != nullptr );
}

// Функция для сброса состояния между тестами
static void ResetUITestState()
{
    ui->setValue(sidUI_TestSensor1, 0);
    ui->setValue(sidUI_TestSensor2, 0);
    ui->setValue(sidUI_TestOutput1, 0);
    ui->setValue(sidUI_TestOutput2, 0);
    ui->setValue(sidUI_TestCommand_S, 0);
    ui->setValue(sidUI_TestResult_C, 0);
    msleep(200); // Даем время на обработку сброса
}

// Функция для выполнения теста и проверки результата
static bool ExecuteUITest(int testCommand, int expectedOutput1 = -1, int expectedOutput2 = -1, int maxWaitMsec = 6000)
{
#if debuglog
    cout << "=== Executing UI Test " << testCommand << " ===" << endl;
    cout << "Before test - Command: " << ui->getValue(sidUI_TestCommand_S)
         << ", Result: " << ui->getValue(sidUI_TestResult_C)
         << ", Output1: " << ui->getValue(sidUI_TestOutput1)
         << ", Output2: " << ui->getValue(sidUI_TestOutput2) << endl;
#endif

    // Сохраняем исходное состояние команды
    int originalCommand = ui->getValue(sidUI_TestCommand_S);

    // Сбрасываем результат перед началом теста
    ui->setValue(sidUI_TestResult_C, 0);

    // Устанавливаем команду теста
    ui->setValue(sidUI_TestCommand_S, testCommand);

    // Интеллектуальное ожидание с проверкой готовности
    bool testCompleted = false;
    int elapsedTime = 0;
    const int checkInterval = 100; // Проверяем каждые 100 мс
    int lastResult = 0;

    while (elapsedTime < maxWaitMsec && !testCompleted)
    {
        msleep(checkInterval);
        elapsedTime += checkInterval;

        // Проверяем, готов ли результат теста
        int currentResult = ui->getValue(sidUI_TestResult_C);

        // Если результат изменился
        if (currentResult != lastResult)
        {
#if debuglog
            cout << "Result changed to " << currentResult << " after " << elapsedTime << " ms" << endl;
#endif
            lastResult = currentResult;
        }

        // 0 - тест еще выполняется, другие значения - тест завершен
        if (currentResult != 0)
        {
            testCompleted = true;

#if debuglog
            cout << "Test completed after " << elapsedTime << " ms" << endl;
            cout << "After test - Command: " << ui->getValue(sidUI_TestCommand_S)
                 << ", Result: " << ui->getValue(sidUI_TestResult_C)
                 << ", Output1: " << ui->getValue(sidUI_TestOutput1)
                 << ", Output2: " << ui->getValue(sidUI_TestOutput2) << endl;
#endif
        }

        // Дополнительная проверка: если команда была сброшена, тест завершился
        if (ui->getValue(sidUI_TestCommand_S) != testCommand)
        {
            testCompleted = true;
#if debuglog
            cout << "Test command was reset after " << elapsedTime << " ms" << endl;
#endif
        }
    }

    // Если тест не завершился за отведенное время
    if (!testCompleted)
    {
        cout << "Test FAILED: Timeout after " << maxWaitMsec << " ms" << endl;
        cout << "Last state - Command: " << ui->getValue(sidUI_TestCommand_S)
             << ", Result: " << ui->getValue(sidUI_TestResult_C)
             << ", Output1: " << ui->getValue(sidUI_TestOutput1)
             << ", Output2: " << ui->getValue(sidUI_TestOutput2) << endl;
        return false;
    }

    // Проверяем успешность теста
    int finalResult = ui->getValue(sidUI_TestResult_C);

    if (finalResult != 1)
    {
        cout << "Test FAILED: Result is " << finalResult << " (expected 1)" << endl;
        cout << "Final state - Command: " << ui->getValue(sidUI_TestCommand_S)
             << ", Output1: " << ui->getValue(sidUI_TestOutput1)
             << ", Output2: " << ui->getValue(sidUI_TestOutput2) << endl;
        return false;
    }

    // Проверяем ожидаемые значения выходов, если указаны
    bool outputsValid = true;

    if (expectedOutput1 != -1)
    {
        int actualOutput1 = ui->getValue(sidUI_TestOutput1);

        if (actualOutput1 != expectedOutput1)
        {
            cout << "Test FAILED: Output1 expected " << expectedOutput1
                 << " but got " << actualOutput1 << endl;
            outputsValid = false;
        }
    }

    if (expectedOutput2 != -1)
    {
        int actualOutput2 = ui->getValue(sidUI_TestOutput2);

        if (actualOutput2 != expectedOutput2)
        {
            cout << "Test FAILED: Output2 expected " << expectedOutput2
                 << " but got " << actualOutput2 << endl;
            outputsValid = false;
        }
    }

    if (!outputsValid)
    {
        return false;
    }

    cout << "Test PASSED (completed in " << elapsedTime << " ms)" << endl;
    return true;
}
// -----------------------------------------------------------------------------
TEST_CASE("JSEngine: start", "[jscript]")
{
    InitTest();

    REQUIRE( ui->getValue(sidStart_C) == 10 );
    REQUIRE( ui->getValue(sidStop_C) != 15 );
    REQUIRE( ui->getValue(sidStart_S) == 100 );
}
// -----------------------------------------------------------------------------
TEST_CASE("JSEngine: step", "[jscript]")
{
    InitTest();
    ui->setValue(sidJS1_S, 100);
    msleep(200);
    REQUIRE(ui->getValue(sidJS2_C) == 100);

    ui->setValue(sidJS1_S, 110);
    msleep(200);
    REQUIRE(ui->getValue(sidJS2_C) == 110);

}
// -----------------------------------------------------------------------------
TEST_CASE("JSEngine: local func", "[jscript]")
{
    InitTest();

    ui->setValue(sidJS1_S, 100);
    msleep(200);
    REQUIRE(ui->getValue(sidJS4_C) == 200);
    REQUIRE(ui->getValue(sidJS3_C) == 100);

    ui->setValue(sidJS1_S, 110);
    msleep(200);
    REQUIRE(ui->getValue(sidJS4_C) == 220);
    REQUIRE(ui->getValue(sidJS3_C) == 110);
}
// -----------------------------------------------------------------------------
TEST_CASE("JSEngine: modules tests", "[jscript][modules]")
{
    InitTest();

    //    ui->setValue(sidStartModuleTests, 100);
    //    msleep(10000);
    //    REQUIRE(ui->getValue(sidModuleTestsResult) == 1);
}
// -----------------------------------------------------------------------------
TEST_CASE("JSEngine: ui object - getValue by ID", "[jscript][ui][1]")
{
    InitTest();
    ResetUITestState();

    // Тест 1: ui.getValue() с числовым ID
    ui->setValue(sidUI_TestSensor1, 42);
    REQUIRE(ExecuteUITest(1, 42, -1)); // Ожидаем UI_TestOutput1 = 42
}
// -----------------------------------------------------------------------------
TEST_CASE("JSEngine: ui object - getValue by name", "[jscript][ui][2]")
{
    InitTest();
    ResetUITestState();

    // Тест 2: ui.getValue() с именем датчика в виде строки
    ui->setValue(sidUI_TestSensor2, 123);
    REQUIRE(ExecuteUITest(2, -1, 123)); // Ожидаем UI_TestOutput2 = 123
}
// -----------------------------------------------------------------------------
TEST_CASE("JSEngine: ui object - setValue by ID", "[jscript][ui][3]")
{
    InitTest();
    ResetUITestState();

    // Тест 3: ui.setValue() с числовым ID
    REQUIRE(ExecuteUITest(3, 999, -1)); // Ожидаем UI_TestOutput1 = 999
}
// -----------------------------------------------------------------------------
TEST_CASE("JSEngine: ui object - setValue by name", "[jscript][ui][4]")
{
    InitTest();
    ResetUITestState();

    // Тест 4: ui.setValue() с именем датчика в виде строки
    REQUIRE(ExecuteUITest(4, -1, 888)); // Ожидаем UI_TestOutput2 = 888
}
// -----------------------------------------------------------------------------
TEST_CASE("JSEngine: ui object - askSensor reaction", "[jscript][ui][5]")
{
    InitTest();
    ResetUITestState();

    // Тест 5: проверка реакции на изменение датчика после askSensor
    REQUIRE(ExecuteUITest(5)); // Запускаем тест 5

    // Меняем значение датчика - JS должен отреагировать в uniset_on_sensor
    ui->setValue(sidUI_TestSensor1, 77);
    msleep(400);

    REQUIRE(ui->getValue(sidUI_TestOutput1) == 77); // Проверяем реакцию на изменение
}
// -----------------------------------------------------------------------------
TEST_CASE("JSEngine: ui object - UIONotify constants", "[jscript][ui][6]")
{
    InitTest();
    ResetUITestState();

    // Тест 6: проверка констант UIONotify
    REQUIRE(ExecuteUITest(6, UniversalIO::UIONotify, UniversalIO::UIODontNotify));
}
// -----------------------------------------------------------------------------
TEST_CASE("JSEngine: ui object - combined test", "[jscript][ui][7]")
{
    InitTest();
    ResetUITestState();

    // Тест 7: комбинированный тест - чтение и запись
    ui->setValue(sidUI_TestSensor1, 30);
    ui->setValue(sidUI_TestSensor2, 20);
    REQUIRE(ExecuteUITest(7, 60, 30)); // Ожидаем UI_TestOutput1 = 60, UI_TestOutput2 = 30
}
// -----------------------------------------------------------------------------
TEST_CASE("JSEngine: uniset2-logs", "[jscript][ui][8]")
{
    InitTest();
    ResetUITestState();

    // Тест 8: uniset2-logs
    REQUIRE(ExecuteUITest(8, -1, -1));
}
// -----------------------------------------------------------------------------
TEST_CASE("JSEngine: passive timer", "[jscript][ui][9]")
{
    InitTest();
    ResetUITestState();

    // Тест 9: passive timer
    REQUIRE(ExecuteUITest(9, -1, -1, 6000));
}
// -----------------------------------------------------------------------------
TEST_CASE("JSEngine: delay timer", "[jscript][ui][10]")
{
    InitTest();
    ResetUITestState();

    // Тест 10: delay timer
    REQUIRE(ExecuteUITest(10, -1, -1, 6000));
}
// -----------------------------------------------------------------------------
TEST_CASE("JSEngine: timers system", "[jscript][ui][11]")
{
    InitTest();
    ResetUITestState();

    // Тест 11: uniset2-timers
    REQUIRE(ExecuteUITest(11, -1, -1, 8000));
}
// -----------------------------------------------------------------------------
TEST_CASE("JSEngine: mini http", "[jscript][http][12]")
{
    InitTest();
    ResetUITestState();

    // Тест 12: uniset2-mini-http + router
    REQUIRE(ExecuteUITest(12, -1, -1, 8000));
}
// -----------------------------------------------------------------------------
TEST_CASE("JSEngine: simitator module", "[jscript][simitator][13]")
{
    InitTest();
    ResetUITestState();

    // Тест 13: uniset2-simitator
    REQUIRE(ExecuteUITest(13, -1, -1, 8000));
}
// -----------------------------------------------------------------------------
TEST_CASE("JSEngine: modbus client", "[jscript][modbus][14]")
{
    InitTest();
    ResetUITestState();

    REQUIRE(ExecuteUITest(14, 0, 0, 3000));
}
// -----------------------------------------------------------------------------
