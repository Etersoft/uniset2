#include "Configuration.h"
#include "Extensions.h"
#include "OPCUAServer.h"
#include "Configuration.h"
#include "UniSetActivator.h"
#include "RunLock.h"
// --------------------------------------------------------------------------
using namespace uniset;
using namespace std;
// --------------------------------------------------------------------------
int main(int argc, char** argv)
{
    //  std::ios::sync_with_stdio(false);
    std::shared_ptr<RunLock> rlock = nullptr;
    try
    {
        if( argc > 1 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) )
        {
            cout << "--confile filename - configuration file. Default: configure.xml" << endl;
            OPCUAServer::help_print();
            return 0;
        }

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

        auto conf = uniset_init(argc, argv);

        ObjectId shmID = DefaultObjectId;
        string sID = conf->getArgParam("--smemory-id");

        if( !sID.empty() )
            shmID = conf->getControllerID(sID);
        else
             shmID = uniset::extensions::getSharedMemoryID();

        if( shmID == DefaultObjectId )
        {
            cerr << sID << "? SharedMemoryID not found in " << conf->getControllersSection() << " section" << endl;
            return 1;
        }

        auto srv = OPCUAServer::init_opcua_server(argc, argv, shmID, nullptr, "opcua");

        if( !srv )
            return 1;

        auto act = UniSetActivator::Instance();
        act->add(srv);

        SystemMessage sm(SystemMessage::StartUp);
        act->broadcast( sm.transport_msg() );
        act->run(false);
        return 0;
    }
    catch( const std::exception& ex )
    {
        cerr << "(OPCUAServer::main): " << ex.what() << endl;
    }
    catch(...)
    {
        cerr << "(OPCUAServer::main): catch ..." << endl;
    }

    if( rlock )
        rlock->unlock();

    return 1;
}
