#include <iostream>
#include "Configuration.h"
#include "Extensions.h"
#include "LProcessor.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// -----------------------------------------------------------------------------
int main(int argc, const char **argv)
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
        ulog.logFile( logname.str() );
        dlog.logFile( logname.str() );

        string schema = conf->getArgParam("--schema");
        if( schema.empty() )
        {
            dlog.crit() << "schema-file not defined. Use --schema" << endl;
            return 1;
        }

        LProcessor plc;
        plc.execute(schema);
        return 0;
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

    return 1;
}
// -----------------------------------------------------------------------------
