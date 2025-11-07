#include <sstream>
#include "MBTCPMaster.h"
#include "Configuration.h"
#include "Debug.h"
#include "UniSetActivator.h"
#include "Extensions.h"
#include "RunLock.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
using namespace uniset::extensions;
// -----------------------------------------------------------------------------
int main( int argc, const char** argv )
{
    //std::ios::sync_with_stdio(false);

    if( argc > 1 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) )
    {
        cout << endl;
        cout << "Usage: uniset2-mbtcpmaster args1 args2" << endl;
        cout << endl;
        cout << "--smemory-id objectName  - SharedMemory objectID. Default: autodetect" << endl;
        cout << "--confile filename       - configuration file. Default: configure.xml" << endl;
        cout << endl;
        MBTCPMaster::help_print(argc, argv);
        cout << endl;
        cout << " Global options:" << endl;
        cout << uniset::Configuration::help() << endl;
        return 0;
    }

    std::shared_ptr<RunLock> rlock = nullptr;
    try
    {
        int n = uniset::findArgParam("--run-lock",argc, argv);
        if( n != -1 )
        {
            if( n >= argc )
            {
                cerr << "Unknown lock file. Use --run-lock filename" << endl;
                return 1;
            }

            rlock = make_shared<RunLock>(argv[n+1]);
            if( rlock->isLocked() )
            {
                cerr << "ERROR: process is already running.. Lockfile: " << argv[n+1] << endl;
                return 1;
            }

            cout << "Run with lockfile: " << string(argv[n+1]) << endl;
            rlock->lock();
        }

        auto conf = uniset_init( argc, argv );

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

        auto mb = MBTCPMaster::init_mbmaster(argc, argv, shmID);

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
    catch( const uniset::Exception& ex )
    {
        cerr << "(mbtcpmaster): " << ex << std::endl;
    }
    catch(...)
    {
        std::exception_ptr p = std::current_exception();
        cerr << "(mbtcpmaster): catch.." << (p ? p.__cxa_exception_type()->name() : "null") << std::endl;
    }

    if( rlock )
        rlock->unlock();

    return 1;
}
