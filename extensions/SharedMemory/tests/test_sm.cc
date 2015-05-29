#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <memory>
#include "UniSetTypes.h"
#include "UInterface.h"
#include "TestObject.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
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
	msleep(200);
	CHECK_FALSE( obj->in_sensor_s );

	obj->out_output_c = 1200;
	msleep(200);
	REQUIRE( ui->getValue(508) == 1200 );

	obj->out_output_c = 100;
	msleep(200);
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
