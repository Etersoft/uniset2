#include <iostream>
#include <sstream>
#include "Exceptions.h"
#include "ClickHouseInterface.h"
// --------------------------------------------------------------------------
using namespace uniset;
using namespace std;
// --------------------------------------------------------------------------
int main(int argc, char** argv)
{
    std::string dbname("testdb");

    if( argc > 1 )
        dbname = string(argv[1]);

    size_t num = 10;

    if( argc > 2 )
        num = atoi(argv[2]);

    try
    {
        ClickHouseInterface db;

        cout << "connect to '" << dbname << "'..." << endl;

        if( !db.nconnect("localhost", "", "", dbname) )
        {
            cerr << "[FAILED] connect error: " << db.error() << endl;
            db.close();
            return 1;
        }

        cout << "connect to '" << dbname << "' [OK]" << endl;

        auto col1 = std::make_shared<clickhouse::ColumnUInt64>();
        auto col2 = std::make_shared<clickhouse::ColumnUInt64>();

        auto arrTagNames = std::make_shared<clickhouse::ColumnArray>(std::make_shared<clickhouse::ColumnString>());
        auto arrTagValues = std::make_shared<clickhouse::ColumnArray>(std::make_shared<clickhouse::ColumnString>());

        for( size_t i = 0; i < num; i++ )
        {
            col1->Append(i);
            col2->Append(i);
            auto tagName = std::make_shared<clickhouse::ColumnString>();
            tagName->Append("tag1");

            auto tagVal = std::make_shared<clickhouse::ColumnString>();
            tagVal->Append("tagval1");
            arrTagValues->AppendAsColumn(tagVal);
            arrTagNames->AppendAsColumn(tagName);
        }


        clickhouse::Block blk(4, num);
        blk.AppendColumn("col1", col1);
        blk.AppendColumn("col2", col2);
        blk.AppendColumn("tags.name", arrTagNames);
        blk.AppendColumn("tags.value", arrTagValues);

        string tblname = dbname + ".tests";

        string qcreate = "CREATE TABLE IF NOT EXISTS " + tblname + " (col1 UInt64, col2 UInt64, tags Nested(name String, value String)) ENGINE = Memory";

        if( !db.execute(qcreate) )
        {
            cerr << "create table error: " << db.error() << endl;
            db.close();
            return 1;
        }


        for ( int i = 0; i < 5; i++ )
        {
            auto start = std::chrono::system_clock::now();

            if( !db.insert(tblname, blk) )
            {
                cerr << "query error: " << db.error() << endl;
                db.close();
                return 1;
            }

            auto end = std::chrono::system_clock::now();

            int elapsed_seconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            std::cerr << "INSERT " << num << " records elasped time: " << elapsed_seconds << " ms\n";
        }


        stringstream q;
        q << "SELECT * from " << tblname;

        DBResult r = db.query(q.str());

        if( !r )
        {
            cerr << "db connect error: " << db.error() << endl;
            db.close();
            return 1;
        }

        for( DBResult::iterator it = r.begin(); it != r.end(); it++ )
        {
            cout << "ROW: ";

            for( DBResult::COL::iterator cit = it->begin(); cit != it->end(); cit++ )
                cout << DBResult::as_string(cit) << "(" << DBResult::as_double(cit) << ")  |  ";

            cout << endl;
            cout << "col1: " << it.as_string("col1") << endl;
        }


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
