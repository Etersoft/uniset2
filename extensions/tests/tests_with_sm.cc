#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

#include <memory>
#include <string>
#include "Debug.h"
#include "UniSetActivator.h"
#include "PassiveTimer.h"
#include "SharedMemory.h"
#include "SMInterface.h"
#include "Extensions.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// --------------------------------------------------------------------------
static shared_ptr<SMInterface> smi;
static shared_ptr<SharedMemory> shm;
static UInterface* ui = nullptr;
static ObjectId myID = 6000;
// --------------------------------------------------------------------------
shared_ptr<SharedMemory> shmInstance()
{
    if( !shm )
        throw SystemError("SharedMemory don`t initialize..");

    return shm;
}
// --------------------------------------------------------------------------
shared_ptr<SMInterface> smiInstance()
{
    if( smi == nullptr )
    {
        if( shm == nullptr )
            throw SystemError("SharedMemory don`t initialize..");

        if( ui == nullptr )
            ui = new UInterface();

        smi = make_shared<SMInterface>(shm->getId(), ui, myID, shm );
    }

    return smi;
}
// --------------------------------------------------------------------------
int main(int argc, char* argv[] )
{
    Catch::Session session;
    if( argc>1 && ( strcmp(argv[1],"--help")==0 || strcmp(argv[1],"-h")==0 ) )
    {
        cout << "--confile    - Использовать указанный конф. файл. По умолчанию configure.xml" << endl;
        SharedMemory::help_print(argc, argv);
        cout << endl << endl << "--------------- CATCH HELP --------------" << endl;
        session.showHelp("test_with_sm");
        return 0;
    }

    int returnCode = session.applyCommandLine( argc, argv, Catch::Session::OnUnusedOptions::Ignore );
      if( returnCode != 0 ) // Indicates a command line error
        return returnCode;
   
    try
    {
        auto conf = uniset_init(argc,argv);
/*
        conf->initDebug(dlog,"dlog");
        string logfilename = conf->getArgParam("--logfile", "smemory.log");
        string logname( conf->getLogDir() + logfilename );
        ulog.logFile( logname );
        dlog.logFile( logname );
*/
        shm = SharedMemory::init_smemory(argc, argv);
        if( !shm )
            return 1;

        auto act = UniSetActivator::Instance();


        act->addObject(shm);
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
        
        return session.run();
    }
    catch( SystemError& err )
    {
        cerr << "(tests_with_sm): " << err << endl;
    }
    catch( Exception& ex )
    {
        cerr << "(tests_with_sm): " << ex << endl;
    }
    catch( std::exception& e )
    {
        cerr << "(tests_with_sm): " << e.what() << endl;
    }
    catch(...)
    {
        cerr << "(tests_with_sm): catch(...)" << endl;
    }

    return 1;
}
