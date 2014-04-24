#include <string>
#include "Debug.h"
#include "ObjectsActivator.h"
#include "SharedMemory.h"
#include "Extensions.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// --------------------------------------------------------------------------
int main(int argc, const char **argv)
{   
	if( argc>1 && ( strcmp(argv[1],"--help")==0 || strcmp(argv[1],"-h")==0 ) )
	{
		cout << "--confile	- Использовать указанный конф. файл. По умолчанию configure.xml" << endl;
		SharedMemory::help_print(argc, argv);
		return 0;
	}

	try
	{
		string confile = UniSetTypes::getArgParam( "--confile", argc, argv, "configure.xml" );
		conf = new Configuration(argc, argv, confile);

		conf->initDebug(dlog,"dlog");
		string logfilename = conf->getArgParam("--logfile", "smemory.log");
		string logname( conf->getLogDir() + logfilename );
		unideb.logFile( logname );
		dlog.logFile( logname );

		SharedMemory* shm = SharedMemory::init_smemory(argc, argv);
		if( !shm )
			return 1;

		ObjectsActivator act;

		act.addObject(static_cast<class UniSetObject*>(shm));
		SystemMessage sm(SystemMessage::StartUp); 
		act.broadcast( sm.transport_msg() );
		act.run(false);

//		pause();	// пауза, чтобы дочерние потоки успели завершить работу
		return 0;
	}
	catch( SystemError& err )
	{
		dlog[Debug::CRIT] << "(smemory): " << err << endl;
	}
	catch( Exception& ex )
	{
		dlog[Debug::CRIT] << "(smemory): " << ex << endl;
	}
	catch( std::exception& e )
	{
		dlog[Debug::CRIT] << "(smemory): " << e.what() << endl;
	}
	catch(...)
	{
		dlog[Debug::CRIT] << "(smemory): catch(...)" << endl;
	}
	
	return 1;
}
