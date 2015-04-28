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
    CHECK( conf!=nullptr );

    if( !ui )
    {
        ui = make_shared<UInterface>();
        CHECK( ui->getObjectIndex() != nullptr );
        CHECK( ui->getConf() == conf );
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("[SM]: init from reserv","[sm][reserv]")
{
	InitTest();
/*
	CHECK( ui->getValue(500) == -50 );
	CHECK( ui->getValue(501) == 1 );
	CHECK( ui->getValue(502) == 390 );
	CHECK( ui->getValue(503) == 1 );
*/
}
// -----------------------------------------------------------------------------
