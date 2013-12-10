#include <sstream>
#include "RRDStorage.h"
#include "Configuration.h"
#include "Debug.h"
#include "ObjectsActivator.h"
#include "Extensions.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// -----------------------------------------------------------------------------
int main( int argc, const char** argv )
{
	if( argc>1 && (!strcmp(argv[1],"--help") || !strcmp(argv[1],"-h")) )
	{
		cout << "--smemory-id objectName  - SharedMemory objectID. Default: autodetect" << endl;
		cout << "--confile filename       - configuration file. Default: configure.xml" << endl;
		cout << "--rrdstorage-logfile filename    - logfilename. Default: rrdstorage.log" << endl;
		cout << endl;
		RRDStorage::help_print(argc, argv);
		return 0;
	}

	try
	{
		string confile=UniSetTypes::getArgParam("--confile",argc, argv, "configure.xml");
		conf = new Configuration( argc, argv, confile );

		string logfilename(conf->getArgParam("--rrdstorage-logfile"));
		if( logfilename.empty() )
			logfilename = "rrdstorage.log";

		conf->initDebug(dlog,"dlog");

		std::ostringstream logname;
		string dir(conf->getLogDir());
		logname << dir << logfilename;
		unideb.logFile( logname.str() );
		dlog.logFile( logname.str() );

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

		RRDStorage* db = RRDStorage::init_rrdstorage(argc,argv,shmID);
		if( !db )
		{
			dlog[Debug::CRIT] << "(rrdstorage): init не прошёл..." << endl;
			return 1;
		}

		ObjectsActivator act;
		act.addObject(static_cast<class UniSetObject*>(db));

		SystemMessage sm(SystemMessage::StartUp);
		act.broadcast( sm.transport_msg() );

		unideb(Debug::ANY) << "\n\n\n";
		unideb[Debug::ANY] << "(main): -------------- RRDStorage START -------------------------\n\n";
		dlog(Debug::ANY) << "\n\n\n";
		dlog[Debug::ANY] << "(main): -------------- RRDStorage START -------------------------\n\n";
		act.run(false);
		return 0;
	}
	catch( UniSetTypes::Exception& ex )
	{
		dlog[Debug::CRIT] << "(rrdstorage): " << ex << std::endl;
	}
	catch(...)
	{
		dlog[Debug::CRIT] << "(rrdstorage): catch ..." << std::endl;
	}

	return 1;
}
