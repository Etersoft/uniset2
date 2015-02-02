#include <sstream>
#include <UniSetActivator.h>
#include "Skel.h"
// -----------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
// -----------------------------------------------------------------------------
int main( int argc, const char **argv )
{
    try
    {
        auto conf = uniset_init(argc, argv);

        string logfilename = conf->getArgParam("--logfile", "Skel.log");
        string logname( conf->getLogDir() + logfilename );
        ulog.logFile( logname.c_str() );

        auto act = UniSetActivator::Instance();
        xmlNode* cnode = conf->getNode("Skel");
        if( cnode == NULL )
        {
            dlog.crit() << "(Skel): not found <Skel> in conffile" << endl;
            return 1;
        }

        Skel o("Skel",cnode);
        act.add(o.get_ptr());

        SystemMessage sm(SystemMessage::StartUp);
        act.broadcast( sm.transport_msg() );

        ulogany << "\n\n\n";
        ulogany << "(Skel::main): -------------- Skel START -------------------------\n\n";
        dlogany << "\n\n\n";
        dlogany << "(Skel::main): -------------- Skel START -------------------------\n\n";
        act->run(false);
    }
    catch( const std::exception& ex )
    {
        cerr << "(Skel::main): " << ex.what() << endl;
    }
    catch(...)
    {
        cerr << "(Skel::main): catch(...)" << endl;
    }

    return 0;
}
// -----------------------------------------------------------------------------
