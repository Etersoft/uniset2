#include <sys/wait.h>
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
        string confile = UniSetTypes::getArgParam( "--confile", argc, argv, "configure.xml" );
        conf = new Configuration(argc, argv, confile);

        conf->initDebug(dlog,"dlog");
        string logfilename = conf->getArgParam("--logfile", "smemory.log");
        string logname( conf->getLogDir() + logfilename );
        ulog.logFile( logname );
        dlog.logFile( logname );

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

        UniExchange* shm = UniExchange::init_exchange(argc, argv, shmID);
        if( !shm )
            return 1;

        UniSetActivator act;

        act.addObject(static_cast<class UniSetObject*>(shm));
        SystemMessage sm(SystemMessage::StartUp);
        act.broadcast( sm.transport_msg() );
        act.run(true);
        shm->execute();
        while( waitpid(-1, 0, 0) > 0 );
        return 0;
    }
    catch(SystemError& err)
    {
        ulog.crit() << "(uninetwork): " << err << endl;
    }
    catch(Exception& ex)
    {
        ulog.crit() << "(uninetwork): " << ex << endl;
    }
    catch(...)
    {
        ulog.crit() << "(uninetwork): catch(...)" << endl;
    }

    while( waitpid(-1, 0, 0) > 0 );
    return 1;
}
