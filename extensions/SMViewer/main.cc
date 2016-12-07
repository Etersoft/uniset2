#include <iostream>
#include "Configuration.h"
#include "SMViewer.h"
#include "Extensions.h"
// -----------------------------------------------------------------------------
using namespace uniset;
using namespace uniset::extensions;
using namespace std;
// -----------------------------------------------------------------------------
int main( int argc, const char** argv )
{
	try
	{
		if( argc > 1 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) )
		{
			cout << "--smemory-id objectName  - SharedMemory objectID. Default: read from <SharedMemory>" << endl;
			cout << "--confile filename       - configuration file. Default: configure.xml" << endl;
			return 0;
		}

		auto conf = uniset_init( argc, argv );

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
		return 0;
	}
	catch( const uniset::Exception& ex )
	{
		cout << "(main):" << ex << endl;
	}
	catch(...)
	{
		cout << "Неизвестное исключение!!!!" << endl;
	}

	return 1;
}
// ------------------------------------------------------------------------------------------
