// --------------------------------------------------------------------------
#include <string>
#include <sys/wait.h>
#include <Debug.h>
#include <ObjectsActivator.h>
#include <ThreadCreator.h>
#include "Extensions.h"
#include "RTUExchange.h"
#include "MBSlave.h"
#include "MBTCPMaster.h"
#include "SharedMemory.h"
#include "IOControl.h"
//#include "UniExchange.h"
#include "UNetExchange.h"
#include "Configuration.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// --------------------------------------------------------------------------
static void help_print( int argc, const char* argv[] );
// --------------------------------------------------------------------------
int main( int argc, const char **argv )
{
	if( argc>1 && strcmp(argv[1],"--help")==0 )
	{
		help_print( argc, argv);
		return 0;
	}

	bool add_io 	= findArgParam("--add-io",argc,argv) != -1;
	bool add_rtu 	= findArgParam("--add-rtu",argc,argv) != -1;
	bool add_rtu2 	= findArgParam("--add-rtu2",argc,argv) != -1;
	bool add_mbslave = findArgParam("--add-mbslave",argc,argv) != -1;
	bool add_io2 = findArgParam("--add-io2",argc,argv) != -1;
	bool add_mbmaster = findArgParam("--add-mbmaster",argc,argv) != -1;
	bool add_mbmaster2 = findArgParam("--add-mbmaster2",argc,argv) != -1;
	bool add_mbmaster3 = findArgParam("--add-mbmaster3",argc,argv) != -1;
	//bool add_uniexchange = findArgParam("--add-unet",argc,argv) != -1;
	//bool add_uniexchange2 = findArgParam("--add-unet2",argc,argv) != -1;
	bool add_unetudp = findArgParam("--add-unetudp",argc,argv) != -1;

	bool add_mbtcp_ses1 = findArgParam("--add-mbtcp-ses1",argc,argv) != -1;
	bool add_mbtcp_ses2 = findArgParam("--add-mbtcp-ses2",argc,argv) != -1;
	bool add_mbtcp_ses3 = findArgParam("--add-mbtcp-ses3",argc,argv) != -1;
	bool add_mbtcp_ses4 = findArgParam("--add-mbtcp-ses4",argc,argv) != -1;

	try
	{
		string confile = UniSetTypes::getArgParam( "--confile", argc, argv, "configure.xml" );
		conf = new Configuration(argc, argv, confile);

		string logfilename = conf->getArgParam("--logfile", "smemory2.log");
		string logname( conf->getLogDir() + logfilename );
		UniSetExtensions::dlog.logFile( logname );
		unideb.logFile( logname );
		conf->initDebug(UniSetExtensions::dlog,"dlog");

		ObjectsActivator act;
		// ------------ SharedMemory ----------------
		SharedMemory* shm = SharedMemory::init_smemory(argc,argv);
		if( shm == NULL )
			return 1;

		act.addManager(static_cast<class ObjectsManager*>(shm));
		// ------------ IOControl ----------------
		ThreadCreator<IOControl>* io_thr = NULL;
		if( add_io )
		{
			IOControl* ic = IOControl::init_iocontrol(argc,argv,shm->getId(),shm);
			if( ic == NULL )
				return 1;
			io_thr = new ThreadCreator<IOControl>(ic, &IOControl::execute);
			if( io_thr == NULL )
				return 1;

			act.addObject(static_cast<class UniSetObject*>(ic));
		}
		// ------------- IOControl2 --------------
		ThreadCreator<IOControl>* io2_thr = NULL;
		if( add_io2 )
		{
			IOControl* ic2 = IOControl::init_iocontrol(argc,argv,shm->getId(),shm,"io2");
			if( ic2 == NULL )
				return 1;
			io2_thr = new ThreadCreator<IOControl>(ic2, &IOControl::execute);
			if( io2_thr == NULL )
				return 1;

			act.addObject(static_cast<class UniSetObject*>(ic2));
		}
		// ------------- RTU Exchange --------------
		if( add_rtu )
		{
			RTUExchange* rtu = RTUExchange::init_rtuexchange(argc,argv,shm->getId(),shm,"rs");
			if( rtu == NULL )
				return 1;
			act.addObject(static_cast<class UniSetObject*>(rtu));
		}
		// ------------- RTU2 Exchange --------------
		if( add_rtu2 )
		{
			RTUExchange* rtu2 = RTUExchange::init_rtuexchange(argc,argv,shm->getId(),shm,"rs2");
			if( rtu2 == NULL )
				return 1;
			act.addObject(static_cast<class UniSetObject*>(rtu2));
		}
		// ------------- MBSlave --------------
		if( add_mbslave )
		{
			MBSlave* mbs = MBSlave::init_mbslave(argc,argv,shm->getId(),shm);
			if( mbs == NULL )
				return 1;

			act.addObject(static_cast<class UniSetObject*>(mbs));
		}

		// ------------- MBTCPMaster 1 --------------
		if( add_mbmaster )
		{
			MBTCPMaster* mbm1 = MBTCPMaster::init_mbmaster(argc,argv,shm->getId(),shm,"mbtcp");
			if( mbm1 == NULL )
				return 1;

			act.addObject(static_cast<class UniSetObject*>(mbm1));
		}
		// ------------- MBTCPMaster 2 --------------
		if( add_mbmaster2 )
		{
			MBTCPMaster* mbm2 = MBTCPMaster::init_mbmaster(argc,argv,shm->getId(),shm,"mbtcp2");
			if( mbm2 == NULL )
				return 1;

			act.addObject(static_cast<class UniSetObject*>(mbm2));
		}
		// ------------- MBTCPMaster 3 --------------
		if( add_mbmaster3 )
		{
			MBTCPMaster* mbm3 = MBTCPMaster::init_mbmaster(argc,argv,shm->getId(),shm,"mbtcp3");
			if( mbm3 == NULL )
				return 1;

			act.addObject(static_cast<class UniSetObject*>(mbm3));
		}
#if 0
 UniExchnage deprecated		
		// ------------- UniExchange 1 --------------
		ThreadCreator<UniExchange>* unet_thr = NULL;
		if( add_uniexchange )
		{
			UniExchange* unet = UniExchange::init_exchange(argc,argv,shm->getId(),shm,"unet");
			if( unet == NULL )
				return 1;

			unet_thr = new ThreadCreator<UniExchange>(unet, &UniExchange::execute);
			if( unet_thr == NULL )
				return 1;

			act.addManager(static_cast<class ObjectsManager*>(unet));
		}
		// ------------- UniExchange 2 --------------
		ThreadCreator<UniExchange>* unet2_thr = NULL;
		if( add_uniexchange2 )
		{
			UniExchange* unet2 = UniExchange::init_exchange(argc,argv,shm->getId(),shm,"unet2");
			if( unet2 == NULL )
				return 1;

			unet2_thr = new ThreadCreator<UniExchange>(unet2, &UniExchange::execute);
			if( unet2_thr == NULL )
				return 1;

			act.addManager(static_cast<class ObjectsManager*>(unet2));
		}
#endif
		// ------------- MBTCPMaster SES1 --------------
		if( add_mbtcp_ses1 )
		{
			MBTCPMaster* mbm_ses1 = MBTCPMaster::init_mbmaster(argc,argv,shm->getId(),shm,"mbtcp-ses1");
			if( mbm_ses1 == NULL )
				return 1;

			act.addObject(static_cast<class UniSetObject*>(mbm_ses1));
		}
		// ------------- MBTCPMaster SES2 --------------
		if( add_mbtcp_ses2 )
		{
			MBTCPMaster* mbm_ses2 = MBTCPMaster::init_mbmaster(argc,argv,shm->getId(),shm,"mbtcp-ses2");
			if( mbm_ses2 == NULL )
				return 1;

			act.addObject(static_cast<class UniSetObject*>(mbm_ses2));
		}
		// ------------- MBTCPMaster SES3 --------------
		if( add_mbtcp_ses3 )
		{
			MBTCPMaster* mbm_ses3 = MBTCPMaster::init_mbmaster(argc,argv,shm->getId(),shm,"mbtcp-ses3");
			if( mbm_ses3 == NULL )
				return 1;

			act.addObject(static_cast<class UniSetObject*>(mbm_ses3));
		}
		// ------------- MBTCPMaster SES4 --------------
		if( add_mbtcp_ses4 )
		{
			MBTCPMaster* mbm_ses4 = MBTCPMaster::init_mbmaster(argc,argv,shm->getId(),shm,"mbtcp-ses4");
			if( mbm_ses4 == NULL )
				return 1;

			act.addObject(static_cast<class UniSetObject*>(mbm_ses4));
		}

		// ------------- UNetUDP --------------
		if( add_unetudp )
		{
			UNetExchange* unet = UNetExchange::init_unetexchange(argc,argv,shm->getId(),shm);
			if( unet == NULL )
				return 1;

			act.addObject(static_cast<class UniSetObject*>(unet));
		}
		// ---------------------------------------
		SystemMessage sm(SystemMessage::StartUp);
		act.broadcast( sm.transport_msg() );

		if( io_thr )
			io_thr->start();

		if( io2_thr )
			io2_thr->start();

/*
		if( unet_thr )
			unet_thr->start();

		if( unet2_thr )
			unet2_thr->start();
*/
		act.run(false);
		while( waitpid(-1, 0, 0) > 0 );
		
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

	while( waitpid(-1, 0, 0) > 0 );
	return 1;
}
// --------------------------------------------------------------------------
void help_print( int argc, const char* argv[])
{
	cout << "--skip-io      - skip IOControl" << endl;
	cout << "--add-rtu      - Start RTUExchange" << endl;
	cout << "--add-rtu2     - Start RTUExchange2" << endl;
	cout << "--add-mbslave  - Start ModbusSlave" << endl;
	cout << "--add-io2      - Start IOControl2" << endl;
	cout << "--add-mbmaster   - Start MBTCPMaster" << endl;
	cout << "--add-mbmaster2  - Start MBTCPMaster2" << endl;
	cout << "--add-mbmaster3  - Start MBTCPMaster3" << endl;
	cout << "--add-unet       - Start UniNetwork" << endl;
	cout << "--add-mbtcp-ses1 - Exchange for FastwelIO (gdg1)" << endl;
	cout << "--add-mbtcp-ses2 - Exchange for FastwelIO (gdg2)" << endl;
	cout << "--add-mbtcp-ses3 - Exchange for FastwelIO (gdg3)" << endl;
	cout << "--add-mbtcp-ses4 - Exchange for FastwelIO (gdg4)" << endl;
	cout << "--add-uniset-unet2 - Exchange for UNet2" << endl;

	cout << "\n   ###### SM options ###### \n" << endl;
	SharedMemory::help_print(argc,argv);

	cout << "\n   ###### IO options ###### \n" << endl;
	IOControl::help_print(argc,argv);

	cout << "\n   ###### RTU options ###### \n" << endl;
	RTUExchange::help_print(argc,argv);

	cout << "\n   ###### ModbusSlave options ###### \n" << endl;
	MBSlave::help_print(argc,argv);

	cout << "\n   ###### ModbusTCP Master options ###### \n" << endl;
	MBTCPMaster::help_print(argc,argv);

//	cout << "\n   ###### UniExchange options ###### \n" << endl;
//	UniExchange::help_print(argc,argv);

	cout << "\n   ###### UNetExchange options ###### \n" << endl;
	UNetExchange::help_print(argc,argv);

//	CanExchange::help_print(argc,argv); //не реализована!!!

	cout << "\n   ###### Common options ###### \n" << endl;
	cout << "--confile			- " << endl;
	cout << "--logfile			- " << endl;
	cout << "--myEDS			- " << endl;
	cout << "--slaveEDS			- " << endl;
	cout << "--nodeID			- " << endl;
	cout << "--initPause		- " << endl;
	cout << "--baudrate			- " << endl;
	cout << "--maxHBValue		- " << endl;
	cout << "--nodeName			- " << endl;
	cout << "--sendTime			- " << endl;
	cout << "--lifeTime			- " << endl;
	cout << "--workTime			- " << endl;
	cout << "--receiveTime		- " << endl;
}
