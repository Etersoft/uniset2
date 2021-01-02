#include <sstream>
#include <memory>
#include <UniSetActivator.h>
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

        xmlNode* cnode = conf->getNode("Skel");

        if( cnode == NULL )
        {
            cerr << "(Skel): not found <Skel> in conffile" << endl;
            return 1;
        }

        auto o = make_shared<Skel>("Skel", cnode);

        auto act = UniSetActivator::Instance();
        act->add(o);

        SystemMessage sm(SystemMessage::StartUp);
        act->broadcast( sm.transport_msg() );

        ulogany << "\n\n\n";
        ulogany << "(Skel::main): -------------- Skel START -------------------------\n\n";
        dlogany << "\n\n\n";
        dlogany << "(Skel::main): -------------- Skel START -------------------------\n\n";
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
