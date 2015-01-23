// --------------------------------------------------------------------------
#include <sstream>
#include <string>
#include <cc++/socket.h>
#include "MBSlave.h"
#include "Configuration.h"
#include "Debug.h"
#include "UniSetActivator.h"
#include "Extensions.h"

// --------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace UniSetExtensions;
using namespace std;
// --------------------------------------------------------------------------
int main(int argc, const char **argv)
{
    std::ios::sync_with_stdio(false);
    if( argc>1 && (!strcmp(argv[1],"--help") || !strcmp(argv[1],"-h")) )
    {
        cout << "--smemory-id objectName  - SharedMemory objectID. Default: autodetect" << endl;
        cout << "--confile filename       - configuration file. Default: configure.xml" << endl;
        cout << "--mbs-logfile filename    - logfilename. Default: mbslave.log" << endl;
        cout << endl;
        MBSlave::help_print(argc,argv);
        return 0;
    }

    try
    {
        auto conf = uniset_init(argc, argv);

        string logfilename(conf->getArgParam("--mbs-logfile"));
        if( logfilename.empty() )
            logfilename = "mbslave.log";

        std::ostringstream logname;
        string dir(conf->getLogDir());
        logname << dir << logfilename;
        ulog()->logFile( logname.str() );
        dlog()->logFile( logname.str() );

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

        auto s = MBSlave::init_mbslave(argc,argv,shmID);
        if( !s )
        {
            dcrit << "(mbslave): init не прошёл..." << endl;
            return 1;
        }

        auto act = UniSetActivator::Instance();
        act->add(s);
        SystemMessage sm(SystemMessage::StartUp);
        act->broadcast( sm.transport_msg() );

        ulogany << "\n\n\n";
        ulogany << "(main): -------------- MBSlave START -------------------------\n\n";
        dlogany << "\n\n\n";
        dlogany << "(main): -------------- MBSlave START -------------------------\n\n";

        act->run(false);
//        on_sigchild(SIGTERM);
        return 0;
    }
    catch( SystemError& err )
    {
        dcrit << "(mbslave): " << err << endl;
    }
    catch( Exception& ex )
    {
        dcrit << "(mbslave): " << ex << endl;
    }
    catch( std::exception& e )
    {
        dcrit << "(mbslave): " << e.what() << endl;
    }
    catch(...)
    {
        dcrit << "(mbslave): catch(...)" << endl;
    }

  //  on_sigchild(SIGTERM);
    return 1;
}
// --------------------------------------------------------------------------
