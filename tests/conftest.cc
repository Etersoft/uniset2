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
	if( argc>1 && strcmp(argv[1],"--help")==0 )
	{
		cout << "--confile	- Configuration file. Default: test.xml" << endl;
		return 0;
	}

	try
	{
		string confile = UniSetTypes::getArgParam( "--confile", argc, argv, "test.xml" );
		conf = new Configuration(argc, argv, confile);

		string t(conf->oind->getTextName(1));
		cout << "**** check getTextName: " << ( t.empty() ?  "FAILED" : "OK" ) << endl;

		string ln("/Projects/Sensors/VeryVeryLongNameSensor_ForTest_AS");
		string ln_t("VeryVeryLongNameSensor_ForTest_AS");
		cout << "**** check getShortName: " << ( ln_t == ORepHelpers::getShortName(ln) ? "OK" : "FAILED" ) << endl;

		string mn(conf->oind->getMapName(1));
		cout << "**** check getMapName: " << ( mn.empty() ?  "FAILED" : "OK" ) << endl;

		UniversalIO::IOTypes t1=conf->getIOType(1);
		cout << "**** getIOType for " << mn << endl;
		cout << "**** check getIOType(id): (" << t1 << ") " << ( t1 == UniversalIO::UnknownIOType ?  "FAILED" : "OK" ) << endl;		
		UniversalIO::IOTypes t2=conf->getIOType(mn);
		cout << "**** check getIOType(name): (" << t2 << ") " << ( t2 == UniversalIO::UnknownIOType ?  "FAILED" : "OK" ) << endl;		
		UniversalIO::IOTypes t3=conf->getIOType("Input1_S");
		cout << "**** check getIOType(name): for short name 'Input1_S': (" << t3 << ") " << ( t3 == UniversalIO::UnknownIOType ?  "FAILED" : "OK" ) << endl;

		return 0;
	}
	catch(SystemError& err)
	{
		cerr << "(conftest): " << err << endl;
	}
	catch(Exception& ex)
	{
		cerr << "(conftest): " << ex << endl;
	}
	catch(...)
	{
		cerr << "(conftest): catch(...)" << endl;
	}
	
	return 1;
}
