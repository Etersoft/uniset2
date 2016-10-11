#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

#include <string>
#include "Debug.h"
#include "UniSetActivator.h"
#include "PassiveTimer.h"
#include "SharedMemory.h"
#include "Extensions.h"
#include "NullSM.h"
#include "TestObject.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// --------------------------------------------------------------------------
std::shared_ptr<TestObject> obj;
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
			session.showHelp("tests");
			return 0;
		}

		int returnCode = session.applyCommandLine( argc, argv, Catch::Session::OnUnusedOptions::Ignore );

		if( returnCode != 0 ) // Indicates a command line error
			return returnCode;

		auto conf = uniset_init(argc, argv);

		auto shm = SharedMemory::init_smemory(argc, argv);

		if( !shm )
			return 1;

		auto act = UniSetActivator::Instance();

		act->add(shm);

		/*
				ObjectId ns_id = conf->getControllerID("ReservSharedMemory");

				if( ns_id == DefaultObjectId )
				{
					cerr << "Not found ID for 'ReservSharedMemory'" << endl;
					return 1;
				}

				auto nullsm = make_shared<NullSM>(ns_id, "reserv-sm-configure.xml");
				act->add(nullsm);
		*/
		ObjectId o_id = conf->getObjectID("TestObject");

		if( o_id == DefaultObjectId )
		{
			cerr << "Not found ID for 'TestObject'" << endl;
			return 1;
		}

		xmlNode* o_node = conf->getNode("TestObject");

		obj = make_shared<TestObject>(o_id, o_node);
		act->add(obj);

		SystemMessage sm(SystemMessage::StartUp);
		act->broadcast( sm.transport_msg() );
		act->run(true);

		int tout = 6000;
		PassiveTimer pt(tout);

		while( !pt.checkTime() && !shm->exist() )
			msleep(100);

		while( !pt.checkTime() && !obj->exist() )
			msleep(100);

		if( !shm->exist() || !obj->exist() )
		{
			cerr << "(tests): SharedMemory not exist! (timeout=" << tout << ")" << endl;
			return 1;
		}

		return session.run();
	}
	catch( const SystemError& err )
	{
		cerr << "(tests): " << err << endl;
	}
	catch( const UniSetTypes::Exception& ex )
	{
		cerr << "(tests): " << ex << endl;
	}
	catch( const std::exception& e )
	{
		cerr << "(tests): " << e.what() << endl;
	}
	catch(...)
	{
		cerr << "(tests): catch(...)" << endl;
	}

	return 1;
}
