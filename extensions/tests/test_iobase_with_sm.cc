#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <memory>
#include "Exceptions.h"
#include "Extensions.h"
#include "tests_with_sm.h"
#include "IOBase.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// -----------------------------------------------------------------------------
static SMInterface* shm = nullptr;
static void init_test()
{
	shm = smiInstance();
	CHECK( shm != nullptr );
}

static bool init_iobase( IOBase* ib, const std::string& sensor )
{
	init_test();
	CHECK( shm != nullptr );

	auto conf = uniset_conf();
	xmlNode* snode = conf->getXMLObjectNode( conf->getSensorID(sensor) );
	CHECK( snode != 0 );
	UniXML::iterator it(snode);
	shm->initIterator(ib->d_it);
	shm->initIterator(ib->d_it);
	return IOBase::initItem(ib,it,shm, "", false);
}
// -----------------------------------------------------------------------------
TEST_CASE("[IOBase::depend]: DI depend on the DI","[iobase][depend][di-di][extensions]")
{
    CHECK( uniset_conf()!=nullptr );
	auto conf = uniset_conf();

	init_test();

    ObjectId di = conf->getSensorID("DependTest_DI_S");
    CHECK( di != DefaultObjectId );

    SECTION("DI depend on the DI (depend_value=0)..")
    {
        IOBase ib;
		CHECK( init_iobase(&ib,"DependTest_DI3_S") );

		shm->setValue(di,0);
		CHECK_FALSE( ib.check_depend(shm) );

		shm->setValue(di,1);
		CHECK( ib.check_depend(shm) );
    }

    SECTION("DI depend on the DI (depend_value=1)..")
    {
        IOBase ib;
		CHECK( init_iobase(&ib,"DependTest_DI4_S") );

		shm->setValue(di,0);
		CHECK( ib.check_depend(shm) );

		shm->setValue(di,1);
		CHECK_FALSE( ib.check_depend(shm) );
    }

    SECTION("DI depend on the DI (default value)")
    {
        IOBase ib;
		CHECK( init_iobase(&ib,"DependTest_DI5_S") );

		shm->setValue(di,0);
		CHECK_FALSE( ib.check_depend(shm) );

		shm->setValue(di,1);
		CHECK( ib.check_depend(shm) );
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("[IOBase::depend]: DI depend on the AI","[iobase][depend][di-ai][extensions]")
{
    CHECK( uniset_conf()!=nullptr );
	auto conf = uniset_conf();

	init_test();

    ObjectId ai = conf->getSensorID("DependTest_AI_AS");
    CHECK( ai != DefaultObjectId );

    SECTION("DI depend on the AI (depend_value=0)..")
    {
        IOBase ib;
		CHECK( init_iobase(&ib,"DependTest_DI1_S") );

		shm->setValue(ai,0);
		CHECK_FALSE( ib.check_depend(shm) );

		shm->setValue(ai,1);
		CHECK( ib.check_depend(shm) );
    }

    SECTION("DI depend on the AI (depend_value=1)..")
    {
        IOBase ib;
		CHECK( init_iobase(&ib,"DependTest_DI2_S") );

		shm->setValue(ai,0);
		CHECK( ib.check_depend(shm) );

		shm->setValue(ai,1);
		CHECK_FALSE( ib.check_depend(shm) );
    }

    SECTION("DI depend on the AI (default value)")
    {
        IOBase ib;
		CHECK( init_iobase(&ib,"DependTest_DI6_S") );

		shm->setValue(ai,0);
		CHECK_FALSE( ib.check_depend(shm) );

		shm->setValue(ai,1);
		CHECK( ib.check_depend(shm) );
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("[IOBase::depend]: AI depend on the AI","[iobase][depend][ai-ai][extensions]")
{
    CHECK( uniset_conf()!=nullptr );
	auto conf = uniset_conf();

	init_test();

    ObjectId ai = conf->getSensorID("DependTest_AI_AS");
    CHECK( ai != DefaultObjectId );

    SECTION("AI depend on the AI (depend_value=10)..")
    {
        IOBase ib;
		CHECK( init_iobase(&ib,"DependTest_AI2_AS") );

		shm->setValue(ai,0);
		CHECK_FALSE( ib.check_depend(shm) );

		IOBase::processingAsAI(&ib,20,shm,true);
		REQUIRE( shm->getValue(ib.si.id) == -10 );

		shm->setValue(ai,10);
		CHECK( ib.check_depend(shm) );
		IOBase::processingAsAI(&ib,20,shm,true);
		REQUIRE( shm->getValue(ib.si.id) == 20 );

		shm->setValue(ai,2);
		CHECK_FALSE( ib.check_depend(shm) );

		IOBase::processingAsAI(&ib,20,shm,true);
		REQUIRE( shm->getValue(ib.si.id) == -10 );
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("[IOBase::depend]: AO depend on the DI","[iobase][depend][ao-di][extensions]")
{
    CHECK( uniset_conf()!=nullptr );
	auto conf = uniset_conf();

	init_test();

    ObjectId di = conf->getSensorID("DependTest_DI_S");
    CHECK( di != DefaultObjectId );

    SECTION("AO depend on the DI..")
    {
        IOBase ib;
		CHECK( init_iobase(&ib,"DependTest_AO2_AS") );

		shm->setValue(di,0);
		CHECK_FALSE( ib.check_depend(shm) );
		REQUIRE( IOBase::processingAsAO(&ib,shm,true) == 10000 );

		shm->setValue(di,1);
		CHECK( ib.check_depend(shm) );
		REQUIRE( IOBase::processingAsAO(&ib,shm,true) == 15 ); // 15 - default value (см. tests_with_sm_xml)

		shm->setValue(di,0);
		CHECK_FALSE( ib.check_depend(shm) );
		REQUIRE( IOBase::processingAsAO(&ib,shm,true) == 10000 );
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("IOBase with SM","[iobase][extensions]")
{
	WARN("IOBase with SM not yet!");
	// rawdata
	// ignore
	// ioinvert
	// precision
}
// -----------------------------------------------------------------------------
