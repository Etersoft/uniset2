#include "Configuration.h"
#include "DBServer_PostgreSQL.h"
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
            cout << "Usage: uniset2-pgsql-dbserver --confile configure.xml args1 args2" << endl;
            cout << endl;
            DBServer_PostgreSQL::help_print(argc, argv);
            cout << " Global options:" << endl;
            cout << uniset::Configuration::help() << endl;
            return 0;
        }

        auto conf = uniset_init(argc, argv);

        auto dbs = DBServer_PostgreSQL::init_dbserver(argc, argv);
        auto act = UniSetActivator::Instance();
        act->add(dbs);
        act->run(false);
    }
    catch( const uniset::Exception& ex )
    {
        cerr << "(DBServer_PosgreSQL::main): " << ex << endl;
    }
    catch( std::exception& ex )
    {
        cerr << "(DBServer_PosgreSQL::main): " << ex.what() << endl;
    }
    catch(...)
    {
        cerr << "(DBServer_PosgreSQL::main): catch ..." << endl;
    }

    return 0;
}
