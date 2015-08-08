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
static std::shared_ptr<SMInterface> shm;
static void init_test()
{
	shm = smiInstance();
	CHECK( shm != nullptr );
}
// -----------------------------------------------------------------------------
static bool init_iobase( IOBase* ib, const std::string& sensor )
{
	init_test();
	CHECK( shm != nullptr );

	auto conf = uniset_conf();
	xmlNode* snode = conf->getXMLObjectNode( conf->getSensorID(sensor) );
	CHECK( snode != 0 );
	UniXML::iterator it(snode);
	shm->initIterator(ib->d_it);
	shm->initIterator(ib->ioit);
	shm->initIterator(ib->t_ait);
	return IOBase::initItem(ib, it, shm, "", false);
}
// -----------------------------------------------------------------------------
TEST_CASE("[IOBase::depend]: DI depend on the DI", "[iobase][depend][di-di][extensions]")
{
	CHECK( uniset_conf() != nullptr );
	auto conf = uniset_conf();

	init_test();

	ObjectId di = conf->getSensorID("DependTest_DI_S");
	CHECK( di != DefaultObjectId );

	SECTION("DI depend on the DI (depend_value=0)..")
	{
		IOBase ib;
		CHECK( init_iobase(&ib, "DependTest_DI3_S") );

		shm->setValue(di, 0);
		CHECK_FALSE( ib.check_depend(shm) );

		shm->setValue(di, 1);
		CHECK( ib.check_depend(shm) );
	}

	SECTION("DI depend on the DI (depend_value=1)..")
	{
		IOBase ib;
		CHECK( init_iobase(&ib, "DependTest_DI4_S") );

		shm->setValue(di, 0);
		CHECK( ib.check_depend(shm) );

		shm->setValue(di, 1);
		CHECK_FALSE( ib.check_depend(shm) );
	}

	SECTION("DI depend on the DI (default value)")
	{
		IOBase ib;
		CHECK( init_iobase(&ib, "DependTest_DI5_S") );

		shm->setValue(di, 0);
		CHECK_FALSE( ib.check_depend(shm) );

		shm->setValue(di, 1);
		CHECK( ib.check_depend(shm) );
	}
}
// -----------------------------------------------------------------------------
TEST_CASE("[IOBase::depend]: DI depend on the AI", "[iobase][depend][di-ai][extensions]")
{
	CHECK( uniset_conf() != nullptr );
	auto conf = uniset_conf();

	init_test();

	ObjectId ai = conf->getSensorID("DependTest_AI_AS");
	CHECK( ai != DefaultObjectId );

	SECTION("DI depend on the AI (depend_value=0)..")
	{
		IOBase ib;
		CHECK( init_iobase(&ib, "DependTest_DI1_S") );

		shm->setValue(ai, 0);
		CHECK_FALSE( ib.check_depend(shm) );

		shm->setValue(ai, 1);
		CHECK( ib.check_depend(shm) );
	}

	SECTION("DI depend on the AI (depend_value=1)..")
	{
		IOBase ib;
		CHECK( init_iobase(&ib, "DependTest_DI2_S") );

		shm->setValue(ai, 0);
		CHECK( ib.check_depend(shm) );

		shm->setValue(ai, 1);
		CHECK_FALSE( ib.check_depend(shm) );
	}

	SECTION("DI depend on the AI (default value)")
	{
		IOBase ib;
		CHECK( init_iobase(&ib, "DependTest_DI6_S") );

		shm->setValue(ai, 0);
		CHECK_FALSE( ib.check_depend(shm) );

		shm->setValue(ai, 1);
		CHECK( ib.check_depend(shm) );
	}
}
// -----------------------------------------------------------------------------
TEST_CASE("[IOBase::depend]: AI depend on the AI", "[iobase][depend][ai-ai][extensions]")
{
	CHECK( uniset_conf() != nullptr );
	auto conf = uniset_conf();

	init_test();

	ObjectId ai = conf->getSensorID("DependTest_AI_AS");
	CHECK( ai != DefaultObjectId );

	SECTION("AI depend on the AI (depend_value=10)..")
	{
		IOBase ib;
		CHECK( init_iobase(&ib, "DependTest_AI2_AS") );

		shm->setValue(ai, 0);
		CHECK_FALSE( ib.check_depend(shm) );

		IOBase::processingAsAI(&ib, 20, shm, true);
		REQUIRE( shm->getValue(ib.si.id) == -10 );

		shm->setValue(ai, 10);
		CHECK( ib.check_depend(shm) );
		IOBase::processingAsAI(&ib, 20, shm, true);
		REQUIRE( shm->getValue(ib.si.id) == 20 );

		shm->setValue(ai, 2);
		CHECK_FALSE( ib.check_depend(shm) );

		IOBase::processingAsAI(&ib, 20, shm, true);
		REQUIRE( shm->getValue(ib.si.id) == -10 );
	}
}
// -----------------------------------------------------------------------------
TEST_CASE("[IOBase::depend]: AO depend on the DI", "[iobase][depend][ao-di][extensions]")
{
	CHECK( uniset_conf() != nullptr );
	auto conf = uniset_conf();

	init_test();

	ObjectId di = conf->getSensorID("DependTest_DI_S");
	CHECK( di != DefaultObjectId );

	SECTION("AO depend on the DI..")
	{
		IOBase ib;
		CHECK( init_iobase(&ib, "DependTest_AO2_AS") );

		shm->setValue(di, 0);
		CHECK_FALSE( ib.check_depend(shm) );
		REQUIRE( IOBase::processingAsAO(&ib, shm, true) == 10000 );

		shm->setValue(di, 1);
		CHECK( ib.check_depend(shm) );
		REQUIRE( IOBase::processingAsAO(&ib, shm, true) == 15 ); // 15 - default value (см. tests_with_sm_xml)

		shm->setValue(di, 0);
		CHECK_FALSE( ib.check_depend(shm) );
		REQUIRE( IOBase::processingAsAO(&ib, shm, true) == 10000 );
	}
}
// -----------------------------------------------------------------------------
TEST_CASE("[IOBase::calibration]: AI calibration", "[iobase][calibration][ai][extensions]")
{
	CHECK( uniset_conf() != nullptr );
	auto conf = uniset_conf();

	init_test();

	ObjectId ai = conf->getSensorID("CalibrationTest_AI1_AS");
	CHECK( ai != DefaultObjectId );

	IOBase ib;
	CHECK( init_iobase(&ib, "CalibrationTest_AI1_AS") );

	SECTION("AI calibration (asAI)")
	{
		IOBase::processingAsAI(&ib, 0, shm, true);
		REQUIRE( shm->getValue(ai) == 0 );

		IOBase::processingAsAI(&ib, -100, shm, true);
		REQUIRE( shm->getValue(ai) == -1000 );

		IOBase::processingAsAI(&ib, 100, shm, true);
		REQUIRE(  shm->getValue(ai) == 1000 );
	}

	SECTION("AI calibration (asAO)")
	{
		shm->setValue(ai, 0);
		REQUIRE( IOBase::processingAsAO(&ib, shm, true) == 0 );

		shm->setValue(ai, -1000);
		REQUIRE( IOBase::processingAsAO(&ib, shm, true) == -100 );

		shm->setValue(ai, 1000);
		REQUIRE( IOBase::processingAsAO(&ib, shm, true) == 100 );
	}

	SECTION("AI calibration (FasAI)")
	{
		IOBase::processingFasAI(&ib, 0.0, shm, true);
		REQUIRE( shm->getValue(ai) == 0 );

		IOBase::processingFasAI(&ib, -99.9, shm, true); // округлится до 100
		REQUIRE( shm->getValue(ai) == -1000 );

		IOBase::processingFasAI(&ib, -99.4, shm, true); // округлится до 99
		REQUIRE( shm->getValue(ai) == -990 );

		IOBase::processingFasAI(&ib, 99.9, shm, true);
		REQUIRE(  shm->getValue(ai) == 1000 );

		IOBase::processingFasAI(&ib, 99.4, shm, true);
		REQUIRE(  shm->getValue(ai) == 990 );
	}

	SECTION("AI calibration (FasAO)")
	{
		shm->setValue(ai, 0);
		REQUIRE( IOBase::processingFasAO(&ib, shm, true) == 0 );

		shm->setValue(ai, -1000);
		REQUIRE( IOBase::processingFasAO(&ib, shm, true) == -100 );

		shm->setValue(ai, -995);
		REQUIRE( IOBase::processingFasAO(&ib, shm, true) == -99.5f );

		shm->setValue(ai, 1000);
		REQUIRE( IOBase::processingFasAO(&ib, shm, true) == 100 );

		shm->setValue(ai, 995);
		REQUIRE( IOBase::processingFasAO(&ib, shm, true) == 99.5f );

		shm->setValue(ai, 994);
		REQUIRE( IOBase::processingFasAO(&ib, shm, true) == 99.4f );
	}

	SECTION("AI calibration rawdata=1 (asAI)")
	{
		ib.rawdata = true;

		IOBase::processingAsAI(&ib, 0, shm, true);
		REQUIRE( shm->getValue(ai) == 0 );

		IOBase::processingAsAI(&ib, -100, shm, true);
		REQUIRE( shm->getValue(ai) == -100 );

		IOBase::processingAsAI(&ib, 100, shm, true);
		REQUIRE(  shm->getValue(ai) == 100 );
	}

	SECTION("AI calibration rawdata=1 (asAO)")
	{
		ib.rawdata = true;

		shm->setValue(ai, 0);
		REQUIRE( IOBase::processingAsAO(&ib, shm, true) == 0 );

		shm->setValue(ai, -1000);
		REQUIRE( IOBase::processingAsAO(&ib, shm, true) == -1000 );

		shm->setValue(ai, 1000);
		REQUIRE( IOBase::processingAsAO(&ib, shm, true) == 1000 );
	}

	SECTION("AI calibration rawdata=1 (FasAI)")
	{
		ib.rawdata = true;

		IOBase::processingFasAI(&ib, 0.0, shm, true);
		REQUIRE( shm->getValue(ai) == 0 );

		IOBase::processingFasAI(&ib, 1.0f, shm, true);
		REQUIRE( shm->getValue(ai) == 1065353216 );
	}

	SECTION("AI calibration (FasAO)")
	{
		ib.rawdata = true;

		shm->setValue(ai, 0);
		REQUIRE( IOBase::processingFasAO(&ib, shm, true) == 0 );

		shm->setValue(ai, 1065353216);
		REQUIRE( IOBase::processingFasAO(&ib, shm, true) == 1.0f );
	}
}
// -----------------------------------------------------------------------------
TEST_CASE("[IOBase::calibration]: AI calibration (precision)", "[iobase][calibration][precision][ai][extensions]")
{
	CHECK( uniset_conf() != nullptr );
	auto conf = uniset_conf();

	init_test();

	ObjectId ai = conf->getSensorID("CalibrationTest_AI2_AS");
	CHECK( ai != DefaultObjectId );

	IOBase ib;
	CHECK( init_iobase(&ib, "CalibrationTest_AI2_AS") );

	SECTION("AI calibration with precision (asAI)")
	{
		IOBase::processingAsAI(&ib, 0, shm, true);
		REQUIRE( shm->getValue(ai) == 0 );

		IOBase::processingAsAI(&ib, -100, shm, true);
		REQUIRE( shm->getValue(ai) == -100000 );

		IOBase::processingAsAI(&ib, 100, shm, true);
		REQUIRE(  shm->getValue(ai) == 100000 );
	}

	SECTION("AI calibration with precision (asAO)")
	{
		shm->setValue(ai, 0);
		REQUIRE( IOBase::processingAsAO(&ib, shm, true) == 0 );

		shm->setValue(ai, -100000);
		REQUIRE( IOBase::processingAsAO(&ib, shm, true) == -100 );

		shm->setValue(ai, 100000);
		REQUIRE( IOBase::processingAsAO(&ib, shm, true) == 100 );
	}

	SECTION("AI calibration with precision (FasAI)")
	{
		IOBase::processingFasAI(&ib, 0.0, shm, true);
		REQUIRE( shm->getValue(ai) == 0 );

		IOBase::processingFasAI(&ib, -0.99, shm, true); // сперва будет возведено в степень (10^precision)
		REQUIRE( shm->getValue(ai) == -990 ); // потом откалибруется

		IOBase::processingFasAI(&ib, 0.99, shm, true);
		REQUIRE(  shm->getValue(ai) == 990 );

		IOBase::processingFasAI(&ib, 0.87, shm, true);
		REQUIRE(  shm->getValue(ai) == 870 );
	}

	SECTION("AI calibration (FasAO)")
	{
		shm->setValue(ai, 0);
		REQUIRE( IOBase::processingFasAO(&ib, shm, true) == 0 );

		shm->setValue(ai, -990);
		REQUIRE( IOBase::processingFasAO(&ib, shm, true) == -0.99f );

		shm->setValue(ai, -995);
		REQUIRE( IOBase::processingFasAO(&ib, shm, true) == -0.995f );

		shm->setValue(ai, 1000);
		REQUIRE( IOBase::processingFasAO(&ib, shm, true) == 1.0f );

		shm->setValue(ai, 995);
		REQUIRE( IOBase::processingFasAO(&ib, shm, true) == 0.995f );

		shm->setValue(ai, 994);
		REQUIRE( IOBase::processingFasAO(&ib, shm, true) == 0.994f );
	}

	SECTION("AI calibration noprecision=1 (FasAO)")
	{
		ib.noprecision = true;

		shm->setValue(ai, 0);
		REQUIRE( IOBase::processingFasAO(&ib, shm, true) == 0 );

		shm->setValue(ai, -990);
		REQUIRE( IOBase::processingFasAO(&ib, shm, true) == -99.0f );

		shm->setValue(ai, -995);
		REQUIRE( IOBase::processingFasAO(&ib, shm, true) == -99.5f );

		shm->setValue(ai, 1000);
		REQUIRE( IOBase::processingFasAO(&ib, shm, true) == 100.0f );

		shm->setValue(ai, 995);
		REQUIRE( IOBase::processingFasAO(&ib, shm, true) == 99.5f );

		shm->setValue(ai, 994);
		REQUIRE( IOBase::processingFasAO(&ib, shm, true) == 99.4f );

		ib.noprecision = false;
	}

	SECTION("AI calibration noprecision=1 (FasAI)")
	{
		ib.noprecision = true;

		IOBase::processingFasAI(&ib, 0.0, shm, true);
		REQUIRE( shm->getValue(ai) == 0 );

		IOBase::processingFasAI(&ib, -99.5, shm, true); // округлится до -100
		REQUIRE( shm->getValue(ai) == -1000 ); // потом откалибруется

		IOBase::processingFasAI(&ib, 99.4, shm, true); // округлися до 99
		REQUIRE(  shm->getValue(ai) == 990 );

		ib.noprecision = false;
	}
}
// -----------------------------------------------------------------------------
TEST_CASE("[IOBase::calibration]: asDI, asDO", "[iobase][di][do][extensions]")
{
	CHECK( uniset_conf() != nullptr );
	auto conf = uniset_conf();

	init_test();

	SECTION("AI as DI,DO..")
	{
		ObjectId ai = conf->getSensorID("AsDI1_AS");
		CHECK( ai != DefaultObjectId );

		IOBase ib;
		CHECK( init_iobase(&ib, "AsDI1_AS") );

		IOBase::processingAsDI(&ib, false, shm, true);
		REQUIRE( shm->getValue(ai) == 0 );
		REQUIRE( IOBase::processingAsDO(&ib, shm, true) == false );

		IOBase::processingAsDI(&ib, true, shm, true);
		REQUIRE( shm->getValue(ai) == 1 );
		REQUIRE( IOBase::processingAsDO(&ib, shm, true) == true );

		shm->setValue(ai, -10);
		REQUIRE( IOBase::processingAsDO(&ib, shm, true) == 1 );
		shm->setValue(ai, 0);
		REQUIRE( IOBase::processingAsDO(&ib, shm, true) == false );
		shm->setValue(ai, 10);
		REQUIRE( IOBase::processingAsDO(&ib, shm, true) == true );
	}

	SECTION("DI as DI,DO..")
	{
		ObjectId di = conf->getSensorID("AsDI2_S");
		CHECK( di != DefaultObjectId );

		IOBase ib;
		CHECK( init_iobase(&ib, "AsDI2_S") );

		IOBase::processingAsDI(&ib, false, shm, true);
		REQUIRE( shm->getValue(di) == 0 );
		REQUIRE( IOBase::processingAsDO(&ib, shm, true) == false );

		IOBase::processingAsDI(&ib, true, shm, true);
		REQUIRE( shm->getValue(di) == 1 );
		REQUIRE( IOBase::processingAsDO(&ib, shm, true) == true );

		shm->setValue(di, 0);
		REQUIRE( IOBase::processingAsDO(&ib, shm, true) == false );
	}
}
// -----------------------------------------------------------------------------
TEST_CASE("[IOBase]: FasAI to DI", "[iobase][floatToDI]")
{
	CHECK( uniset_conf() != nullptr );

	IOBase ib;
	CHECK( init_iobase(&ib, "FasDI_S") );

	float f = 23.23;

	IOBase::processingFasAI(&ib, f, shm, true);

	CHECK(shm->getValue(121) == 1 );

	f = 0;
	IOBase::processingFasAI(&ib, f, shm, true);
	CHECK(shm->getValue(121) == 0 );
}
// -----------------------------------------------------------------------------
TEST_CASE("IOBase with SM", "[iobase][extensions]")
{
	WARN("IOBase with SM: Not all tests implemented!");
	// ignore
	// ioinvert
	// iofront
	// UndefinedState
	// threshold_ai
	// processingThreshold
}
// -----------------------------------------------------------------------------
TEST_CASE("[IOBase]: FasAO --> FasAI", "[iobase][float]")
{
	CHECK( uniset_conf() != nullptr );

	IOBase ib;
	CHECK( init_iobase(&ib, "FasAI_S") );

	shm->setValue(119, 232);
	float f = IOBase::processingFasAO(&ib, shm, true);

	CHECK( f == 23.2f );

	IOBase::processingFasAI(&ib, f, shm, true);

	CHECK(shm->getValue(119) == 232 );
}
// -----------------------------------------------------------------------------
