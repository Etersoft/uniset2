#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

#include <string>
#include "Debug.h"
#include "UniSetActivator.h"
#include "PassiveTimer.h"
#include "SharedMemory.h"
#include "OPCUAExchange.h"
#include "OPCUATestServer.h"
#include "Extensions.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace uniset;
using namespace uniset::extensions;
// --------------------------------------------------------------------------
std::shared_ptr<SharedMemory> shm;
std::shared_ptr<OPCUAExchange> opc;
shared_ptr<OPCUATestServer> opcTestServer1;
shared_ptr<OPCUATestServer> opcTestServer2;
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
        bool subscr = findArgParam("--subscr", argc, argv) != -1;

        shm = SharedMemory::init_smemory(argc, argv);

        if( !shm )
            return 1;

        auto act = UniSetActivator::Instance();

        opc = OPCUAExchange::init_opcuaexchange(argc, argv, shm->getId(), (apart ? nullptr : shm ), "opcua");

        if( !opc )
            return 1;

        act->add(opc);
        act->add(shm);

        SystemMessage sm(SystemMessage::StartUp);
        act->broadcast( sm.transport_msg() );
        act->run(true);

        struct RAIITestServer
        {
            RAIITestServer(shared_ptr<OPCUATestServer>& srv): server(srv)
            {
                server->start();
            }
            ~RAIITestServer()
            {
                server->stop();
            }

            shared_ptr<OPCUATestServer> server;
        };

        opcTestServer1 = make_shared<OPCUATestServer>("127.0.0.1", 4840);
        opcTestServer2 = make_shared<OPCUATestServer>("127.0.0.1", 4841);

        int rlimit = getArgInt("--server-maxNodesPerRead", argc, argv, "0");
        int wlimit = getArgInt("--server-maxNodesPerWrite", argc, argv, "0");
        opcTestServer1->setRWLimits(rlimit, wlimit);
        opcTestServer2->setRWLimits(rlimit, wlimit);

        if(subscr) //Если работает по подписке, то нужно до старта сервера создать объекты для подписки!
        {
            opcTestServer1->setI32("Attr1", 0);
            opcTestServer1->setBool("Attr2", false);
            opcTestServer1->setI32("Attr3", 0);
            opcTestServer1->setBool("Attr4", false);
            opcTestServer1->setI32("Attr5", 0);
            opcTestServer1->setI32("Attr6", 0);
            opcTestServer1->setI32("Attr7", 0);
            opcTestServer1->setF32("FloatOut1", 0.0);
            opcTestServer1->setF32("Float1", 0.0);
            opcTestServer1->setI32(101, 0);

            opcTestServer2->setI32("Attr1", 0);
            opcTestServer2->setBool("Attr2", false);
            opcTestServer2->setI32("Attr3", 0);
            opcTestServer2->setBool("Attr4", false);
            opcTestServer2->setI32("Attr5", 0);
            opcTestServer2->setI32("Attr6", 0);
            opcTestServer2->setI32("Attr7", 0);
            opcTestServer2->setF32("FloatOut1", 0.0);
            opcTestServer2->setF32("Float1", 0.0);
            opcTestServer2->setI32(101, 0);
        }

        RAIITestServer r1(opcTestServer1);
        RAIITestServer r2(opcTestServer2);

        int tout = 6000;
        PassiveTimer pt(tout);

        while( !pt.checkTime() || !act->exist() || !opc->exist() || !shm->exist() || !opcTestServer1->isRunning() || !opcTestServer2->isRunning() )
            msleep(300);

        if( !shm->exist() )
        {
            cerr << "(tests_with_sm): SharedMemory not exist! (timeout=" << tout << ")" << endl;
            return 1;
        }

        if( !opc->exist() )
        {
            cerr << "(tests_with_sm): OPCUAGate not exist! (timeout=" << tout << ")" << endl;
            return 1;
        }

        if( !opcTestServer1->isRunning() )
        {
            cerr << "(tests_with_sm): opcTestServer1 not exist! (timeout=" << tout << ")" << endl;
            return 1;
        }

        if( !opcTestServer2->isRunning() )
        {
            cerr << "(tests_with_sm): opcTestServer2 not exist! (timeout=" << tout << ")" << endl;
            return 1;
        }

        return session.run();
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
