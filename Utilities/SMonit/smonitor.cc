#include <iostream>
#include <string>
#include "ObjectsActivator.h"
#include "Configuration.h"
#include "SMonitor.h"
// -----------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
// -----------------------------------------------------------------------------
int main( int argc, const char **argv )
{
	try
	{
		if( argc>1 && ( !strcmp(argv[1],"--help") || !strcmp(argv[1],"-h") ) )
		{
			cout << "Usage: uniset-smonit [ args ] --sid id1@node1,Sensor2@node2,id2,sensorname3,... "
//				 << " --script scriptname \n"
				 << " --confile configure.xml \n";
			return 0;
		}

		uniset_init(argc,argv,"configure.xml");

		ObjectId ID(DefaultObjectId);
		string name = conf->getArgParam("--name", "TestProc");

		ID = conf->getObjectID(name);
		if( ID == UniSetTypes::DefaultObjectId )
		{
			cerr << "(main): идентификатор '" << name
				<< "' не найден в конф. файле!"
				<< " в секции " << conf->getObjectsSection() << endl;
			return 0;
		}

		ObjectsActivator act;
		SMonitor tp(ID);
		act.addObject(&tp);

		SystemMessage sm(SystemMessage::StartUp);
		act.broadcast( sm.transport_msg() );
		act.run(false);
	}
	catch( Exception& ex )
	{
		cout << "(main):" << ex << endl;
	}
	catch(...)
	{
		cout << "(main): Неизвестное исключение!!!!"<< endl;
	}

	return 0;
}
// ------------------------------------------------------------------------------------------
