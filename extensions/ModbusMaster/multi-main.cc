#include <sstream>
#include "MBTCPMultiMaster.h"
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
        cout << "--smemory-id objectName  - SharedMemory objectID. Default: get from configure..." << endl;
        cout << "--confile filename       - configuration file. Default: configure.xml" << endl;
        cout << "--mbtcp-logfile filename    - logfilename. Default: mbtcpmultimaster.log" << endl;
        cout << endl;
        MBTCPMultiMaster::help_print(argc, argv);
        return 0;
    }

    try
    {
        auto conf = uniset_init( argc, argv );

        string logfilename(conf->getArgParam("--mbtcp-logfile"));
        if( logfilename.empty() )
            logfilename = "mbtcpmultimaster.log";

        conf->initDebug(dlog(),"dlog");

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

        auto mb = MBTCPMultiMaster::init_mbmaster(argc,argv,shmID);
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
        ulogany << "(main): -------------- MBTCPMulti Exchange START -------------------------\n\n";
        dlogany << "\n\n\n";
        dlogany << "(main): -------------- MBTCPMulti Exchange START -------------------------\n\n";

        act->run(false);
        return 0;
    }
    catch( Exception& ex )
    {
        dcrit << "(mbtcpmultimaster): " << ex << std::endl;
    }
    catch(...)
    {
        dcrit << "(mbtcpmultimaster): catch ..." << std::endl;
    }

    return 1;
}
