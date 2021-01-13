#include "Configuration.h"
#include "UWebSocketGate.h"
#include "Configuration.h"
#include "UniSetActivator.h"
// --------------------------------------------------------------------------
using namespace uniset;
using namespace std;
// --------------------------------------------------------------------------
int main(int argc, char** argv)
{
    //  std::ios::sync_with_stdio(false);

    try
    {
        if( argc > 1 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) )
        {
            cout << "--confile filename       - configuration file. Default: configure.xml" << endl;
            UWebSocketGate::help_print();
            return 0;
        }

        auto conf = uniset_init(argc, argv);

        auto ws = UWebSocketGate::init_wsgate(argc, argv, "ws-");

        if( !ws )
            return 1;

        auto act = UniSetActivator::Instance();
        act->add(ws);

        SystemMessage sm(SystemMessage::StartUp);
        act->broadcast( sm.transport_msg() );
        act->run(false);
        return 0;
    }
    catch( const std::exception& ex )
    {
        cerr << "(UWebSocketGate::main): " << ex.what() << endl;
    }
    catch(...)
    {
        cerr << "(UWebSocketGate::main): catch ..." << endl;
    }

    return 1;
}
