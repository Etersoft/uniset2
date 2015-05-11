#include <iostream>
#include "Configuration.h"
#include "Extensions.h"
#include "UniSetActivator.h"
#include "PassiveLProcessor.h"

// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// -----------------------------------------------------------------------------
int main(int argc, const char** argv)
{
	std::ios::sync_with_stdio(false);

	if( argc > 1 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) )
	{
		cout << "--smemory-id objectName  - SharedMemory objectID. Default: autodetect" << endl;
		cout << "--confile filename       - configuration file. Default: configure.xml" << endl;
		cout << "--plproc-logfile filename   - logfilename. Default: mbslave.log" << endl;
		cout << endl;
		PassiveLProcessor::help_print(argc, argv);
		return 0;
	}

	try
	{
		auto conf = uniset_init( argc, argv );

		string logfilename(conf->getArgParam("--plproc-logfile"));

		if( logfilename.empty() )
			logfilename = "logicproc.log";

		std::ostringstream logname;
		string dir(conf->getLogDir());
		logname << dir << logfilename;
		ulog()->logFile( logname.str() );
		dlog()->logFile( logname.str() );

		string schema = conf->getArgParam("--schema");

		if( schema.empty() )
		{
			cerr << "schema-file not defined. Use --schema" << endl;
			return 1;
		}

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

		auto plp = PassiveLProcessor::init_plproc(argc, argv, shmID);

		auto act = UniSetActivator::Instance();
		act->add(plp);

		SystemMessage sm(SystemMessage::StartUp);
		act->broadcast( sm.transport_msg() );

		ulogany << "\n\n\n";
		ulogany << "(main): -------------- IOControl START -------------------------\n\n";
		dlogany << "\n\n\n";
		dlogany << "(main): -------------- IOControl START -------------------------\n\n";
		act->run(false);
		return 0;
	}
	catch( const LogicException& ex )
	{
		cerr << ex << endl;
	}
	catch( const Exception& ex )
	{
		cerr << ex << endl;
	}
	catch( ... )
	{
		cerr << " catch ... " << endl;
	}

	return 1;
}
// -----------------------------------------------------------------------------
