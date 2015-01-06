#include <sstream>
#include "UniSetActivator.h"
#include "Extensions.h"
#include "RTUExchange.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// -----------------------------------------------------------------------------
int main( int argc, char** argv )
{
    try
    {
        if( argc>1 && (!strcmp(argv[1],"--help") || !strcmp(argv[1],"-h")) )
        {
            cout << "--smemory-id objectName  - SharedMemory objectID. Default: read from <SharedMemory>" << endl;
            cout << "--confile filename       - configuration file. Default: configure.xml" << endl;
            cout << "--rs-logfile filename    - logfilename. Default: rtuexchange.log" << endl;
            cout << endl;
            RTUExchange::help_print(argc, argv);
            return 0;
        }

        auto conf = uniset_init( argc, argv );

        string logfilename(conf->getArgParam("--rs-logfile"));
        if( logfilename.empty() )
            logfilename = "rtuexchange.log";

        conf->initDebug(dlog,"dlog");

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

        auto rs = RTUExchange::init_rtuexchange(argc,argv,shmID,0,"rs");
        if( !rs )
        {
            dcrit << "(rtuexchange): init не прошёл..." << endl;
            return 1;
        }

        auto act = UniSetActivator::Instance();
        act->addObject(rs);

        SystemMessage sm(SystemMessage::StartUp);
        act->broadcast( sm.transport_msg() );

        ulog << "\n\n\n";
        ulog << "(main): -------------- RTU Exchange START -------------------------\n\n";
        dlog << "\n\n\n";
        dlog << "(main): -------------- RTU Exchange START -------------------------\n\n";

        act->run(false);

        on_sigchild(SIGTERM);
        return 0;
    }
    catch( Exception& ex )
    {
        dcrit << "(rtuexchange): " << ex << std::endl;
    }
    catch(...)
    {
        dcrit << "(rtuexchange): catch ..." << std::endl;
    }

    on_sigchild(SIGTERM);
    return 1;
}
