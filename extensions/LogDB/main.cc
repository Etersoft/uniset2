#include "Configuration.h"
#include "LogDB.h"
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
            cout << "--confile filename - configuration file. Default: configure.xml" << endl;
            LogDB::help_print();
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

        auto db = LogDB::init_logdb(argc, argv);

        if( !db )
            return 1;

        db->run(false);
        return 0;
    }
    catch( const std::exception& ex )
    {
        cerr << "(LogDB::main): " << ex.what() << endl;
    }
    catch(...)
    {
        cerr << "(LogDB::main): catch ..." << endl;
    }

    if( rlock )
        rlock->unlock();

    return 1;
}
