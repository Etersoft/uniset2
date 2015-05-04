#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <memory>
#include "UniSetTypes.h"
#include "UInterface.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
// -----------------------------------------------------------------------------
static shared_ptr<UInterface> ui;
// -----------------------------------------------------------------------------
void InitTest()
{
	auto conf = uniset_conf();
	CHECK( conf != nullptr );

	if( !ui )
	{
		ui = make_shared<UInterface>();
		CHECK( ui->getObjectIndex() != nullptr );
		CHECK( ui->getConf() == conf );
	}
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

	ui->setValue(500,30);
	CHECK( ui->getValue(500) == 30 );

	ui->setValue(500,-30);
	CHECK( ui->getValue(500) == -30 );

	ui->setValue(500,0);
	CHECK( ui->getValue(500) == 0 );
}
// -----------------------------------------------------------------------------
TEST_CASE("[SM]: threshold", "[sm][threshold]")
{
	InitTest();

	ui->setValue(503,30);
	CHECK( ui->getValue(504) == 0 );

	ui->setValue(503,400);
	CHECK( ui->getValue(504) == 1 );

	ui->setValue(503,25);
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

	ui->setValue(506,2);
	msleep(500);
	CHECK( ui->getValue(507) );
	msleep(2000);
	CHECK_FALSE( ui->getValue(507) );

	ui->setValue(506,4);
	msleep(2000);
	CHECK( ui->getValue(507) );
}
// -----------------------------------------------------------------------------
