#include "Configuration.h"
#include "DBServer_SQLite.h"
#include "UniSetActivator.h"
#include "Debug.h"
#include "RunLock.h"
// --------------------------------------------------------------------------
using namespace uniset;
using namespace std;
// --------------------------------------------------------------------------
int main(int argc, char** argv)
{
    //  std::ios::sync_with_stdio(false);
    std::shared_ptr<RunLock> rlock = nullptr;
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

        int n = uniset::findArgParam("--run-lock",argc, argv);
        if( n != -1 )
        {
            if( n >= argc )
            {
                cerr << "Unknown lock file. Use --run-lock filename" << endl;
                return 1;
            }

            rlock = make_shared<RunLock>(argv[n+1]);
            if( rlock->isLocked() )
            {
                cerr << "ERROR: process is already running.. Lockfile: " << argv[n+1] << endl;
                return 1;
            }

            cout << "Run with lockfile: " << string(argv[n+1]) << endl;
            rlock->lock();
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

    if( rlock )
        rlock->unlock();

    return 0;
}
