// --------------------------------------------------------------------------
#include <sys/wait.h>
#include <sstream>
#include <string>
#include <cc++/socket.h>
#include "MBSlave.h"
#include "Configuration.h"
#include "Debug.h"
#include "UniSetActivator.h"
#include "Extensions.h"

// --------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace UniSetExtensions;
using namespace std;
// --------------------------------------------------------------------------
int main(int argc, const char **argv)
{

	if( argc>1 && (!strcmp(argv[1],"--help") || !strcmp(argv[1],"-h")) )
	{
		cout << "--smemory-id objectName  - SharedMemory objectID. Default: autodetect" << endl;
		cout << "--confile filename       - configuration file. Default: configure.xml" << endl;
		cout << "--mbs-logfile filename    - logfilename. Default: mbslave.log" << endl;
		cout << endl;
		MBSlave::help_print(argc,argv);
		return 0;
	}

	try
	{
		string confile = UniSetTypes::getArgParam( "--confile", argc, argv, "configure.xml" );
		conf = new Configuration(argc, argv,confile);

		string logfilename(conf->getArgParam("--mbs-logfile"));
		if( logfilename.empty() )
			logfilename = "mbslave.log";
	
		std::ostringstream logname;
		string dir(conf->getLogDir());
		logname << dir << logfilename;
		ulog.logFile( logname.str() );
		dlog.logFile( logname.str() );

		conf->initDebug(dlog,"dlog");

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

		MBSlave* s = MBSlave::init_mbslave(argc,argv,shmID);
		if( !s )
		{
			dlog.crit() << "(mbslave): init не прошёл..." << endl;
			return 1;
		}

		UniSetActivator act;
		act.addObject(static_cast<class UniSetObject*>(s));
		SystemMessage sm(SystemMessage::StartUp); 
		act.broadcast( sm.transport_msg() );

		ulog << "\n\n\n";
		ulog << "(main): -------------- MBSlave START -------------------------\n\n";
		dlog << "\n\n\n";
		dlog << "(main): -------------- MBSlave START -------------------------\n\n";

		act.run(false);
		while( waitpid(-1, 0, 0) > 0 );
		return 0;
	}
	catch( SystemError& err )
	{
		dlog.crit() << "(mbslave): " << err << endl;
	}
	catch( Exception& ex )
	{
		dlog.crit() << "(mbslave): " << ex << endl;
	}
	catch( std::exception& e )
	{
		dlog.crit() << "(mbslave): " << e.what() << endl;
	}
	catch(...)
	{
		dlog.crit() << "(mbslave): catch(...)" << endl;
	}

	while( waitpid(-1, 0, 0) > 0 );
	return 1;
}
// --------------------------------------------------------------------------
