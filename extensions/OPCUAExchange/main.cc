#include <sstream>
#include "OPCUAExchange.h"
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
    //std::ios::sync_with_stdio(false);

    if( argc > 1 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) )
    {
        cout << endl;
        cout << "Usage: uniset2-opcuaclient args1 args2" << endl;
        cout << endl;
        cout << "--smemory-id objectName  - SharedMemory objectID. Default: autodetect" << endl;
        cout << "--confile filename       - configuration file. Default: configure.xml" << endl;
        cout << endl;
        OPCUAExchange::help_print(argc, argv);
        cout << endl;
        cout << " Global options:" << endl;
        cout << uniset::Configuration::help() << endl;
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

        auto cli = OPCUAExchange::init_opcuaexchange(argc, argv, shmID);

        if( !cli )
        {
            cerr << "init OPCUAClient failed." << endl;
            return 1;
        }

        auto act = UniSetActivator::Instance();
        act->add(cli);

        SystemMessage sm(SystemMessage::StartUp);
        act->broadcast( sm.transport_msg() );

        ulogany << "\n\n\n";
        ulogany << "(main): -------------- OPAUAClient START -------------------------\n\n";
        dlogany << "\n\n\n";
        dlogany << "(main): -------------- OPCUAClient START -------------------------\n\n";
        act->run(false);
        return 0;
    }
    catch( uniset::Exception& ex )
    {
        cerr << "(opcuaclient): " << ex << std::endl;
    }
    catch(...)
    {
        std::exception_ptr p = std::current_exception();
        cerr << "(opcuaclient): catch.." << (p ? p.__cxa_exception_type()->name() : "null") << std::endl;
    }

    return 1;
}
