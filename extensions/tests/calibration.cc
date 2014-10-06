#include <catch.hpp>


#include "Configuration.h"
#include "Extensions.h"
#include "Calibration.h"

using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;

TEST_CASE("Calibration","[calibration]")
{
	Calibration* cal = buildCalibrationDiagram("testcal");
	CHECK( cal!=0 );

	SECTION("getValue")
	{
		REQUIRE( cal->getValue(1500,true) == 600 );
		REQUIRE( cal->getValue(1500,false) == Calibration::outOfRange );
		REQUIRE( cal->getValue(1000) == 600 );
		REQUIRE( cal->getValue(933) == 466 );
		REQUIRE( cal->getValue(800) == 300 );
		REQUIRE( cal->getValue(700) == 250 );
		REQUIRE( cal->getValue(600) == 200 );
		REQUIRE( cal->getValue(650) > 200 );
		REQUIRE( cal->getValue(650) < 250 );
	
		REQUIRE( cal->getValue(200)== 60 );
		REQUIRE( cal->getValue(50) == 20 );
	
		REQUIRE( cal->getValue(10) == 0 );
		REQUIRE( cal->getValue(5) == 0 );
		REQUIRE( cal->getValue(0) == 0 );
		REQUIRE( cal->getValue(-5) == 0 );
		REQUIRE( cal->getValue(-10) == 0 );
		
		REQUIRE( cal->getValue(-50) == -20 );
		REQUIRE( cal->getValue(-500) == -80 );
		REQUIRE( cal->getValue(-900) == -250 );
		REQUIRE( cal->getValue(-1000) == -300 );
		REQUIRE( cal->getValue(-1050,true) == -300 );
		REQUIRE( cal->getValue(-1050,false) == Calibration::outOfRange );
	}

	SECTION("Other function")
	{
		REQUIRE( cal->getMinValue() == -300 );
		REQUIRE( cal->getMaxValue() == 600 );
		REQUIRE( cal->getLeftValue() == -300 );
		REQUIRE( cal->getRightValue() == 600 );

		REQUIRE( cal->getRawValue(-80) == -500 );
		REQUIRE( cal->getRawValue(-500,false) == Calibration::outOfRange );
		REQUIRE( cal->getRawValue(-500,true) == -1000 );
		REQUIRE( cal->getRawValue(150) == 500 );
		REQUIRE( cal->getRawValue(700,false) == Calibration::outOfRange );
		REQUIRE( cal->getRawValue(700,true) == 1000 );

		REQUIRE( cal->getMinRaw() == -1000 );
		REQUIRE( cal->getMaxRaw() == 1000 );
		REQUIRE( cal->getLeftRaw() == -1000 );
		REQUIRE( cal->getRightRaw() == 1000 );
	}
}
