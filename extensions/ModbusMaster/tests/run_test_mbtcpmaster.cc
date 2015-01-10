#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

#include <string>
#include "Debug.h"
#include "UniSetActivator.h"
#include "PassiveTimer.h"
#include "SharedMemory.h"
#include "Extensions.h"
#include "MBTCPMaster.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// --------------------------------------------------------------------------
int main(int argc, char* argv[] )
{
    Catch::Session session;
    if( argc>1 && ( strcmp(argv[1],"--help")==0 || strcmp(argv[1],"-h")==0 ) )
    {
        cout << "--confile    - Использовать указанный конф. файл. По умолчанию configure.xml" << endl;
        SharedMemory::help_print(argc, argv);
        cout << endl << endl << "--------------- CATCH HELP --------------" << endl;
        session.showHelp("tests_mbtcpmaster");
        return 0;
    }

    int returnCode = session.applyCommandLine( argc, argv, Catch::Session::OnUnusedOptions::Ignore );
    if( returnCode != 0 ) // Indicates a command line error
        return returnCode;

    try
    {
        auto conf = uniset_init(argc,argv);
        conf->initDebug(dlog,"dlog");
        dlog.logFile("./smtest.log");

        bool apart = findArgParam("--apart",argc,argv) != -1;

        auto shm = SharedMemory::init_smemory(argc, argv);
        if( !shm )
            return 1;

        auto mb = MBTCPMaster::init_mbmaster(argc,argv,shm->getId(), (apart ? nullptr : shm ));
        if( !mb )
            return 1;

        auto act = UniSetActivator::Instance();

        act->addObject(shm);
        act->addObject(mb);

        SystemMessage sm(SystemMessage::StartUp);
        act->broadcast( sm.transport_msg() );
        act->run(true);

        int tout = 6000;
        PassiveTimer pt(tout);
        while( !pt.checkTime() && !act->exist() )
            msleep(100);

        if( !act->exist() )
        {
            cerr << "(tests_mbtcpmaster): SharedMemory not exist! (timeout=" << tout << ")" << endl;
            return 1;
        }

        return session.run();
    }
    catch( SystemError& err )
    {
        cerr << "(tests_mbtcpmaster): " << err << endl;
    }
    catch( Exception& ex )
    {
        cerr << "(tests_mbtcpmaster): " << ex << endl;
    }
    catch( std::exception& e )
    {
        cerr << "(tests_mbtcpmaster): " << e.what() << endl;
    }
    catch(...)
    {
        cerr << "(tests_mbtcpmaster): catch(...)" << endl;
    }

    return 1;
}
