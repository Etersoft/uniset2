#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

#include <string>
#include "Debug.h"
#include "UniSetActivator.h"
#include "PassiveTimer.h"
#include "SharedMemory.h"
#include "Extensions.h"
#include "MBTCPMultiMaster.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace uniset;
using namespace uniset::extensions;
// --------------------------------------------------------------------------
std::shared_ptr<MBTCPMultiMaster> mbmm;
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
            session.showHelp("test_mbtcpmultimaster");
            return 0;
        }

        int returnCode = session.applyCommandLine( argc, argv, Catch::Session::OnUnusedOptions::Ignore );

        if( returnCode != 0 ) // Indicates a command line error
            return returnCode;

        auto conf = uniset_init(argc, argv);
        dlog()->logFile("./smtest.log");

        bool apart = findArgParam("--apart", argc, argv) != -1;

        auto shm = SharedMemory::init_smemory(argc, argv);

        if( !shm )
            return 1;

        auto mb = MBTCPMultiMaster::init_mbmaster(argc, argv, shm->getId(), (apart ? nullptr : shm ));

        if( !mb )
            return 1;

        mbmm = mb;

        auto act = UniSetActivator::Instance();

        act->add(shm);
        act->add(mb);

        SystemMessage sm(SystemMessage::StartUp);
        act->broadcast( sm.transport_msg() );
        act->run(true);
        int tout = 6000;
        PassiveTimer pt(tout);

        while( !pt.checkTime() && !act->exist() )
            msleep(100);

        if( !act->exist() )
        {
            cerr << "(tests_mbtcpmultimaster): SharedMemory not exist! (timeout=" << tout << ")" << endl;
            return 1;
        }

        return session.run();
    }
    catch( const SystemError& err )
    {
        cerr << "(tests_mbtcpmultimaster): " << err << endl;
    }
    catch( const uniset::Exception& ex )
    {
        cerr << "(tests_mbtcpmultimaster): " << ex << endl;
    }
    catch( const std::exception& e )
    {
        cerr << "(tests_mbtcpmultimaster): " << e.what() << endl;
    }
    catch(...)
    {
        cerr << "(tests_mbtcpmultimaster): catch(...)" << endl;
    }

    return 1;
}
