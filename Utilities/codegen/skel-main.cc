#include <sstream>
#include <ObjectsActivator.h>
#include "Skel.h"
// -----------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
// -----------------------------------------------------------------------------
int main( int argc, const char **argv )
{
	try
	{
		string confile = uniset_init(argc, argv);

		string logfilename = conf->getArgParam("--logfile", "Skel.log");
		string logname( conf->getLogDir() + logfilename );
//		dlog.logFile( logname.c_str() );
		unideb.logFile( logname.c_str() );
//		conf->initDebug(dlog,"dlog");

		ObjectsActivator act;
		xmlNode* cnode = conf->getNode("Skel");
		if( cnode == NULL )
		{
			dlog[Debug::CRIT] << "(Skel): not found <Skel> in conffile" << endl;
			return 1;
		}
		
		Skel o("Skel",cnode);
		act.addObject( static_cast<UniSetObject*>(&o) );

		SystemMessage sm(SystemMessage::StartUp); 
		act.broadcast( sm.transport_msg() );

		unideb(Debug::ANY) << "\n\n\n";
		unideb[Debug::ANY] << "(Skel::main): -------------- Skel START -------------------------\n\n";
		dlog(Debug::ANY) << "\n\n\n";
		dlog[Debug::ANY] << "(Skel::main): -------------- Skel START -------------------------\n\n";
		act.run(false);
	}
	catch(SystemError& err)
	{
		cerr << "(Skel::main): " << err << endl;
	}
	catch(Exception& ex)
	{
		cerr << "(Skel::main): " << ex << endl;
	}
	catch(...)
	{
		cerr << "(Skel::main): catch(...)" << endl;
	}

	return 0;
}
// -----------------------------------------------------------------------------
