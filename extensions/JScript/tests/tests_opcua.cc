#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

#include <string>
#include <unordered_set>
#include "Debug.h"
#include "UniSetActivator.h"
#include "PassiveTimer.h"
#include "SharedMemory.h"
#include "JSProxy.h"
#include "Extensions.h"
#include "OPCUATestServer.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace uniset;
using namespace uniset::extensions;
// --------------------------------------------------------------------------
std::shared_ptr<SharedMemory> shm;
std::shared_ptr<JSProxy> js;
std::shared_ptr<OPCUATestServer> server;
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
        auto conf = uniset_init(argc, argv);
        bool apart = findArgParam("--apart", argc, argv) != -1;

        shm = SharedMemory::init_smemory(argc, argv);

        if( !shm )
            return 1;

        js = JSProxy::init(argc, argv);

        if( !js )
            return 1;


        auto act = UniSetActivator::Instance();

        act->add(shm);
        act->add(js);

        SystemMessage sm(SystemMessage::StartUp);
        act->broadcast( sm.transport_msg() );
        act->run(true);

        int tout = 6000;
        PassiveTimer pt(tout);

        while( !pt.checkTime() || !act->exist() || !js->exist() || !shm->exist() )
            msleep(300);

        if( !shm->exist() )
        {
            cerr << "(tests_opc_ua): SharedMemory not exist! (timeout=" << tout << ")" << endl;
            return 1;
        }

        if( !js->exist() )
        {
            cerr << "(tests_opc_ua): JSEngine not exist! (timeout=" << tout << ")" << endl;
            return 1;
        }

        int rc = session.run();

        if( server )
        {
            server->stop();
            server.reset();
        }

        return rc;
    }
    catch( const SystemError& err )
    {
        cerr << "(tests_opc_ua): " << err << endl;
    }
    catch( const uniset::Exception& ex )
    {
        cerr << "(tests_opc_ua): " << ex << endl;
    }
    catch( const std::exception& e )
    {
        cerr << "(tests_opc_ua): " << e.what() << endl;
    }
    catch(...)
    {
        cerr << "(tests_opc_ua): catch(...)" << endl;
    }

    return 1;
}
