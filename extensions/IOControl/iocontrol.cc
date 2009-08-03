// $Id: iocontrol.cc,v 1.3 2009/01/17 14:34:40 vpashka Exp $
// --------------------------------------------------------------------------
#include <string>
#include "Debug.h"
#include "ObjectsActivator.h"
#include "Configuration.h"
#include "IOControl.h"
#include "Extensions.h"
// --------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
using namespace UniSetExtensions;
// --------------------------------------------------------------------------
int main(int argc, const char **argv)
{   
	if( argc>1 && strcmp(argv[1],"--help")==0 )
	{
		cout << "--io-confile	- Использовать указанный конф. файл. По умолчанию configure.xml" << endl;
		cout << "--io-logfile fname	- выводить логи в файл fname. По умолчанию iocontrol.log" << endl;
		IOControl::help_print(argc,argv);
		return 0;
	}

	try
	{
		string confile = UniSetTypes::getArgParam( "--confile", argc, argv, "configure.xml" );
		conf = new Configuration(argc, argv, confile);

		conf->initDebug(dlog,"dlog");
		string logfilename = conf->getArgParam("--io-logfile","iocontrol.log");
		string logname( conf->getLogDir() + logfilename );
		dlog.logFile( logname.c_str() );
		unideb.logFile( logname.c_str() );

		ObjectId shmID = DefaultObjectId;
		string sID = conf->getArgParam("--smemory-id");
		if( !sID.empty() )
			shmID = conf->getControllerID(sID);
		else
			shmID = getSharedMemoryID();

		if( shmID == DefaultObjectId )
		{
			cerr << sID << "? SharedMemoryID not found in " 
					<< conf->getControllersSection() << " section" << endl;
			return 1;
		}

		
		IOControl* ic = IOControl::init_iocontrol(argc,argv,shmID);
		if( !ic )
		{
			dlog[Debug::CRIT] << "(iocontrol): init не прошёл..." << endl;
			return 1;
		}

		ObjectsActivator act;
		act.addObject(static_cast<class UniSetObject*>(ic));

		SystemMessage sm(SystemMessage::StartUp); 
		act.broadcast( sm.transport_msg() );

		unideb(Debug::ANY) << "\n\n\n";
		unideb[Debug::ANY] << "(main): -------------- IOControl START -------------------------\n\n";
		dlog(Debug::ANY) << "\n\n\n";
		dlog[Debug::ANY] << "(main): -------------- IOControl START -------------------------\n\n";
		act.run(true);
		msleep(500);
		ic->execute();
	}
	catch(SystemError& err)
	{
		dlog[Debug::CRIT] << "(iocontrol): " << err << endl;
	}
	catch(Exception& ex)
	{
		dlog[Debug::CRIT] << "(iocontrol): " << ex << endl;
	}
	catch(...)
	{
		dlog[Debug::CRIT] << "(iocontrol): catch(...)" << endl;
	}

	return 0;
}
