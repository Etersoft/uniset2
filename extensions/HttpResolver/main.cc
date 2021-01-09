#include "Configuration.h"
#include "HttpResolver.h"
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
            cout << endl;
            cout << "Usage: uniset2-httpresolver args1 args2" << endl;
            cout << endl;
            cout << "--confile filename - configuration file. Default: configure.xml" << endl;
            cout << endl;
            HttpResolver::help_print();
            return 0;
        }

        auto resolver = HttpResolver::init_resolver(argc, argv);

        if( !resolver )
            return 1;

        resolver->run();
        return 0;
    }
    catch( const std::exception& ex )
    {
        cerr << "(HttpResolver::main): " << ex.what() << endl;
    }
    catch(...)
    {
        cerr << "(HttpResolver::main): catch ..." << endl;
    }

    return 1;
}
