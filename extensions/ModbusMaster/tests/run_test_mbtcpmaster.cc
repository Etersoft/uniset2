#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

#include <string>
#include "Debug.h"
#include "UniSetActivator.h"
#include "PassiveTimer.h"
#include "SharedMemory.h"
#include "Extensions.h"
#include "MBTCPMaster.h"
#include "SMInterface.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// --------------------------------------------------------------------------
std::shared_ptr<SharedMemory> shm;
// --------------------------------------------------------------------------
int main( int argc, const char* argv[] )
{
	try
	{
		Catch::Session session;

		if( argc > 1 && ( strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0 ) )
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

		auto conf = uniset_init(argc, argv);
		dlog()->logFile("./smtest.log");

		bool apart = findArgParam("--apart", argc, argv) != -1;

		shm = SharedMemory::init_smemory(argc, argv);

		if( !shm )
			return 1;

		auto mb = MBTCPMaster::init_mbmaster(argc, argv, shm->getId(), (apart ? nullptr : shm ));

		if( !mb )
			return 1;

		auto act = UniSetActivator::Instance();

		act->add(shm);
		act->add(mb);

		SystemMessage sm(SystemMessage::StartUp);
		act->broadcast( sm.transport_msg() );
		act->run(true);

		int tout = conf->getArgPInt("--timeout", 8000);
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
	catch( const Exception& ex )
	{
		cerr << "(tests_mbtcpmaster): " << ex << endl;
	}
	catch( const std::exception& e )
	{
		cerr << "(tests_mbtcpmaster): " << e.what() << endl;
	}
	catch(...)
	{
		cerr << "(tests_mbtcpmaster): catch(...)" << endl;
	}

	return 1;
}
