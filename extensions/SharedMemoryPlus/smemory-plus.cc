/*
 * Copyright (c) 2015 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Lesser Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
// -------------------------------------------------------------------------
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
#ifdef UNISET_ENABLE_IO
std::list< ThreadCreator<IOControl>* > lst_iothr;
#endif
// --------------------------------------------------------------------------
int main( int argc, const char** argv )
{
	//	std::ios::sync_with_stdio(false);

	if( argc > 1 && ( strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0 ) )
	{
		help_print( argc, argv );
		return 0;
	}

	try
	{
		auto conf = uniset_init(argc, argv);

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
