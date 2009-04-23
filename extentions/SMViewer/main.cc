#include <iostream>
#include "Configuration.h"
#include "SMViewer.h"
#include "Extentions.h"
// -----------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace UniSetExtentions;
using namespace std;
// -----------------------------------------------------------------------------
int main( int argc, char **argv )
{
	try
	{
		if( argc>1 && (!strcmp(argv[1],"--help") || !strcmp(argv[1],"-h")) )
		{
			cout << "--smemory-id objectName  - SharedMemory objectID. Default: read from <SharedMemory>" << endl;
			cout << "--confile filename       - configuration file. Default: configure.xml" << endl;
			return 0;
		}

		string confile = UniSetTypes::getArgParam( "--confile", argc, argv, "configure.xml" );
		conf = new Configuration( argc, argv, confile );

		ObjectId shmID = DefaultObjectId;
		string sID = conf->getArgParam("--smemory-id");
		if( !sID.empty() )
			shmID = conf->getControllerID(sID);
		else
			shmID = getSharedMemoryID();

		if( shmID == DefaultObjectId )
		{
			cerr << sID << "? SharedMemoryID not found in " << conf->getControllersSection() << " section" << endl;
			return 1;
		}

		SMViewer smv(shmID);
		smv.run();
	}
	catch( Exception& ex )
	{
		cout << "(main):" << ex << endl;
	}
	catch(...)
	{
		cout << "Неизвестное исключение!!!!"<< endl;
	}

	return 0;
}
// ------------------------------------------------------------------------------------------
