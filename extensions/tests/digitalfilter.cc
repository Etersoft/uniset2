#include <iostream>
#include <vector>
#include <iomanip>

#include "Exceptions.h"
#include "Extensions.h"
#include "DigitalFilter.h"


using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;

int main( int argc, const char** argv )
{
	try
	{
        DigitalFilter df;
        DigitalFilter df_m;

        vector<long> dat={0,234,356,344,234,320,250,250,250,250,250,250,250,251,252,251,252,252,250};

        for( auto v: dat )
        {
		df.add(v);

		cout << "[" << setw(4) << v << "]: "
		     << "   filter1: " << setw(4) << df.current1()
		     << "  filterRC: " << setw(4) << df.currentRC()
		     << "    median: " << setw(4) << df_m.median(v)
		     << endl;
        }

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
