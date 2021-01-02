#include <sstream>
#include "RRDServer.h"
#include "Configuration.h"
#include "Debug.h"
#include "UniSetActivator.h"
#include "Extensions.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
using namespace uniset::extensions;
// -----------------------------------------------------------------------------
int main( int argc, const char** argv )
{
    //  std::ios::sync_with_stdio(false);

    if( argc > 1 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) )
    {
        cout << "--smemory-id objectName  - SharedMemory objectID. Default: autodetect" << endl;
        cout << "--confile filename       - configuration file. Default: configure.xml" << endl;
        cout << "--rrdstorage-logfile filename    - logfilename. Default: rrdstorage.log" << endl;
        cout << endl;
        RRDServer::help_print(argc, argv);
        return 0;
    }

    try
    {
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

        auto db = RRDServer::init_rrdstorage(argc, argv, shmID);

        if( !db )
        {
            dcrit << "(rrdstorage): init не прошёл..." << endl;
            return 1;
        }

        auto act = UniSetActivator::Instance();
        act->add(db);

        SystemMessage sm(SystemMessage::StartUp);
        act->broadcast( sm.transport_msg() );

        ulogany << "\n\n\n";
        ulogany << "(main): -------------- RRDServer START -------------------------\n\n";
        dlogany << "\n\n\n";
        dlogany << "(main): -------------- RRDServer START -------------------------\n\n";
        act->run(false);
        return 0;
    }
    catch( uniset::Exception& ex )
    {
        dcrit << "(rrdstorage): " << ex << std::endl;
    }
    catch(...)
    {
        dcrit << "(rrdstorage): catch ..." << std::endl;
    }

    return 1;
}
