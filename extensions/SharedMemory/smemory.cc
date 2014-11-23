#include <string>
#include "Debug.h"
#include "UniSetActivator.h"
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
        cout << "--confile    - Использовать указанный конф. файл. По умолчанию configure.xml" << endl;
        SharedMemory::help_print(argc, argv);
        return 0;
    }

    try
    {
        auto conf = uniset_init(argc, argv);

        conf->initDebug(dlog,"dlog");
        string logfilename = conf->getArgParam("--logfile", "smemory.log");
        string logname( conf->getLogDir() + logfilename );
        ulog.logFile( logname );
        dlog.logFile( logname );

        SharedMemory* shm = SharedMemory::init_smemory(argc, argv);
        if( !shm )
            return 1;

        UniSetActivatorPtr act = UniSetActivator::Instance();

        act->addObject(static_cast<class UniSetObject*>(shm));
        SystemMessage sm(SystemMessage::StartUp);
        act->broadcast( sm.transport_msg() );
        act->run(false);
        return 0;
    }
    catch( SystemError& err )
    {
        dlog.crit() << "(smemory): " << err << endl;
    }
    catch( Exception& ex )
    {
        dlog.crit() << "(smemory): " << ex << endl;
    }
    catch( std::exception& e )
    {
        dlog.crit() << "(smemory): " << e.what() << endl;
    }
    catch(...)
    {
        dlog.crit() << "(smemory): catch(...)" << endl;
    }

    return 1;
}
