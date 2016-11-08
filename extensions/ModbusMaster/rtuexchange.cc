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
#include <sstream>
#include "UniSetActivator.h"
#include "Extensions.h"
#include "RTUExchange.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
using namespace uniset::extensions;
// -----------------------------------------------------------------------------
int main( int argc, char** argv )
{
	//	std::ios::sync_with_stdio(false);

	try
	{
		if( argc > 1 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) )
		{
			cout << "--smemory-id objectName  - SharedMemory objectID. Default: read from <SharedMemory>" << endl;
			cout << "--confile filename       - configuration file. Default: configure.xml" << endl;
			cout << endl;
			RTUExchange::help_print(argc, argv);
			return 0;
		}

		auto conf = uniset_init( argc, argv );

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

		auto rs = RTUExchange::init_rtuexchange(argc, argv, shmID, 0, "rs");

		if( !rs )
		{
			dcrit << "(rtuexchange): init не прошёл..." << endl;
			return 1;
		}

		auto act = UniSetActivator::Instance();
		act->add(rs);

		SystemMessage sm(SystemMessage::StartUp);
		act->broadcast( sm.transport_msg() );

		ulogany << "\n\n\n";
		ulogany << "(main): -------------- RTU Exchange START -------------------------\n\n";
		dlogany << "\n\n\n";
		dlogany << "(main): -------------- RTU Exchange START -------------------------\n\n";

		act->run(false);
		return 0;
	}
	catch( const uniset::Exception& ex )
	{
		dcrit << "(rtuexchange): " << ex << std::endl;
	}
	catch(...)
	{
		dcrit << "(rtuexchange): catch ..." << std::endl;
	}

	return 1;
}
