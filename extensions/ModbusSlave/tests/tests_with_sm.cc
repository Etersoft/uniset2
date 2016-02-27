#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

#include <string>
#include "Debug.h"
#include "UniSetActivator.h"
#include "PassiveTimer.h"
#include "SharedMemory.h"
#include "Extensions.h"
#include "MBSlave.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
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
			session.showHelp("test_with_sm");
			return 0;
		}

		int returnCode = session.applyCommandLine( argc, argv, Catch::Session::OnUnusedOptions::Ignore );

		if( returnCode != 0 ) // Indicates a command line error
			return returnCode;

		auto conf = uniset_init(argc, argv);

		bool apart = findArgParam("--apart", argc, argv) != -1;

		auto shm = SharedMemory::init_smemory(argc, argv);

		if( !shm )
			return 1;

		auto mbs = MBSlave::init_mbslave(argc, argv, shm->getId(), (apart ? nullptr : shm ));

		if( !mbs )
			return 1;

		auto act = UniSetActivator::Instance();

		act->add(shm);
		act->add(mbs);

		SystemMessage sm(SystemMessage::StartUp);
		act->broadcast( sm.transport_msg() );
		act->run(true);

		int tout = 6000;
		PassiveTimer pt(tout);

		while( !pt.checkTime() && !act->exist() && !mbs->exist() )
			msleep(100);

		if( !act->exist() )
		{
			cerr << "(tests_with_sm): SharedMemory not exist! (timeout=" << tout << ")" << endl;
			return 1;
		}

		if( !mbs->exist() )
		{
			cerr << "(tests_with_sm): ModbusSlave not exist! (timeout=" << tout << ")" << endl;
			return 1;
		}

		return session.run();
	}
	catch( const SystemError& err )
	{
		cerr << "(tests_with_sm): " << err << endl;
	}
	catch( const Exception& ex )
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
