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

		int id = conf->getArgInt("--sid");
		if( id <=0 )
		{
			cerr << "unknown sensor ID. Use --sid " << endl;
			return 1;
		}
	
		UniversalInterface ui;

		cout << "getChangedTime for ID=" << id << ":" << endl;

		IOController_i::ShortIOInfo inf = ui.getChangedTime(id,conf->getLocalNode());

		struct tm* tms = localtime(&inf.tv_sec);

		char t_str[ 150 ];
		strftime( t_str, sizeof(t_str), "%d %b %Y %H:%M:%S", tms );
	
		cout << "id=" << id
			<< " value=" << inf.value
			<< " last changed: " << string(t_str) << endl;
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
