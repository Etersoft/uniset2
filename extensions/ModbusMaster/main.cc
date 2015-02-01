#include <sstream>
#include "MBTCPMaster.h"
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
    std::ios::sync_with_stdio(false);
    if( argc>1 && (!strcmp(argv[1],"--help") || !strcmp(argv[1],"-h")) )
    {
        cout << "--smemory-id objectName  - SharedMemory objectID. Default: autodetect" << endl;
        cout << "--confile filename       - configuration file. Default: configure.xml" << endl;
        cout << "--mbtcp-logfile filename    - logfilename. Default: mbtcpmaster.log" << endl;
        cout << endl;
        MBTCPMaster::help_print(argc, argv);
        return 0;
    }

    try
    {
        auto conf = uniset_init( argc, argv );

        string logfilename(conf->getArgParam("--mbtcp-logfile"));
        if( logfilename.empty() )
            logfilename = "mbtcpmaster.log";

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

        auto mb = MBTCPMaster::init_mbmaster(argc,argv,shmID);
        if( !mb )
        {
            dcrit << "(mbmaster): init MBTCPMaster failed." << endl;
            return 1;
        }

        auto act = UniSetActivator::Instance();
        act->add(mb);

        SystemMessage sm(SystemMessage::StartUp);
        act->broadcast( sm.transport_msg() );

        ulogany << "\n\n\n";
        ulogany << "(main): -------------- MBTCP Exchange START -------------------------\n\n";
        dlogany << "\n\n\n";
        dlogany << "(main): -------------- MBTCP Exchange START -------------------------\n\n";
        act->run(false);
        return 0;
    }
    catch( const Exception& ex )
    {
        dcrit << "(mbtcpmaster): " << ex << std::endl;
    }
    catch(...)
    {
        std::exception_ptr p = std::current_exception();
        std::clog <<(p ? p.__cxa_exception_type()->name() : "null") << std::endl;

        dcrit << "(mbtcpmaster): catch ..." << std::endl;
    }

    return 1;
}
