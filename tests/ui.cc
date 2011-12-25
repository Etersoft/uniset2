#include <time.h>
#include "UniversalInterface.h"
#include "UniSetTypes.h"

using namespace std;
using namespace UniSetTypes;

int main( int argc, const char **argv )
{
	try
	{
	//UniSetTypes::Configuration* myconf = new UniSetTypes::Configuration(argc,argv,"test.xml");
	//	UniversalInterface* ui = new UniversalInterface(myconf);
		//cout << "************************ myconf=" << myconf << " conf=" << UniSetTypes::conf << endl;

	UniSetTypes::Configuration* myconf2 = new UniSetTypes::Configuration(argc,argv,"conf21300.xml");
		UniversalInterface* ui2 = new UniversalInterface(myconf2);

		cout << "************************ myconf2=" << myconf2 << " conf=" << UniSetTypes::conf << endl;

//   		cout << "Conf1: get=" << ui1->getValue(12) << endl;
		cout << "Conf2: get=" << ui2->getValue(200033) << endl;
#if 0
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
		UniversalIO::IOTypes t = ui.getConfIOType(id1);
		cout << "sensor ID=" << id1 << " iotype=" << t << endl;
		if( t != UniversalIO::DigitalInput )
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
#endif
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
