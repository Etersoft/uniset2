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

#if 0
#include <iostream>
#include "Exceptions.h"
#include "UniXML.h"
#include "UniSetTypes.h"
#include "Configuration.h"
#include "Extensions.h"
#include "Calibration.h"

#include <vector>
#include <deque>

using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;

int main( int argc, const char** argv )
{
    try
    {
        uniset_init(argc,argv,"test.xml");
        Calibration* cal = buildCalibrationDiagram("testcal");

        cout << "diagram: " << cal << endl;
        cout << "-1500 --> " << cal->getValue(-1500) << endl;
        cout << "-933 --> " << cal->getValue(-933) << endl;
        cout << "-844 --> " << cal->getValue(-844) << endl;
        cout << "-540 --> " << cal->getValue(-540) << endl;
        cout << "-320 --> " << cal->getValue(-320) << endl;
        cout << "-200 --> " << cal->getValue(-200) << endl;
        // проверка кэша....
        cout << "-200 --> " << cal->getValue(-200) << endl;
        cout << "-200 --> " << cal->getValue(-200) << endl;
        cout << "-200 --> " << cal->getValue(-200) << endl;
        cout << "-200 --> " << cal->getValue(-200) << endl;
        // --------------
        cout << "-100 --> " << cal->getValue(-100) << endl;
        cout << "-200 --> " << cal->getValue(-200) << endl;
        cout << "-100 --> " << cal->getValue(-100) << endl;
        cout << " -75 --> " << cal->getValue(-75) << endl;
        cout << " -50 --> " << cal->getValue(-50) << endl;
        cout << " -25 --> " << cal->getValue(-25) << endl;
        cout << "  -0 --> " << cal->getValue(0) << endl;
        cout << "  25 --> " << cal->getValue(25) << endl;
        cout << "  50 --> " << cal->getValue(50) << endl;
        cout << "  75 --> " << cal->getValue(75) << endl;
        cout << " 100 --> " << cal->getValue(100) << endl;
        cout << " 200 --> " << cal->getValue(200) << endl;
        cout << " 310 --> " << cal->getValue(310) << endl;
        cout << " 415 --> " << cal->getValue(415) << endl;
        cout << " 556 --> " << cal->getValue(556) << endl;
        cout << " 873 --> " << cal->getValue(873) << endl;
        cout << " 1500 --> " << cal->getValue(1500) << endl;

        cout << endl << " RAW VALUE.." << endl;
        cout << "-1220 --> " << cal->getRawValue(-1220) << endl;
        cout << " -200 --> " << cal->getRawValue(-200) << endl;
        cout << "  -60 --> " << cal->getRawValue(-60) << endl;
        cout << "  -40 --> " << cal->getRawValue(-40) << endl;
        cout << "  -20 --> " << cal->getRawValue(-20) << endl;
        cout << "    0 --> " << cal->getRawValue(0) << endl;
        cout << "   20 --> " << cal->getRawValue(20) << endl;
        cout << "   40 --> " << cal->getRawValue(40) << endl;
        cout << "   60 --> " << cal->getRawValue(60) << endl;
        cout << "  200 --> " << cal->getRawValue(200) << endl;
        cout << " 1500 --> " << cal->getRawValue(1500) << endl;

        return 0;
    }
    catch( Exception& ex )
    {
        cerr << "(main): " << ex << std::endl;
    }
    catch(...)
    {
        cerr << "(main): catch ..." << std::endl;
    }

    return 1;
}
#endif