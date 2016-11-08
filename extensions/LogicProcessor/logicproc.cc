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
#include <iostream>
#include "Configuration.h"
#include "Extensions.h"
#include "LProcessor.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
using namespace uniset::extensions;
// -----------------------------------------------------------------------------
int main(int argc, const char** argv)
{
	//	std::ios::sync_with_stdio(false);

	try
	{
		auto conf = uniset_init( argc, argv );

		string logfilename(conf->getArgParam("--logicproc-logfile"));

		if( logfilename.empty() )
			logfilename = "logicproc.log";

		std::ostringstream logname;
		string dir(conf->getLogDir());
		logname << dir << logfilename;
		ulog()->logFile( logname.str() );
		dlog()->logFile( logname.str() );

		string schema = conf->getArgParam("--schema");

		if( schema.empty() )
		{
			dcrit << "schema-file not defined. Use --schema" << endl;
			return 1;
		}

		LProcessor plc;
		plc.execute(schema);
		return 0;
	}
	catch( const LogicException& ex )
	{
		cerr << ex << endl;
	}
	catch( const uniset::Exception& ex )
	{
		cerr << ex << endl;
	}
	catch( ... )
	{
		cerr << " catch ... " << endl;
	}

	return 1;
}
// -----------------------------------------------------------------------------
