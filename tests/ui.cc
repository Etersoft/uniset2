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
		char dbuf[20];
		snprintf(dbuf,sizeof(dbuf),"%02d/%02d/%04d ", tms->tm_mday, tms->tm_mon+1, tms->tm_year+1900);

		char tbuf[20];
		snprintf(tbuf,sizeof(tbuf),"%02d:%02d:%02d ", tms->tm_hour, tms->tm_min, tms->tm_sec);

	
		cout << "id=" << id
			<< " value=" << inf.value
			<< " last changed: " << string(dbuf) << " " << string(tbuf) 
			<< endl;
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
