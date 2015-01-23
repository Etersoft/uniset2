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
    std::ios::sync_with_stdio(false);

    if( argc>1 && ( strcmp(argv[1],"--help")==0 || strcmp(argv[1],"-h")==0 ) )
    {
        cout << "--confile    - Использовать указанный конф. файл. По умолчанию configure.xml" << endl;
        SharedMemory::help_print(argc, argv);
        return 0;
    }
    
    try
    {
        auto conf = uniset_init(argc, argv);

        conf->initDebug(dlog(),"dlog");
        string logfilename = conf->getArgParam("--logfile", "smemory.log");
        string logname( conf->getLogDir() + logfilename );
        ulog()->logFile( logname );
        dlog()->logFile( logname );

        auto shm = SharedMemory::init_smemory(argc, argv);
        if( !shm )
            return 1;

        auto act = UniSetActivator::Instance();

        act->add(shm);
        SystemMessage sm(SystemMessage::StartUp);
        act->broadcast( sm.transport_msg() );
        act->run(false);
        return 0;
    }
    catch( SystemError& err )
    {
        dcrit << "(smemory): " << err << endl;
    }
    catch( Exception& ex )
    {
        dcrit << "(smemory): " << ex << endl;
    }
    catch( std::exception& e )
    {
        dcrit << "(smemory): " << e.what() << endl;
    }
    catch(...)
    {
        dcrit << "(smemory): catch(...)" << endl;
    }

    return 1;
}
