#include <sstream>
#include "UniSetActivator.h"
#include "Extensions.h"
#include "UNetExchange.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
using namespace uniset::extensions;
// -----------------------------------------------------------------------------
int main( int argc, const char** argv )
{
	//	std::ios::sync_with_stdio(false);

	try
	{
		if( argc > 1 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) )
		{
			cout << "--smemory-id objectName  - SharedMemory objectID. Default: read from <SharedMemory>" << endl;
			cout << "--confile filename       - configuration file. Default: configure.xml" << endl;
			cout << endl;
			UNetExchange::help_print(argc, argv);
			return 0;
		}

		auto conf = uniset_init(argc, argv);

		ObjectId shmID = DefaultObjectId;
		string sID = conf->getArgParam("--smemory-id");

		if( !sID.empty() )
			shmID = conf->getControllerID(sID);
		else
			shmID = getSharedMemoryID();

		if( shmID == DefaultObjectId )
		{
			cerr << sID << "? SharedMemoryID not found in " << conf->getControllersSection() << " section" << endl;
			return 1;
		}

		auto unet = UNetExchange::init_unetexchange(argc, argv, shmID);

		if( !unet )
		{
			dcrit << "(unetexchange): init failed.." << endl;
			return 1;
		}

		auto act = UniSetActivator::Instance();
		act->add(unet);

		SystemMessage sm(SystemMessage::StartUp);
		act->broadcast( sm.transport_msg() );

		ulogany << "\n\n\n";
		ulogany << "(main): -------------- UDPRecevier START -------------------------\n\n";
		dlogany << "\n\n\n";
		dlogany << "(main): -------------- UDPReceiver START -------------------------\n\n";

		act->run(false);
		on_sigchild(SIGTERM);
	}
	catch( const uniset::Exception& ex )
	{
		dcrit << "(unetexchange): " << ex << std::endl;
	}
	catch(...)
	{
		dcrit << "(unetexchange): catch ..." << std::endl;
	}

	on_sigchild(SIGTERM);
	return 0;
}
