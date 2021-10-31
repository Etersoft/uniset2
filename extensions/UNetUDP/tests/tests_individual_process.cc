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
int main(int argc, char* argv[] )
{
    try
    {
        Catch::Session session;

        if( argc > 1 && ( strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0 ) )
        {
            cout << "--confile    - Использовать указанный конф. файл. По умолчанию configure.xml" << endl;
            SharedMemory::help_print(argc, argv);
            cout << endl << endl << "--------------- CATCH HELP --------------" << endl;
            session.showHelp("test_with_sm");
            return 0;
        }

        int returnCode = session.applyCommandLine( argc, argv, Catch::Session::OnUnusedOptions::Ignore );

//        if( returnCode != 0 ) // Indicates a command line error
//            return returnCode;

        uniset_init(argc, argv);

        UNetExchange* unet = UNetExchange::init_unetexchange(argc, argv, getSharedMemoryID());

        if( !unet )
            return 1;

        UniSetActivatorPtr act = UniSetActivator::Instance();

        act->addObject(static_cast<class UniSetObject*>(unet));

        SystemMessage sm(SystemMessage::StartUp);
        act->broadcast( sm.transport_msg() );
        act->run(true);

        int tout = 6000;
        PassiveTimer pt(tout);

        while( !pt.checkTime() && !act->exist() )
            msleep(100);

        if( !act->exist() )
        {
            cerr << "(tests_with_sm): SharedMemory not exist! (timeout=" << tout << ")" << endl;
            return 1;
        }

        int ret = session.run();

        act->oaDestroy();
        return ret;
    }
    catch( const SystemError& err )
    {
        cerr << "(tests_with_sm): " << err << endl;
    }
    catch( const uniset::Exception& ex )
    {
        cerr << "(tests_with_sm): " << ex << endl;
    }
    catch( const std::exception& e )
    {
        cerr << "(tests_with_sm): " << e.what() << endl;
    }
    catch(...)
    {
        cerr << "(tests_with_sm): catch(...)" << endl;
    }

    return 1;
}
