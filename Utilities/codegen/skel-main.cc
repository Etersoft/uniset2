#include <sstream>
#include <string>
#include <memory>
#include "UniSetActivator.h"
#include "UHelpers.h"
#include "Skel.h"
// -----------------------------------------------------------------------------
using namespace uniset;
using namespace std;
// -----------------------------------------------------------------------------
int main( int argc, const char** argv )
{
    try
    {
        auto conf = uniset_init(argc, argv);

        string name = conf->getArgParam("--name","Skel");
        string secname = conf->getArgParam("--secname","Skel");

        auto o = make_object<Skel>(name,secname);

        auto act = UniSetActivator::Instance();
        act->add(o);

        SystemMessage sm(SystemMessage::StartUp);
        act->broadcast( sm.transport_msg() );

        ulogany << "\n\n\n";
        ulogany << "(Skel::main): -------------- Skel START -------------------------\n\n";
        act->run(false);
        return 0;
    }
    catch( const std::exception& ex )
    {
        cerr << "(Skel::main): " << ex.what() << endl;
    }
    catch(...)
    {
        cerr << "(Skel::main): catch(...)" << endl;
    }

    return 1;
}
// -----------------------------------------------------------------------------
