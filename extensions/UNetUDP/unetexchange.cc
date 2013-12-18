#include <sstream>
#include <sys/wait.h>
#include "UniSetActivator.h"
#include "Extensions.h"
#include "UNetExchange.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// -----------------------------------------------------------------------------
int main( int argc, const char** argv )
{
    try
    {
        if( argc>1 && (!strcmp(argv[1],"--help") || !strcmp(argv[1],"-h")) )
        {
            cout << "--smemory-id objectName  - SharedMemory objectID. Default: read from <SharedMemory>" << endl;
            cout << "--confile filename       - configuration file. Default: configure.xml" << endl;
            cout << "--unet-logfile filename    - logfilename. Default: udpexchange.log" << endl;
            cout << endl;
            UNetExchange::help_print(argc,argv);
            return 0;
        }

        uniset_init(argc,argv);

        string logfilename(conf->getArgParam("--unet-logfile"));
        if( logfilename.empty() )
            logfilename = "udpexchange.log";


        std::ostringstream logname;
        string dir(conf->getLogDir());
        logname << dir << logfilename;
        ulog.logFile( logname.str() );
        UniSetExtensions::dlog.logFile( logname.str() );
        conf->initDebug(UniSetExtensions::dlog,"dlog");

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

        UNetExchange* unet = UNetExchange::init_unetexchange(argc,argv,shmID);
        if( !unet )
        {
            dlog.crit() << "(unetexchange): init failed.." << endl;
            return 1;
        }

        UniSetActivator act;
        act.addObject(static_cast<class UniSetObject*>(unet));

        SystemMessage sm(SystemMessage::StartUp);
        act.broadcast( sm.transport_msg() );

        ulog << "\n\n\n";
        ulog << "(main): -------------- UDPRecevier START -------------------------\n\n";
        dlog << "\n\n\n";
        dlog << "(main): -------------- UDPReceiver START -------------------------\n\n";

        act.run(false);
        while( waitpid(-1, 0, 0) > 0 );
    }
    catch( Exception& ex )
    {
        dlog.crit() << "(unetexchange): " << ex << std::endl;
    }
    catch(...)
    {
        dlog.crit() << "(unetexchange): catch ..." << std::endl;
    }

    while( waitpid(-1, 0, 0) > 0 );
    return 0;
}
