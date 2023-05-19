#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <memory>
#include <future>
#include "UniSetTypes.h"
#include "UInterface.h"
#include "DelayTimer.h"
#include "TestObject.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// -----------------------------------------------------------------------------
static shared_ptr<UInterface> ui;
extern shared_ptr<TestObject> obj;
// -----------------------------------------------------------------------------
void InitTest()
{
    auto conf = uniset_conf();
    REQUIRE( conf != nullptr );

    if( !ui )
    {
        ui = make_shared<UInterface>();
        REQUIRE( ui->getObjectIndex() != nullptr );
        REQUIRE( ui->getConf() == conf );
    }

    REQUIRE( obj != nullptr );
}
// -----------------------------------------------------------------------------
/*
TEST_CASE("[SM]: init from reserv", "[sm][reserv]")
{
    InitTest();
    CHECK( ui->getValue(500) == -50 );
    CHECK( ui->getValue(501) == 1 );
    CHECK( ui->getValue(503) == 390 );
    CHECK( ui->getValue(504) == 1 );
}
*/
// -----------------------------------------------------------------------------
TEST_CASE("[SM]: get/set", "[sm][getset]")
{
    InitTest();

    ui->setValue(500, 30);
    CHECK( ui->getValue(500) == 30 );

    ui->setValue(500, -30);
    CHECK( ui->getValue(500) == -30 );

    ui->setValue(500, 0);
    CHECK( ui->getValue(500) == 0 );
}
// -----------------------------------------------------------------------------
TEST_CASE("[SM]: threshold", "[sm][threshold]")
{
    InitTest();

    ui->setValue(503, 30);
    CHECK( ui->getValue(504) == 0 );

    ui->setValue(503, 400);
    CHECK( ui->getValue(504) == 1 );

    ui->setValue(503, 25);
    CHECK( ui->getValue(504) == 0 );
}
// -----------------------------------------------------------------------------
TEST_CASE("[SM]: threshold (invert)", "[sm][threshold]")
{
    InitTest();

    ui->setValue(503, 20);
    CHECK( ui->getValue(515) == 1 );

    ui->setValue(503, 25);
    CHECK( ui->getValue(515) == 1 );

    ui->setValue(503, 35);
    CHECK( ui->getValue(515) == 0 );
}
// -----------------------------------------------------------------------------
TEST_CASE("[SM]: pulsar", "[sm][pulsar]")
{
    InitTest();

    while( ui->getValue(505) )
        msleep(50);

    CHECK_FALSE( ui->getValue(505) );
    msleep(1200);
    CHECK( ui->getValue(505) );
    msleep(1200);
    CHECK_FALSE( ui->getValue(505) );
    msleep(1200);
    CHECK( ui->getValue(505) );
}
// -----------------------------------------------------------------------------
TEST_CASE("[SM]: heartbeat", "[sm][heartbeat]")
{
    InitTest();
    CHECK_FALSE( ui->getValue(507) );

    ui->setValue(506, 2);
    msleep(500);
    CHECK( ui->getValue(507) );
    msleep(2000);
    CHECK_FALSE( ui->getValue(507) );

    ui->setValue(506, 4);
    msleep(2000);
    CHECK( ui->getValue(507) );
}
// -----------------------------------------------------------------------------
TEST_CASE("[SM]: event", "[sm][event]")
{
    // SM при старте должна была прислать..
    // здесь просто проверяем
    CHECK( obj->getEvnt() );
}
// -----------------------------------------------------------------------------
TEST_CASE("[SM]: askSensors", "[sm][ask]")
{
    InitTest();
    ui->setValue(509, 1);
    msleep(200);
    CHECK( obj->in_sensor_s );

    ui->setValue(509, 0);
    msleep(300);
    CHECK_FALSE( obj->in_sensor_s );

    obj->out_output_c = 1200;
    msleep(300);
    REQUIRE( ui->getValue(508) == 1200 );

    obj->out_output_c = 100;
    msleep(300);
    REQUIRE( ui->getValue(508) == 100 );
}
// -----------------------------------------------------------------------------
TEST_CASE("[SM]: askDoNotNotify", "[sm][ask]")
{
    InitTest();

    ui->setValue(509, 1);
    msleep(200);
    CHECK( obj->in_sensor_s );

    obj->askDoNotNotify();

    ui->setValue(509, 0); // !!
    msleep(200);
    CHECK( obj->in_sensor_s ); //не поменялось значение

    obj->askNotifyChange();
    msleep(200);
    CHECK( obj->in_sensor_s ); // не менялось..
    ui->setValue(509, 1);
    msleep(200);
    CHECK( obj->in_sensor_s );
    ui->setValue(509, 0);
    msleep(200);
    CHECK_FALSE( obj->in_sensor_s );

    obj->askDoNotNotify();
    ui->setValue(509, 1);
    msleep(200);
    CHECK_FALSE( obj->in_sensor_s );

    obj->askNotifyFirstNotNull();
    msleep(200);
    CHECK( obj->in_sensor_s ); // должно придти т.к. равно "1"
}
// -----------------------------------------------------------------------------
TEST_CASE("[SM]: heartbeat test N2", "[sm][heartbeat]")
{
    InitTest();

    obj->runHeartbeat(5);
    msleep(obj->getHeartbeatTime() + 100);
    CHECK( ui->getValue(511) );

    obj->runHeartbeat(2);
    obj->stopHeartbeat();
    msleep(3000);
    CHECK_FALSE( ui->getValue(511) );

    obj->runHeartbeat(5);
    msleep(obj->getHeartbeatTime() + 100);
    msleep(1500);
    CHECK( ui->getValue(511) );
}
// -----------------------------------------------------------------------------
TEST_CASE("[SM]: depend test", "[sm][depend]")
{
    InitTest();

    REQUIRE( ui->getValue(514) == 0 );
    REQUIRE( ui->getValue(512) == 1000 );
    REQUIRE( ui->getValue(513) == 0 );

    // проверяем что датчик DI работает
    ui->setValue(513, 1);
    msleep(300);
    REQUIRE( obj->in_dependDI_s == 1 );
    ui->setValue(513, 0);
    msleep(300);
    REQUIRE( obj->in_dependDI_s == 0 );

    // проверяем что датчик AI не работает, т.к. забклоирован
    ui->setValue(512, 100);
    REQUIRE( ui->getValue(512) == 1000 );
    msleep(300);
    REQUIRE( obj->in_dependAI_s == 1000 );
    ui->setValue(512, 50);
    REQUIRE( ui->getValue(512) == 1000 );
    msleep(300);
    REQUIRE( obj->in_dependAI_s == 1000 );

    // проверяем что после разблолокирования датчик принимает текущее значение
    // и процесс приходит sensorInfo
    ui->setValue(512, 40); // выставляем значение (находясь под блокировкой)
    REQUIRE( obj->in_dependAI_s == 1000 ); // у процесса пока блокированное значение

    // ----- разблокируем
    ui->setValue(514, 1);
    msleep(300);
    REQUIRE( ui->getValue(514) == 1 );

    // проверяем dependAI
    REQUIRE( ui->getValue(512) == 40 );
    msleep(300);
    REQUIRE( obj->in_dependAI_s == 40 );

    // dependDI наоборот заблокировался
    REQUIRE( ui->getValue(513) == 0 );
    ui->setValue(513, 1);
    msleep(300);
    REQUIRE( ui->getValue(513) == 0 );
    REQUIRE( obj->in_dependDI_s == 0 );


    // ----- блокируем
    ui->setValue(514, 0);
    msleep(300);
    REQUIRE( ui->getValue(514) == 0 );

    // проверяем dependAI
    REQUIRE( ui->getValue(512) == 1000 );
    msleep(300);
    REQUIRE( obj->in_dependAI_s == 1000 );

    // dependDI наоборот разблокировался
    REQUIRE( ui->getValue(513) == 1 );
    msleep(300);
    REQUIRE( obj->in_dependDI_s == 1 );
}
// -----------------------------------------------------------------------------
TEST_CASE("[SM]: monitonic sensor message", "[sm][monitonic]")
{
    InitTest();

    REQUIRE( obj->exist() );

    // Проверка корректной последовательности прихода SensorMessage.
    // Тест заключается в том, что параллельно вызывается setValue()
    // и askSensors() и сообщения должны приходить в правильном порядке.
    // Для проверки этого датчик монотонно увеличивается на +1
    // сама проверка см. TestObject::sensorInfo()
    auto conf = uniset_conf();
    const long max = uniset::getArgInt("--monotonic-max-value", conf->getArgc(), conf->getArgv(), "1000");

    auto&& write_worker = [&max]
    {
        try
        {
            for( long val = 0; val <= max; val++ )
                ui->setValue(516, val);
        }
        catch( std::exception& ex )
        {
            return false;
        }

        return true;
    };

    obj->startMonitonicTest();

    auto ret = std::async(std::launch::async, write_worker);

    for( long n = 0; n <= max; n++ )
        obj->askMonotonic();

    REQUIRE( ret.get() );

    DelayTimer dt(2000, 0);

    while( !dt.check(obj->isEmptyQueue()) )
        msleep(500);

    REQUIRE( obj->isMonotonicTestOK() );
    REQUIRE( obj->getLostMessages() == 0 );
    REQUIRE_FALSE( obj->isFullQueue() );
    REQUIRE( obj->getLastValue() == max );

    // print statistic
    //  uniset::SimpleInfo_var si = obj->getInfo(0);
    //  cerr << std::string(si->info) << endl;
}
// -----------------------------------------------------------------------------
TEST_CASE("[SM]: sendText", "[sm][sendText]")
{
    InitTest();

    std::string txt = "Hello world";

    ui->sendText(obj->getId(), txt, 3);
    msleep(300);

    REQUIRE( obj->getLastTextMessage() == txt );
    REQUIRE( obj->getLastTextMessageType() == 3 );
}
// -----------------------------------------------------------------------------
TEST_CASE("[SM]: freezeValue", "[sm][freezeValue]")
{
    InitTest();

    IOController_i::SensorInfo si;
    si.id = 517;
    si.node = uniset_conf()->getLocalNode();

    REQUIRE_NOTHROW( ui->setValue(517, 100) );
    REQUIRE( ui->getValue(517) == 100 );
    msleep(300);
    REQUIRE( obj->in_freeze_s == 100 );

    REQUIRE_NOTHROW( ui->freezeValue(si, true, 10) );
    REQUIRE( ui->getValue(517) == 10 );
    msleep(300);
    REQUIRE( obj->in_freeze_s == 10 );

    REQUIRE_NOTHROW( ui->setValue(517, 150) );
    REQUIRE( ui->getValue(517) == 10 );
    msleep(300);
    REQUIRE( obj->in_freeze_s == 10 );

    REQUIRE_NOTHROW( ui->freezeValue(si, false, 10) );
    REQUIRE( ui->getValue(517) == 150 );
    msleep(300);
    REQUIRE( obj->in_freeze_s == 150 );

    // default freezvalue
    REQUIRE( ui->getValue(518) == 10 );
    REQUIRE_NOTHROW( ui->setValue(518, 160) );
    REQUIRE( ui->getValue(518) == 10 );
    si.id = 518;
    si.node = uniset_conf()->getLocalNode();
    REQUIRE_NOTHROW( ui->freezeValue(si, false, 5) );
    REQUIRE( ui->getValue(518) == 160 );
}
// -----------------------------------------------------------------------------
TEST_CASE("[SM]: readonly", "[sm][readonly]")
{
    InitTest();

    REQUIRE_THROWS_AS(ui->setValue(519, 11), uniset::IOBadParam);
    REQUIRE(ui->getValue(519) == 100);
    msleep(300);
    REQUIRE(ui->getValue(519) == 100);
    REQUIRE_THROWS_AS(ui->setValue(519, 11), uniset::IOBadParam);
    REQUIRE(ui->getValue(519) == 100);
}
// -----------------------------------------------------------------------------