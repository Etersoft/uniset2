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
#include "MBTCPMultiMaster.h"
#include "Configuration.h"
#include "Debug.h"
#include "UniSetActivator.h"
#include "Extensions.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// -----------------------------------------------------------------------------
int main( int argc, const char** argv )
{
	if( argc > 1 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) )
	{
		cout << "--smemory-id objectName  - SharedMemory objectID. Default: get from configure..." << endl;
		cout << "--confile filename       - configuration file. Default: configure.xml" << endl;
		cout << endl;
		MBTCPMultiMaster::help_print(argc, argv);
		return 0;
	}

	try
	{
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

		auto act = UniSetActivator::Instance();

		int count = conf->getArgPInt("--count",50);
		for( int i=1; i<=count; i++ )
		{
			ostringstream prefix;
			prefix << "mbtcp" << i;

			auto mb = MBTCPMultiMaster::init_mbmaster(argc, argv,shmID,nullptr, prefix.str());
			if( !mb )
			{
				cerr << "(main): " << prefix.str() <<  " init failed..." << endl;
				return 1;
			}

			act->add(mb);
		}

		SystemMessage sm(SystemMessage::StartUp);
		act->broadcast( sm.transport_msg() );
		act->run(false);
		return 0;
	}
	catch( const Exception& ex )
	{
		cerr << "(mbtcpmultimaster): " << ex << std::endl;
	}
	catch(...)
	{
		cerr << "(mbtcpmultimaster): catch ..." << std::endl;
	}

	return 1;
}
