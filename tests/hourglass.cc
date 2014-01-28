#include <iostream>

using namespace std;

#include "HourGlass.h"
#include "DelayTimer.h"
#include "UniSetTypes.h"

int main()
{
    HourGlass hg;
    
    hg.run(1000);
    hg.rotate(true);
    msleep(200);
    if( hg.check() )
    {
        cerr << "HourGlass: TEST1 FAILED! " << endl;
        return 1;
    }
    
    msleep(1000);
    if( !hg.check() )
    {
        cerr << "HourGlass: TEST1 FAILED! " << endl;
        return 1;
    }
    
    cout << "HourGlass: TEST1 OK!" << endl;

    hg.rotate(false);
    msleep(1000);
    if( hg.check() )
    {
        cerr << "HourGlass: TEST2 FAILED! " << endl;
        return 1;
    }
    cout << "HourGlass: TEST2 OK!" << endl;

    hg.rotate(true);
    msleep(500);
    if( hg.check() )
    {
        cerr << "HourGlass: TEST3 FAILED! " << endl;
        return 1;
    }
    cout << "HourGlass: TEST3 OK!" << endl;

    hg.rotate(false);
    msleep(200);
    hg.rotate(true);
    msleep(200);
    if( hg.check() )
    {
        cerr << "HourGlass: TEST4 FAILED! " << endl;
        return 1;
    }

    msleep(820);
    if( !hg.check() )
    {
        cerr << "HourGlass: TEST5 FAILED! " << endl;
        return 1;
    }

    cout << "HourGlass: TEST4 OK!" << endl;
    return 0;
}
