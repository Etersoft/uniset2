#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <time.h>
#include <limits>
#include <unordered_set>
#include <Poco/Net/NetException.h>
#include "UniSetTypes.h"
#include "FakeIOControl.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// -----------------------------------------------------------------------------
static shared_ptr<UInterface> ui;
extern shared_ptr<FakeIOControl> ioc;
const uniset::timeout_t polltime = 150;
// -----------------------------------------------------------------------------
static void InitTest()
{
    auto conf = uniset_conf();
    CHECK( conf != nullptr );

    if( !ui )
    {
        ui = make_shared<UInterface>();
        // UI понадобиться для проверки записанных в SM значений.
        CHECK( ui->getObjectIndex() != nullptr );
        CHECK( ui->getConf() == conf );
    }

    REQUIRE( ioc != nullptr );
}
// -----------------------------------------------------------------------------
TEST_CASE("IOControl: DI", "[iocontrol][di]")
{
    InitTest();

    ioc->fcard->chInputs[0] = 1;
    msleep(polltime + 10);
    REQUIRE( ui->getValue(1000) == 1 );
    REQUIRE( ui->getValue(1001) == 0 ); // invert

    ioc->fcard->chInputs[0] = 0;
    msleep(polltime + 10);
    REQUIRE( ui->getValue(1000) == 0 );
    REQUIRE( ui->getValue(1001) == 1 ); // invert
}
// -----------------------------------------------------------------------------
TEST_CASE("IOControl: DO", "[iocontrol][do]")
{
    InitTest();

    ui->setValue(1002, 1);
    ui->setValue(1003, 1);
    msleep(polltime + 10);
    REQUIRE( ioc->fcard->chOutputs[0] == 1 );
    REQUIRE( ioc->fcard->chOutputs[1] == 0 ); // invert

    ui->setValue(1002, 0);
    ui->setValue(1003, 0);
    msleep(polltime + 10);
    REQUIRE( ioc->fcard->chOutputs[0] == 0 );
    REQUIRE( ioc->fcard->chOutputs[1] == 1 ); // invert
}
// -----------------------------------------------------------------------------
static size_t pulseCount( FakeComediInterface* card, size_t ch )
{
    // считаем количество импульсов "0 -> 1 -> 0"
    size_t npulse = 0;
    bool prev = false;

    for( size_t i = 0; i < 20 && npulse < 3; i ++ )
    {
        if( card->chOutputs[ch] == 1 && !prev )
            prev = true;
        else if( card->chOutputs[ch] == 0 && prev )
        {
            prev = false;
            npulse++;
        }

        // чтобы не пропустить импульсы.. спим меньше
        msleep( polltime / 2 );
    }

    return npulse;
}
// -----------------------------------------------------------------------------
TEST_CASE("IOControl: AO (lamp)", "[iocontrol][lamp]")
{
    InitTest();

    auto card = ioc->fcard;

    ui->setValue(1004, uniset::lmpBLINK);

    // считаем количество импульсов "0 -> 1 -> 0"
    size_t npulse = pulseCount(card, 4);
    REQUIRE( npulse >= 2 );
}
// -----------------------------------------------------------------------------
TEST_CASE("IOControl: AO", "[iocontrol][ao]")
{
    InitTest();

    auto card = ioc->fcard;

    ui->setValue(1005, 10);
    ui->setValue(1006, 100);
    ui->setValue(1007, 300);
    msleep(polltime + 10);

    REQUIRE( ioc->fcard->chOutputs[5] == 10 );
    REQUIRE( ioc->fcard->chOutputs[6] == 1000 ); // calibration channel
    REQUIRE( ioc->fcard->chOutputs[7] == 1000 ); // caldiagram

    ui->setValue(1005, 100);
    ui->setValue(1006, -100);
    ui->setValue(1007, -300);
    msleep(polltime + 10);

    REQUIRE( ioc->fcard->chOutputs[5] == 100 );
    REQUIRE( ioc->fcard->chOutputs[6] == 0 ); // calibration channel
    REQUIRE( ioc->fcard->chOutputs[7] == -1000 ); // caldiagram
}
// -----------------------------------------------------------------------------
TEST_CASE("IOControl: threshold", "[iocontrol][threshold]")
{
    InitTest();

    auto card = ioc->fcard;

    ioc->fcard->chInputs[10] = 20;
    msleep(polltime + 10);
    REQUIRE( ui->getValue(1010) == 20 );
    REQUIRE( ui->getValue(1011) == 0 ); // threshold [30,40]

    ioc->fcard->chInputs[10] = 35;
    msleep(polltime + 10);
    REQUIRE( ui->getValue(1010) == 35 );
    REQUIRE( ui->getValue(1011) == 0 ); // threshold [30,40]

    ioc->fcard->chInputs[10] = 45;
    msleep(polltime + 10);
    REQUIRE( ui->getValue(1010) == 45 );
    REQUIRE( ui->getValue(1011) == 1 ); // threshold [30,40]

    ioc->fcard->chInputs[10] = 35;
    msleep(polltime + 10);
    REQUIRE( ui->getValue(1010) == 35 );
    REQUIRE( ui->getValue(1011) == 1 ); // threshold [30,40]

    ioc->fcard->chInputs[10] = 25;
    msleep(polltime + 10);
    REQUIRE( ui->getValue(1010) == 25 );
    REQUIRE( ui->getValue(1011) == 0 ); // < lowlimit (30)
}
// -----------------------------------------------------------------------------
TEST_CASE("IOControl: test lamp", "[iocontrol][testlamp]")
{
    InitTest();

    auto card = ioc->fcard;

    // отключаем тест ламп
    card->chInputs[12] = 0;
    msleep(polltime + 10);
    REQUIRE( ui->getValue(1012) == 0 );
    REQUIRE( ui->getValue(1013) == 0 );
    REQUIRE( card->chOutputs[13] == 0 );

    // включаем тест ламп
    card->chInputs[12] = 1;
    msleep(polltime + 10);
    REQUIRE( ui->getValue(1012) == 1 );

    // должны ловить мигание лампочки..
    size_t npulse = pulseCount(card, 13);
    REQUIRE( npulse >= 2 );

    // отключаем тест ламп
    card->chInputs[12] = 0;
    msleep(polltime + 10);
    REQUIRE( ui->getValue(1012) == 0 );
    REQUIRE( ui->getValue(1013) == 0 );
    REQUIRE( card->chOutputs[13] == 0 );

    npulse = pulseCount(card, 13);
    REQUIRE( npulse == 0 );

}
// -----------------------------------------------------------------------------
