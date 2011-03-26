#include <sstream>
#include "ObjectsActivator.h"
#include "Extensions.h"
#include "RTUExchange.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// -----------------------------------------------------------------------------
int main( int argc, char** argv )
{
	try
	{
		if( argc>1 && (!strcmp(argv[1],"--help") || !strcmp(argv[1],"-h")) )
		{
			cout << "--smemory-id objectName  - SharedMemory objectID. Default: read from <SharedMemory>" << endl;
			cout << "--confile filename       - configuration file. Default: configure.xml" << endl;
			cout << "--rs-logfile filename    - logfilename. Default: rtuexchange.log" << endl;
			cout << endl;
			RTUExchange::help_print(argc, argv);
			return 0;
		}

		string confile=UniSetTypes::getArgParam("--confile", argc, argv, "configure.xml");
		conf = new Configuration( argc, argv, confile );

		string logfilename(conf->getArgParam("--rs-logfile"));
		if( logfilename.empty() )
			logfilename = "rtuexchange.log";
	
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

		RTUExchange* rs = RTUExchange::init_rtuexchange(argc,argv,shmID,0,"rs");
		if( !rs )
		{
			dlog[Debug::CRIT] << "(rtuexchange): init не прошёл..." << endl;
			return 1;
		}

		ObjectsActivator act;
		act.addObject(static_cast<class UniSetObject*>(rs));

		SystemMessage sm(SystemMessage::StartUp); 
		act.broadcast( sm.transport_msg() );

		unideb(Debug::ANY) << "\n\n\n";
		unideb[Debug::ANY] << "(main): -------------- RTU Exchange START -------------------------\n\n";
		dlog(Debug::ANY) << "\n\n\n";
		dlog[Debug::ANY] << "(main): -------------- RTU Exchange START -------------------------\n\n";

		act.run(false);
//		msleep(500);
//		rs->execute();
		return 0;
	}
	catch( Exception& ex )
	{
		dlog[Debug::CRIT] << "(rtuexchange): " << ex << std::endl;
	}
	catch(...)
	{
		dlog[Debug::CRIT] << "(rtuexchange): catch ..." << std::endl;
	}

	return 1;
}
