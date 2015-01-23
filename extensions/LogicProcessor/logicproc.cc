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
    std::ios::sync_with_stdio(false);
    try
    {
        auto conf = uniset_init( argc, argv );

        string logfilename(conf->getArgParam("--logicproc-logfile"));
        if( logfilename.empty() )
            logfilename = "logicproc.log";

        conf->initDebug(dlog(),"dlog");

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
