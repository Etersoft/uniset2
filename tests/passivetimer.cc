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


	while(1)
	{
		cerr << "timer=" << pt.checkTime() << endl;
		msleep(500);
	}
	return 0;
}
