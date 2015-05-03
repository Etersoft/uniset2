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
static shared_ptr<UInterface> ui;
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
			ui = make_shared<UInterface>();

		smi = make_shared<SMInterface>(shm->getId(), ui, myID, shm );
	}

	return smi;
}
// --------------------------------------------------------------------------
void run_test(std::size_t concurrency, int bound, shared_ptr<SharedMemory>& shm )
{
	auto&& r_worker = [&shm, bound]
	{
		int num = bound;

		while (num--)
		{
			int v = shm->getValue(11);
		}
	};

	auto&& w_worker = [&shm, bound]
	{
		int num = bound;

		while (num--)
		{
			shm->setValue(11, num);
		}
	};

	std::vector<std::thread> threads;

	for (std::size_t i = 0; i < concurrency - 1; ++i)
	{
		threads.emplace_back(r_worker);
	}

	threads.emplace_back(w_worker);

	for (auto && thread : threads)
	{
		thread.join();
	}
}
// --------------------------------------------------------------------------
int main(int argc, char* argv[] )
{
	try
	{
		auto conf = uniset_init(argc, argv);

		shm = SharedMemory::init_smemory(argc, argv);

		if( !shm )
			return 1;

		auto act = UniSetActivator::Instance();

		act->add(shm);
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

		run_test(8, 1000000, shm);
		return 0;

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
