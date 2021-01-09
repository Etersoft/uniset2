#include "Configuration.h"
#include "DBServer_SQLite.h"
#include "UniSetActivator.h"
#include "Debug.h"
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
            cout << "Usage: uniset2-sqlite-dbserver --confile configure.xml args1 args2" << endl;
            cout << endl;
            DBServer_SQLite::help_print(argc, argv);
            cout << " Global options:" << endl;
            cout << uniset::Configuration::help() << endl;
            return 0;
        }

        auto conf = uniset_init(argc, argv, "configure.xml");

        auto db = DBServer_SQLite::init_dbserver(argc, argv);

        auto act = UniSetActivator::Instance();
        act->add(db);
        act->run(false);
    }
    catch( const std::exception& ex )
    {
        cerr << "(DBServer_SQLite::main): " << ex.what() << endl;
    }
    catch(...)
    {
        cerr << "(DBServer_SQLite::main): catch ..." << endl;
    }

    return 0;
}
