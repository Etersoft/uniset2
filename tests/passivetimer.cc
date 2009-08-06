#include <iostream>

using namespace std;

#include "PassiveTimer.h"
#include "UniSetTypes.h"

PassiveTimer pt(1000);

int main()
{
	while(1)
	{
		cerr << "timer=" << pt.checkTime() << endl;
		msleep(500);
	}
	return 0;
}
