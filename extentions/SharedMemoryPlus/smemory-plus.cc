// --------------------------------------------------------------------------
//! \version $Id: smemory-plus.cc,v 1.1 2009/02/10 20:38:27 vpashka Exp $
// --------------------------------------------------------------------------
#include <string>
#include "Debug.h"
#include "ObjectsActivator.h"
#include "SharedMemory.h"
#include "Extentions.h"
#include "IOControl.h"
#include "RTUExchange.h"
#include "MBSlave.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtentions;
// --------------------------------------------------------------------------
static void help_print( int argc, char* argv[] );
// --------------------------------------------------------------------------
int main( int argc, char **argv )
{   
	if( argc>1 && strcmp(argv[1],"--help")==0 )
	{
		help_print( argc, argv);
		return 0;
	}

	bool add_io 		= findArgParam("--add-io",argc,argv)!=-1 ? true : false;
	bool add_rtu 		= findArgParam("--add-rtu",argc,argv)!=-1 ? true : false;
	bool add_mbslave 	= findArgParam("--add-mbslave",argc,argv)!=-1 ? true : false;

	try
	{
		string confile = UniSetTypes::getArgParam( "--confile", argc, argv, "configure.xml" );
		conf = new Configuration(argc, argv, confile);

		string logfilename = conf->getArgParam("--logfile", "smemory-plus.log");
		string logname( conf->getLogDir() + logfilename );
		unideb.logFile( logname.c_str() );

		ObjectsActivator act;

		// ------------ SharedMemory ----------------
		SharedMemory* shm = SharedMemory::init_smemory(argc,argv);
		if( !shm )
			return 1;

		act.addManager(static_cast<class ObjectsManager*>(shm));

		// ------------ IOControl ----------------
		ThreadCreator<IOControl>* io_thr = 0;
		if( add_io )
		{
			IOControl* ic = IOControl::init_iocontrol(argc,argv,shm->getId(),shm);
			if( !ic )
				return 1;
			io_thr = new ThreadCreator<IOControl>(ic, &IOControl::execute);
			if( !io_thr )
				return 1;

			act.addObject(static_cast<class UniSetObject*>(ic));
		}

		// ------------- RTU Exchange --------------
		if( add_rtu )
		{
			RTUExchange* rtu = RTUExchange::init_rtuexchange(argc,argv,shm->getId(),shm);
			if( !rtu )
				return 1;
			act.addObject(static_cast<class UniSetObject*>(rtu));
		}

		// ------------- MBSLavee --------------
		if( add_mbslave )
		{
			MBSlave* mbs = MBSlave::init_mbslave(argc,argv,shm->getId(),shm);
			if( !mbs )
				return 1;
			
			act.addObject(static_cast<class UniSetObject*>(mbs));
		}

		// ----------------------------------------
		SystemMessage sm(SystemMessage::StartUp); 
		act.broadcast( sm.transport_msg() );	
		act.run(true);

		msleep(1000);
		if( io_thr )
			io_thr->start();

		pause();
		return 0;
	}
	catch(Exception& ex)
	{
		unideb[Debug::CRIT] << "(smemory-plus): " << ex << endl;
	}
    catch( CORBA::SystemException& ex )
    {
    	unideb[Debug::CRIT] << "(smemory-plus): " << ex.NP_minorString() << endl;
    }
	catch(...)
	{
		unideb[Debug::CRIT] << "(smemory-plus): catch(...)" << endl;
	}
	
	return 1;
}
// --------------------------------------------------------------------------
void help_print( int argc, char* argv[])
{
	cout << "--add-io       - Start IOControl" << endl;
	cout << "--add-rtu      - Start RTUExchange" << endl;
	cout << "--add-mbslave  - Start ModbusSlave" << endl;

	cout << "\n   ###### SM options ###### \n" << endl;
	SharedMemory::help_print(argc,argv);
	
	cout << "\n   ###### IO options ###### \n" << endl;
	IOControl::help_print(argc,argv);
	
	cout << "\n   ###### RTU options ###### \n" << endl;
	RTUExchange::help_print(argc,argv);

	cout << "\n   ###### ModbusSlave options ###### \n" << endl;
	MBSlave::help_print(argc,argv);
}
