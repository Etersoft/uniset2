#include <sstream>
#include "SMDBServer.h"
#include "Configuration.h"
#include "Debug.h"
#include "UniSetActivator.h"
#include "Extensions.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// -----------------------------------------------------------------------------
int main( int argc, const char** argv )
{
    if( argc>1 && (!strcmp(argv[1],"--help") || !strcmp(argv[1],"-h")) )
    {
        cout << "--smemory-id objectName  - SharedMemory objectID. Default: autodetect" << endl;
        cout << "--confile filename       - configuration file. Default: configure.xml" << endl;
        cout << "--smdbserver-logfile filename    - logfilename. Default: smdbserver.log" << endl;
        cout << endl;
        SMDBServer::help_print(argc, argv);
        return 0;
    }

    try
    {
        auto conf = uniset_init( argc, argv, confile );

        string logfilename(conf->getArgParam("--smdbserver-logfile"));
        if( logfilename.empty() )
            logfilename = "smdbserver.log";
    
        std::ostringstream logname;
        string dir(conf->getLogDir());
        logname << dir << logfilename;
        ulog.logFile( logname.str() );
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

        auto db = SMDBServer::init_smdbserver(argc,argv,shmID);
        if( !db )
        {
            dcrit << "(smdbserver): init не прошёл..." << endl;
            return 1;
        }

        auto act = UniSetActivator::Instance();
        act->add(db);

        SystemMessage sm(SystemMessage::StartUp); 
        act->broadcast( sm.transport_msg() );

        ulog << "\n\n\n";
        ulog << "(main): -------------- SMDBServer START -------------------------\n\n";
        dlog << "\n\n\n";
        dlog << "(main): -------------- SMDBServer START -------------------------\n\n";
        act->run(false);
        return 0;
    }
    catch( const Exception& ex )
    {
        dcrit << "(smdbserver): " << ex << std::endl;
    }
    catch(...)
    {
        dcrit << "(smdbserver): catch ..." << std::endl;
    }

    return 1;
}
