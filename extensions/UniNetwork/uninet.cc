#include <string>
#include "Debug.h"
#include "UniSetActivator.h"
#include "UniExchange.h"
#include "Extensions.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// --------------------------------------------------------------------------
int main(int argc, const char **argv)
{   
    if( argc>1 && strcmp(argv[1],"--help")==0 )
    {
        cout << "--confile    - Использовать указанный конф. файл. По умолчанию configure.xml" << endl;
        UniExchange::help_print(argc, argv);
        return 0;
    }

    try
    {
        auto conf = uniset_init(argc, argv);

        string logfilename = conf->getArgParam("--logfile", "smemory.log");
        string logname( conf->getLogDir() + logfilename );
        ulog()->logFile( logname );
        dlog()->logFile( logname );

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

        auto uex = UniExchange::init_exchange(argc, argv, shmID);
        if( !uex )
            return 1;

        auto act = UniSetActivator::Instance();

        act->add(uex);
        SystemMessage sm(SystemMessage::StartUp);
        act->broadcast( sm.transport_msg() );
        act->run(true);
        uex->execute();
        on_sigchild(SIGTERM);
        return 0;
    }
    catch( const std::exception& ex )
    {
        dcrit << "(uninetwork): " << ex.what() << endl;
    }
    catch(...)
    {
        dcrit << "(uninetwork): catch(...)" << endl;
    }

    on_sigchild(SIGTERM);
    return 1;
}
