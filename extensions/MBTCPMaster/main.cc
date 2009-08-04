// $Id: rsexchange.cc,v 1.2 2008/06/06 11:03:32 pv Exp $
// -----------------------------------------------------------------------------
#include <sstream>
#include "MBMaster.h"
#include "Configuration.h"
#include "Debug.h"
#include "ObjectsActivator.h"
#include "Extensions.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// -----------------------------------------------------------------------------
int main( int argc, char** argv )
{
	if( argc>1 && (!strcmp(argv[1],"--help") || !strcmp(argv[1],"-h")) )
	{
		cout << "--smemory-id objectName  - SharedMemory objectID. Default: autodetect" << endl;
		cout << "--confile filename       - configuration file. Default: configure.xml" << endl;
		cout << "--mbtcp-logfile filename    - logfilename. Default: mbtcpmaster.log" << endl;
		cout << endl;
		MBMaster::help_print(argc, argv);
		return 0;
	}

	try
	{
		string confile=UniSetTypes::getArgParam("--confile",argc, argv, "configure.xml");
		conf = new Configuration( argc, argv, confile );

		string logfilename(conf->getArgParam("--mbtcp-logfile"));
		if( logfilename.empty() )
			logfilename = "mbtcpmaster.log";
	
		conf->initDebug(dlog,"dlog");
	
		std::ostringstream logname;
		string dir(conf->getLogDir());
		logname << dir << logfilename;
		unideb.logFile( logname.str().c_str() );
		dlog.logFile( logname.str().c_str() );

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

		MBMaster* mb = MBMaster::init_mbmaster(argc,argv,shmID);
		if( !mb )
		{
			dlog[Debug::CRIT] << "(mbmaster): init не прошёл..." << endl;
			return 1;
		}

		ObjectsActivator act;
		act.addObject(static_cast<class UniSetObject*>(mb));

		SystemMessage sm(SystemMessage::StartUp); 
		act.broadcast( sm.transport_msg() );

		unideb(Debug::ANY) << "\n\n\n";
		unideb[Debug::ANY] << "(main): -------------- MBTCP Exchange START -------------------------\n\n";
		dlog(Debug::ANY) << "\n\n\n";
		dlog[Debug::ANY] << "(main): -------------- MBTCP Exchange START -------------------------\n\n";
		act.run(false);
	}
	catch( Exception& ex )
	{
		dlog[Debug::CRIT] << "(mbtcpmaster): " << ex << std::endl;
	}
	catch(...)
	{
		dlog[Debug::CRIT] << "(mbtcpmaster): catch ..." << std::endl;
	}

	return 0;
}
