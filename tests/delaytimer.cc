#include <Catch/catch.hpp>

#include "DelayTimer.h"
#include "UniSetTypes.h"
using namespace std;

TEST_CASE("DelayTimer", "[DelayTimer]" ) 
{
    SECTION( "Default constructor" ) 
    {
		// Работа без задержки..(нулевые задержки)
		DelayTimer dt;
		CHECK_FALSE( dt.get() );
		CHECK_FALSE( dt.check(false) );
		CHECK( dt.check(true) );
		msleep(15);
		CHECK( dt.get() );
		CHECK_FALSE( dt.check(false) );
		CHECK_FALSE( dt.get() );
		CHECK( dt.check(true) );
		CHECK( dt.get() );
    }

    SECTION( "Init constructor" )
    {
		DelayTimer dt(100,50);
		CHECK_FALSE( dt.get() );
		CHECK_FALSE( dt.check(false) );
		CHECK_FALSE( dt.check(true) );
    }

    SECTION( "Working" )
    {
		DelayTimer dt(100,60);
		CHECK_FALSE( dt.get() );
		CHECK_FALSE( dt.check(false) );

		// проверяем срабатывание..
		CHECK_FALSE( dt.check(true) );
		msleep(50);
		CHECK_FALSE( dt.check(true) );
		msleep(60);
		CHECK( dt.check(true) );
		CHECK( dt.get() );
	
		// проверяем отпускание
		// несмотря на вызов check(false).. должно ещё 60 мсек возвращать true
		CHECK( dt.check(false) );
		CHECK( dt.get() );
		msleep(20);
		CHECK( dt.check(false) );
		CHECK( dt.get() );
		msleep(50); // в сумме уже 20+50=70 > 60, значит должно "отпустить"
		CHECK_FALSE( dt.check(false) );
		CHECK_FALSE( dt.get() );
    }

    SECTION( "Debounce" )
    {
		DelayTimer dt(150,100);
		CHECK_FALSE( dt.get() );
		CHECK_FALSE( dt.check(false) );

		// проверяем срабатывание.. (при скакании сигнала)
		CHECK_FALSE( dt.check(true) );
		msleep(50);
		CHECK_FALSE( dt.check(false) );
		msleep(60);
		CHECK_FALSE( dt.check(true) );
		CHECK_FALSE( dt.get() );
	
		msleep(100);
		CHECK_FALSE( dt.check(true) );
		CHECK_FALSE( dt.get() );
		msleep(60);
		CHECK( dt.check(true) );
		CHECK( dt.get() );

		// проверяем отпускание при скакании сигнала
		CHECK( dt.check(false) );
		CHECK( dt.get() );
		msleep(60);
		CHECK( dt.check(true) );
		CHECK( dt.get() );
		dt.check(false);
		msleep(80);
		CHECK( dt.check(false) );
		CHECK( dt.get() );
		msleep(40);
		CHECK_FALSE( dt.check(false) );
		CHECK_FALSE( dt.get() );
    }

    SECTION( "Copy" )
    {
		DelayTimer dt1(100,50);
		DelayTimer dt2(200,100);
		
		dt1 = dt2;
	
		REQUIRE( dt1.getOnDelay() == 200 );
		REQUIRE( dt1.getOffDelay() == 100 );
		
		dt1.check(true);
		msleep(220);
		CHECK( dt1.get() );
		CHECK_FALSE( dt2.get() );
	}
}


#if 0
#include <iostream>

using namespace std;

#include "HourGlass.h"
#include "DelayTimer.h"
#include "UniSetTypes.h"

int main()
{
    // ---------------------- 
    // test DelayTimer
    DelayTimer dtm(1000,500);

    if( dtm.check(true) )
    {
        cerr << "DelayTimer: TEST1 FAILED! " << endl;
        return 1;
    }
    cout << "DelayTimer: TEST1 OK!" << endl;

    msleep(1100);
    if( !dtm.check(true) )
    {
        cerr << "DelayTimer: TEST2 FAILED! " << endl;
        return 1;
    }
    cout << "DelayTimer: TEST2 OK!" << endl;

    if( !dtm.check(false) )
    {
        cerr << "DelayTimer: TEST3 FAILED! " << endl;
        return 1;
    }
    cout << "DelayTimer: TEST3 OK!" << endl;

    msleep(200);
    if( !dtm.check(false) )
    {
        cerr << "DelayTimer: TEST4 FAILED! " << endl;
        return 1;
    }
    cout << "DelayTimer: TEST4 OK!" << endl;

    msleep(500);
    if( dtm.check(false) )
    {
        cerr << "DelayTimer: TEST5 FAILED! " << endl;
        return 1;
    }
    cout << "DelayTimer: TEST5 OK!" << endl;

    dtm.check(true);
    msleep(800);
    if( dtm.check(true) )
    {
        cerr << "DelayTimer: TEST6 FAILED! " << endl;
        return 1;
    }

    cout << "DelayTimer: TEST6 OK!" << endl;

    dtm.check(false);
    msleep(600);
    if( dtm.check(false) )
    {
        cerr << "DelayTimer: TEST7 FAILED! " << endl;
        return 1;
    }
    cout << "DelayTimer: TEST7 OK!" << endl;

    dtm.check(true);
    msleep(1100);
    if( !dtm.check(true) )
    {
        cerr << "DelayTimer: TEST8 FAILED! " << endl;
        return 1;
    }
    cout << "DelayTimer: TEST8 OK!" << endl;


    DelayTimer dtm2(200,0);
    dtm2.check(true);
    msleep(250);
    if( !dtm2.check(true) )
    {
        cerr << "DelayTimer: TEST9 FAILED! " << endl;
        return 1;    
    }
    cerr << "DelayTimer: TEST9 OK! " << endl;

    if( dtm2.check(false) )
    {
        cerr << "DelayTimer: TEST10 FAILED! " << endl;
        return 1;    
    }
    cerr << "DelayTimer: TEST10 OK! " << endl;


    DelayTimer dtm3(200,100);
    dtm3.check(true);
    msleep(190);
    dtm3.check(false);
    dtm3.check(true);
    msleep(50);
    if( dtm3.check(true) )
    {
        cerr << "DelayTimer: TEST11 FAILED! " << endl;
        return 1;    
    }
    cerr << "DelayTimer: TEST11 OK! " << endl;

    msleep(200);
    if( !dtm3.check(true) )
    {
        cerr << "DelayTimer: TEST12 FAILED! " << endl;
        return 1;    
    }
    cerr << "DelayTimer: TEST12 OK! " << endl;

    dtm3.check(false);
    msleep(90);
    dtm3.check(true);
    msleep(50);
    if( !dtm3.check(false) )
    {
        cerr << "DelayTimer: TEST13 FAILED! " << endl;
        return 1;
    }
    cerr << "DelayTimer: TEST13 OK! " << endl;
    msleep(150);
    if( dtm3.check(false) )
    {
        cerr << "DelayTimer: TEST14 FAILED! " << endl;
        return 1;
    }
    cerr << "DelayTimer: TEST14 OK! " << endl;
    return 0;
}
#endif