#include <sstream>
#include "ObjectsActivator.h"
#include "Extensions.h"
#include "UDPReceiver.h"
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
			cout << "--udp-logfile filename    - logfilename. Default: udpexchange.log" << endl;
			cout << endl;
			UDPReceiver::help_print(argc,argv);
			return 0;
		}

		string confile=UniSetTypes::getArgParam("--confile",argc,argv,"configure.xml");
		conf = new Configuration( argc, argv, confile );

		string logfilename(conf->getArgParam("--udp-logfile"));
		if( logfilename.empty() )
			logfilename = "udpexchange.log";
	
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

		UDPReceiver* udp = UDPReceiver::init_udpreceiver(argc,argv,shmID);
		if( !udp )
		{
			dlog[Debug::CRIT] << "(udpreceiver): init не прошёл..." << endl;
			return 1;
		}

		ObjectsActivator act;
		act.addObject(static_cast<class UniSetObject*>(udp));

		SystemMessage sm(SystemMessage::StartUp); 
		act.broadcast( sm.transport_msg() );

		unideb(Debug::ANY) << "\n\n\n";
		unideb[Debug::ANY] << "(main): -------------- UDPRecevier START -------------------------\n\n";
		dlog(Debug::ANY) << "\n\n\n";
		dlog[Debug::ANY] << "(main): -------------- UDPReceiver START -------------------------\n\n";

		act.run(false);
//		msleep(500);
//		rs->execute();
	}
	catch( Exception& ex )
	{
		dlog[Debug::CRIT] << "(udpexchange): " << ex << std::endl;
	}
	catch( ost::SockException& e )
	{
		ostringstream s;
		s << e.getString() << ": " << e.getSystemErrorString();
		dlog[Debug::CRIT] << s.str() << endl;
	}
	catch(...)
	{
		dlog[Debug::CRIT] << "(udpexchange): catch ..." << std::endl;
	}

	return 0;
}
