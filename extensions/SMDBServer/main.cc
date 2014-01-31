#include <sstream>
#include "SMDBServer.h"
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
		cout << "--smdbserver-logfile filename    - logfilename. Default: smdbserver.log" << endl;
		cout << endl;
		SMDBServer::help_print(argc, argv);
		return 0;
	}

	try
	{
		string confile=UniSetTypes::getArgParam("--confile",argc, argv, "configure.xml");
		conf = new Configuration( argc, argv, confile );

		string logfilename(conf->getArgParam("--smdbserver-logfile"));
		if( logfilename.empty() )
			logfilename = "smdbserver.log";
	
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

		SMDBServer* db = SMDBServer::init_smdbserver(argc,argv,shmID);
		if( !db )
		{
			dlog[Debug::CRIT] << "(smdbserver): init не прошёл..." << endl;
			return 1;
		}

		ObjectsActivator act;
		act.addObject(static_cast<class UniSetObject*>(db));

		SystemMessage sm(SystemMessage::StartUp); 
		act.broadcast( sm.transport_msg() );

		unideb(Debug::ANY) << "\n\n\n";
		unideb[Debug::ANY] << "(main): -------------- SMDBServer START -------------------------\n\n";
		dlog(Debug::ANY) << "\n\n\n";
		dlog[Debug::ANY] << "(main): -------------- SMDBServer START -------------------------\n\n";
		act.run(false);
	}
	catch( Exception& ex )
	{
		dlog[Debug::CRIT] << "(smdbserver): " << ex << std::endl;
	}
	catch(...)
	{
		dlog[Debug::CRIT] << "(smdbserver): catch ..." << std::endl;
	}

	return 0;
}
