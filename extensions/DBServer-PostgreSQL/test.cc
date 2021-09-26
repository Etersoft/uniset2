#include <iostream>
#include <sstream>
#include "Exceptions.h"
#include "PostgreSQLInterface.h"
// --------------------------------------------------------------------------
using namespace uniset;
using namespace std;
// --------------------------------------------------------------------------
int main(int argc, char** argv)
{
    std::string dbname("test-db");

    if( argc > 1 )
        dbname = string(argv[1]);

    size_t ver = 1;

    if( argc > 2 )
        ver = atoi(argv[2]);

    size_t num = 10000;

    if( argc > 3 )
        num = atoi(argv[3]);

    try
    {
        PostgreSQLInterface db;

        cout << "connect to '" << dbname << "'..." << endl;

        if( !db.nconnect("localhost", "dbadmin", "dbadmin", dbname) )
        {
            cerr << "[FAILED] connect error: " << db.error() << endl;
            db.close();
            return 1;
        }

        cout << "connect to '" << dbname << "' [OK]" << endl;

        stringstream q;

        if( ver == 1 )
        {
            for( size_t i = 0; i < num; i++ )
            {
                q << "INSERT INTO main_history(date,time,time_usec,sensor_id,value,node)"
                  << " VALUES(now(),now(),0,7,1,3000);";
            }
        }
        else if( ver == 2 )
        {
            q << "INSERT INTO main_history(date,time,time_usec,sensor_id,value,node) VALUES";

            for( size_t i = 0; i < num; i++ )
            {
                q << "(now(),now(),0,7,1,3000),";
            }

            q << "(now(),now(),0,7,1,3000);";
        }

        std::chrono::time_point<std::chrono::system_clock> start, end;

        if( ver == 3 )
        {
            std::initializer_list<std::string_view> cols{ "date", "time", "time_usec", "sensor_id", "value", "node" };
            PostgreSQLInterface::Data data;

            for( size_t i = 0; i < num; i++ )
            {
                PostgreSQLInterface::Record d = { "now()", "now()", "0", "7", "1", "3000" };
                data.push_back(std::move(d));
            }

            start = std::chrono::system_clock::now();

            if( !db.copy("main_history", cols, data) )
            {
                cerr << "query error: " << db.error() << endl;
                db.close();
                return 1;
            }
        }
        else
        {
            start = std::chrono::system_clock::now();

            if( !db.insert( std::move(q.str())) )
            {
                cerr << "query error: " << db.error() << endl;
                db.close();
                return 1;
            }
        }

        end = std::chrono::system_clock::now();

        int elapsed_seconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cerr << "INSERT " << num << " records elasped time: " << elapsed_seconds << " ms\n";

        db.close();
    }
    catch( const uniset::Exception& ex )
    {
        cerr << "(test): " << ex << endl;
    }
    catch( std::exception& ex )
    {
        cerr << "(test): " << ex.what() << endl;
    }
    catch(...)
    {
        cerr << "(test): catch ..." << endl;
    }

    return 0;
}
