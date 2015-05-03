// --------------------------------------------------------------------------
#include <string>
#include <error.h>
#include <errno.h>
#include <memory>
#include <Debug.h>
#include <UniSetActivator.h>
#include <ThreadCreator.h>
#include "Extensions.h"
#include "RTUExchange.h"
#include "MBSlave.h"
#include "MBTCPMaster.h"
#include "SharedMemory.h"
//#include "UniExchange.h"
#include "UNetExchange.h"
#include "Configuration.h"
#ifdef UNISET_ENABLE_IO
#include "IOControl.h"
#endif
#include "LogAgregator.h"
#include "LogServer.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// --------------------------------------------------------------------------
const unsigned int MaxAddNum = 10;
// --------------------------------------------------------------------------
static void help_print( int argc, const char* argv[] );
static std::shared_ptr<LogServer> run_logserver( const std::string& cnamem, std::shared_ptr<LogAgregator>& log );
#ifdef UNISET_ENABLE_IO
std::list< ThreadCreator<IOControl>* > lst_iothr;
#endif
// --------------------------------------------------------------------------
int main( int argc, const char** argv )
{
	std::ios::sync_with_stdio(false);

	if( argc > 1 && ( strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0 ) )
	{
		help_print( argc, argv );
		return 0;
	}

	try
	{
		auto conf = uniset_init(argc, argv);

		string logfilename = conf->getArgParam("--logfile", "smemory-plus.log");
		string logname( conf->getLogDir() + logfilename );
		UniSetExtensions::dlog()->logFile( logname );
		ulog()->logFile( logname );
		dlog()->logFile( logname );

		auto act = UniSetActivator::Instance();
		//act->signal_terminate_event().connect( &activator_terminate );
		// ------------ SharedMemory ----------------
		auto shm = SharedMemory::init_smemory(argc, argv);

		if( !shm )
			return 1;

		act->add(shm);

#ifdef UNISET_ENABLE_IO

		// ------------ IOControl ----------------
		//        std::list< ThreadCreator<IOControl>* > lst_iothr;
		for( unsigned int i = 0; i < MaxAddNum; i++ )
		{
			stringstream s;
			s << "--add-io";

			if( i > 0 ) s << i;

			bool add_io = findArgParam(s.str(), argc, argv) != -1;

			if( add_io )
			{
				stringstream p;
				p << "io";

				if( i > 0 ) p << i;

				dinfo << "(smemory-plus): add IOControl(" << p.str() << ")" << endl;

				auto ic = IOControl::init_iocontrol(argc, argv, shm->getId(), shm, p.str());

				if( !ic )
					return 1;

				ThreadCreator<IOControl>* io_thr = new ThreadCreator<IOControl>(ic.get(), &IOControl::execute);

				if( io_thr == NULL )
					return 1;

				act->add(ic);
				lst_iothr.push_back( io_thr );
			}
		}

#endif

		// ------------- RTU Exchange --------------
		for( unsigned int i = 0; i < MaxAddNum; i++ )
		{
			stringstream s;
			s << "--add-rtu";

			if( i > 0 ) s << i;

			bool add_rtu = findArgParam(s.str(), argc, argv) != -1;

			if( add_rtu )
			{
				stringstream p;
				p << "rtu";

				if( i > 0 ) p << i;

				dinfo << "(smemory-plus): add RTUExchange(" << p.str() << ")" << endl;

				auto rtu = RTUExchange::init_rtuexchange(argc, argv, shm->getId(), shm, p.str());

				if( !rtu )
					return 1;

				act->add(rtu);
			}
		}

		// ------------- MBSlave --------------
		for( unsigned int i = 0; i < MaxAddNum; i++ )
		{
			stringstream s;
			s << "--add-mbslave";

			if( i > 0 ) s << i;

			bool add_mbslave = findArgParam(s.str(), argc, argv) != -1;

			if( add_mbslave )
			{
				stringstream p;
				p << "mbs";

				if( i > 0 ) p << i;

				dinfo << "(smemory-plus): add MBSlave(" << p.str() << ")" << endl;

				auto mbs = MBSlave::init_mbslave(argc, argv, shm->getId(), shm, p.str());

				if( !mbs )
					return 1;

				act->add(mbs);
			}
		}

		// ------------- MBTCPMaster --------------
		for( unsigned int i = 0; i < MaxAddNum; i++ )
		{
			stringstream s;
			s << "--add-mbmaster";

			if( i > 0 ) s << i;

			bool add_mbmaster = findArgParam(s.str(), argc, argv) != -1;

			if( add_mbmaster )
			{
				stringstream p;
				p << "mbtcp";

				if( i > 0 ) p << i;

				dinfo << "(smemory-plus): add MBTCPMaster(" << p.str() << ")" << endl;

				auto mbm1 = MBTCPMaster::init_mbmaster(argc, argv, shm->getId(), shm, p.str());

				if( !mbm1 )
					return 1;

				act->add(mbm1);
			}
		}

		// ------------- UNetUDP --------------
		bool add_unet = findArgParam("--add-unet", argc, argv) != -1;

		if( add_unet )
		{
			auto unet = UNetExchange::init_unetexchange(argc, argv, shm->getId(), shm);

			if( unet == NULL )
				return 1;

			dinfo << "(smemory-plus): add UNetExchnage.." << endl;

			act->add(unet);
		}

		// ---------------------------------------
		// попытка решить вопрос с "зомби" процессами
		signal( SIGCHLD, on_sigchild );
		// ---------------------------------------
		SystemMessage sm(SystemMessage::StartUp);
		act->broadcast( sm.transport_msg() );

#ifdef UNISET_IO_ENABLE

		for( auto& i : lst_iothr )
			i->start();

#endif

		auto la = make_shared<LogAgregator>();
		la->add(ulog());
		la->add(dlog());

		auto logserver = run_logserver("smplus", la);

		if( logserver == 0 )
		{
			cerr << "(smemory-plus): run logserver for 'smplus' FAILED" << endl;
			return 1;
		}

		act->run(false);

		on_sigchild(SIGTERM);
		return 0;
	}
	catch( const Exception& ex )
	{
		dcrit << "(smemory-plus): " << ex << endl;
	}
	catch( const CORBA::SystemException& ex )
	{
		dcrit << "(smemory-plus): " << ex.NP_minorString() << endl;
	}
	catch(...)
	{
		dcrit << "(smemory-plus): catch(...)" << endl;
	}

	on_sigchild(SIGTERM);
	return 1;
}
// --------------------------------------------------------------------------
void help_print( int argc, const char* argv[] )
{
	const int mnum = MaxAddNum - 1;

	cout << "--add-io[1..." << mnum << "]         - Start IOControl" << endl;
	cout << "--add-rtu[1..." << mnum << "]        - Start RTUExchange (rtu master)" << endl;
	cout << "--add-mbslave[1..." << mnum << "]    - Start ModbusSlave (RTU or TCP)" << endl;
	cout << "--add-mbmaster[1..." << mnum << "]   - Start MBTCPMaster" << endl;
	cout << "--add-unet              - Start UNetExchange (UNetUDP)" << endl;

	cout << endl << "###### SM options ######" << endl;
	SharedMemory::help_print(argc, argv);

#ifdef UNISET_IO_ENABLE
	cout << endl << "###### IO options ###### (prefix: --ioX)" << endl;
	IOControl::help_print(argc, argv);
#endif

	cout << endl << "###### RTU options ###### (prefix: --rtuX)" << endl;
	RTUExchange::help_print(argc, argv);

	cout << endl << "###### ModbusSlave options (prefix: --mbsX) ######" << endl;
	MBSlave::help_print(argc, argv);

	cout << endl << "###### ModbusTCP Master options (prefix: --mbtcpX) ######" << endl;
	MBTCPMaster::help_print(argc, argv);

	cout << endl << "###### UNetExchange options ######" << endl;
	UNetExchange::help_print(argc, argv);

	cout << endl << "###### Common options ######" << endl;
	cout << "--confile            - Use confile. Default: configure.xml" << endl;
	cout << "--logfile            - Use logfile. Default: smemory-plus.log" << endl;
}
// -----------------------------------------------------------------------------
std::shared_ptr<LogServer> run_logserver( const std::string& cname, std::shared_ptr<LogAgregator>& log )
{
	auto conf = uniset_conf();
	auto xml = conf->getConfXML();
	xmlNode* cnode = conf->findNode(xml->getFirstNode(), "LogServer", cname);

	if( cnode == 0 )
	{
		cerr << "(init_ulogserver): Not found xmlnode for '" << cname << "'" << endl;
		return 0;

	}

	UniXML::iterator it(cnode);

	auto ls = make_shared<LogServer>( log );

	timeout_t sessTimeout = conf->getArgPInt("--" + cname + "-session-timeout", it.getProp("sessTimeout"), 3600000);
	timeout_t cmdTimeout = conf->getArgPInt("--" + cname + "-cmd-timeout", it.getProp("cmdTimeout"), 2000);
	timeout_t outTimeout = conf->getArgPInt("--" + cname + "-out-timeout", it.getProp("outTimeout"), 2000);

	ls->setSessionTimeout(sessTimeout);
	ls->setCmdTimeout(cmdTimeout);
	ls->setOutTimeout(outTimeout);

	std::string host = conf->getArgParam("--" + cname + "-host", it.getProp("host"));

	if( host.empty() )
	{
		cerr << "(init_ulogserver): " << cname << ": unknown host.." << endl;
		return nullptr;
	}

	ost::tpport_t port = conf->getArgPInt("--" + cname + "-port", it.getProp("port"), 0);

	if( port == 0 )
	{
		cerr << "(init_ulogserver): " << cname << ": unknown port.." << endl;
		return nullptr;
	}

	cout << "logserver: " << host << ":" << port << endl;
	ls->run(host, port, true);
	return ls;
}
// -----------------------------------------------------------------------------
