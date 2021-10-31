#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

#include <string>
#include "Debug.h"
#include "UniSetActivator.h"
#include "PassiveTimer.h"
#include "SharedMemory.h"
#include "Extensions.h"
#include "UNetExchange.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace uniset;
using namespace uniset::extensions;
// --------------------------------------------------------------------------
int main(int argc, const char* argv[] )
{
    try
    {
        Catch::Session session;

        if( argc > 1 && ( strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0 ) )
        {
            cout << "--confile    - Использовать указанный конф. файл. По умолчанию configure.xml" << endl;
            SharedMemory::help_print(argc, argv);
            cout << endl << endl << "--------------- CATCH HELP --------------" << endl;
            session.showHelp();
            return 0;
        }

        int returnCode = session.applyCommandLine( argc, argv );

        if( returnCode != 0 ) // Indicates a command line error
            return returnCode;

        auto conf = uniset_init(argc, argv);

        bool apart = findArgParam("--apart", argc, argv) != -1;

        auto shm = SharedMemory::init_smemory(argc, argv);

        if( !shm )
            return 1;

        auto unet = UNetExchange::init_unetexchange(argc, argv, shm->getId(), (apart ? nullptr : shm ), "unet");

        if( !unet )
            return 1;

        auto act = UniSetActivator::Instance();

        act->add(shm);
        act->add(unet);

        SystemMessage sm(SystemMessage::StartUp);
        act->broadcast( sm.transport_msg() );
        act->run(true);

        int tout = 6000;
        PassiveTimer pt(tout);

        while( !pt.checkTime() && !act->exist() )
            msleep(100);

        if( !act->exist() )
        {
            cerr << "(tests_multicast_with_sm): SharedMemory not exist! (timeout=" << tout << ")" << endl;
            return 1;
        }

        return session.run();
    }
    catch( const SystemError& err )
    {
        cerr << "(tests_multicast_with_sm): " << err << endl;
    }
    catch( const uniset::Exception& ex )
    {
        cerr << "(tests_multicast_with_sm): " << ex << endl;
    }
    catch( const std::exception& e )
    {
        cerr << "(tests_multicast_with_sm): " << e.what() << endl;
    }
    catch(...)
    {
        cerr << "(tests_multicast_with_sm): catch(...)" << endl;
    }

    return 1;
}
