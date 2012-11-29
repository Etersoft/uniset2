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
