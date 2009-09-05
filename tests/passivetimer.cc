#include <iostream>

using namespace std;

#include "PassiveTimer.h"
#include "UniSetTypes.h"

PassiveTimer pt(1000);

int main()
{
	PassiveTimer pt1(5000);
	cout << " pt1.getInterval()=" << pt1.getInterval() << endl;
	
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


	while(1)
	{
		cerr << "timer=" << pt.checkTime() << endl;
		msleep(500);
	}
	return 0;
}
