// --------------------------------------------------------------------------
#include <string>
#include "Debug.h"
#include "Configuration.h"
#include "ORepHelpers.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
// --------------------------------------------------------------------------
int main(int argc, const char **argv)
{   
//	if( argc>1 && strcmp(argv[1],"--help")==0 )
//	{
//		cout << "--confile	- Configuration file. Default: test.xml" << endl;
//		return 0;
//	}

	try
	{
//		string confile = UniSetTypes::getArgParam( "--confile", argc, argv, "test.xml" );
//		conf = new Configuration(argc, argv, confile);
		for( int i=-5; i<5; i++ )
		{

			cout <<  "raw=" << (817+i)
				 <<  "   cal=" << lcalibrate(817+i,817,4095,0,400)
				 << endl;
		}

		return 0;
	}
	catch(SystemError& err)
	{
		cerr << "(calibration): " << err << endl;
	}
	catch(Exception& ex)
	{
		cerr << "(calibration): " << ex << endl;
	}
	catch(...)
	{
		cerr << "(calibration): catch(...)" << endl;
	}

	return 1;
}
