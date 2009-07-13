#include <iostream>
#include "Configuration.h"
#include "Extensions.h"
#include "LProcessor.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// -----------------------------------------------------------------------------
int main(int argc, char **argv)
{
	try
	{
		string confile=UniSetTypes::getArgParam("--confile",argc,argv,"configure.xml");
		conf = new Configuration( argc, argv, confile );

		string logfilename(conf->getArgParam("--logicproc-logfile"));
		if( logfilename.empty() )
			logfilename = "logicproc.log";
	
		conf->initDebug(dlog,"dlog");
	
		std::ostringstream logname;
		string dir(conf->getLogDir());
		logname << dir << logfilename;
		unideb.logFile( logname.str().c_str() );
		dlog.logFile( logname.str().c_str() );

		string schema = conf->getArgParam("--schema");
		if( schema.empty() )
		{
			dlog[Debug::CRIT] << "schema-file not defined. Use --schema" << endl;
			return 1;
		}

		LProcessor plc;
		plc.execute(schema);
	}
	catch( LogicException& ex )
	{
		cerr << ex << endl;
	}
	catch( Exception& ex )
	{
		cerr << ex << endl;
	}
	catch( ... )
	{
		cerr << " catch ... " << endl;
	}
	
	return 0;
}
// -----------------------------------------------------------------------------
