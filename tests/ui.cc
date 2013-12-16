#include <time.h>
#include "UniversalInterface.h"
#include "UniSetTypes.h"

using namespace std;
using namespace UniSetTypes;

int main( int argc, const char **argv )
{
	try
	{
		uniset_init(argc,argv,"test.xml");
		UniversalInterface ui;

		cout << "** check getSensorID function **" << endl;
		ObjectId id1 = conf->getSensorID("Input1_S");
		if( id1 != 1 )
		{
			cout << "** FAILED! check getSensorID function **" << endl;
			return 1;
		}

		cout << "** check getConfIOType function **" << endl;
		UniversalIO::IOType t = ui.getConfIOType(id1);
		cout << "sensor ID=" << id1 << " iotype=" << t << endl;
		if( t != UniversalIO::DI )
		{
			cout << "** FAILED! check getSensorID function **" << endl;
			return 1;
		}

		int id = conf->getArgInt("--sid");
		if( id <= 0 )
		{
			cerr << "unknown sensor ID. Use --sid " << endl;
			return 1;
		}

		cout << "** check getChangedTime for ID=" << id << ":" << endl;

		IOController_i::ShortIOInfo inf = ui.getChangedTime(id,conf->getLocalNode());

		struct tm* tms = localtime(&inf.tv_sec);

		char t_str[ 150 ];
		strftime( t_str, sizeof(t_str), "%d %b %Y %H:%M:%S", tms );
	
		cout << "id=" << id
			<< " value=" << inf.value
			<< " last changed: " << string(t_str) << endl;

		cout << "check getValue: " << ui.getValue(id1) << endl;
		cout << "check setValue: id='" << id1 << "' val=2 ...";
		ui.setValue(id1,2);
		cout << ( ui.getValue(id1) == 2 ? "OK" : "FAIL" ) << endl;
	}
	catch( Exception& ex )
	{
		cout << "(uitest):" << ex << endl;
	}
	catch(...)
	{
		cout << "(uitest): catch ..."<< endl;
	}
	

	return 0;
}
