#include <iostream>

using namespace std;

#include "PassiveTimer.h"
#include "UniSetTypes.h"

PassiveTimer pt(1000);

int main()
{
	PassiveTimer pt1(5000);
	cout << " pt1.getInterval()=" << pt1.getInterval() << " TEST: " << ((pt1.getInterval()==5000) ? "OK" : "FAILED") << endl;
	
	PassiveTimer pt2;
	cout << " pt2.getInterval()=" << pt2.getInterval() << endl;
	if( pt2.getInterval() != 0 )
	{
		cerr << "BAD DEFAULT INITIALIZATION!!!" << endl;
		return 1;
	}

	PassiveTimer pt3(UniSetTimer::WaitUpTime);
	cout << "pt3.getCurrent(): " << pt3.getCurrent() << endl;
	msleep(3000);
	int pt3_ms = pt3.getCurrent();
	cout << "pt3.getCurrent(): " << pt3_ms << endl;
	if( pt3_ms < 3000 )
	{
		cerr << "BAD getCurrent() function for WaitUpTime timer (pt3)" << endl;
		return 1;
	}


	PassiveTimer pt0(0);
	cout << "pt0: check msec=0: " << ( pt0.checkTime() ? "OK" : "FAILED" ) << endl;


	PassiveTimer pt4(350);

	for( int i=0;i<12; i++ )
	{
		cerr << "pt4: check time = " << pt4.checkTime() << endl;
		if( pt4.checkTime() )
		{
			cerr << "pt4: reset..." << endl;
			pt4.reset();
		}
		msleep(200);
	}

	while(1)
	{
		cerr << "timer=" << pt.checkTime() << endl;
		msleep(500);
	}
	return 0;
}
