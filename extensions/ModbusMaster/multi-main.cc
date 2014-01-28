#include <sys/wait.h>
#include <sstream>
#include "MBTCPMultiMaster.h"
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
		cout << "--smemory-id objectName  - SharedMemory objectID. Default: get from configure..." << endl;
		cout << "--confile filename       - configuration file. Default: configure.xml" << endl;
		cout << "--mbtcp-logfile filename    - logfilename. Default: mbtcpmultimaster.log" << endl;
		cout << endl;
		MBTCPMultiMaster::help_print(argc, argv);
		return 0;
	}

	try
	{
		string confile=UniSetTypes::getArgParam("--confile",argc, argv, "configure.xml");
		conf = new Configuration( argc, argv, confile );

		string logfilename(conf->getArgParam("--mbtcp-logfile"));
		if( logfilename.empty() )
			logfilename = "mbtcpmultimaster.log";

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

		MBTCPMultiMaster* mb = MBTCPMultiMaster::init_mbmaster(argc,argv,shmID);
		if( !mb )
		{
			dlog[Debug::CRIT] << "(mbmaster): init MBTCPMaster failed." << endl;
			return 1;
		}

		ObjectsActivator act;
		act.addObject(static_cast<class UniSetObject*>(mb));

		SystemMessage sm(SystemMessage::StartUp);
		act.broadcast( sm.transport_msg() );

		unideb(Debug::ANY) << "\n\n\n";
		unideb[Debug::ANY] << "(main): -------------- MBTCPMulti Exchange START -------------------------\n\n";
		dlog(Debug::ANY) << "\n\n\n";
		dlog[Debug::ANY] << "(main): -------------- MBTCPMulti Exchange START -------------------------\n\n";
		act.run(false);
		while( waitpid(-1, 0, 0) > 0 );
		return 0;
	}
	catch( Exception& ex )
	{
		dlog[Debug::CRIT] << "(mbtcpmultimaster): " << ex << std::endl;
	}
	catch(...)
	{
		dlog[Debug::CRIT] << "(mbtcpmultimaster): catch ..." << std::endl;
	}

	while( waitpid(-1, 0, 0) > 0 );
	return 1;
}
