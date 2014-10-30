#include <iostream>
#include <sstream>
#include "Exceptions.h"
#include "SQLiteInterface.h"
// --------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
// --------------------------------------------------------------------------
int main(int argc, char** argv)
{
	try
	{
		SQLiteInterface db;
		if( !db.connect("test.db") )
		{
			cerr << "db connect error: " << db.error() << endl;
			return 1;
		}

#if 0
		CREATE TABLE main_history (
		  id INTEGER PRIMARY KEY AUTOINCREMENT,
		  date date NOT NULL,
		  time time NOT NULL,
		  time_usec INTEGER NOT NULL,
		  sensor_id INTEGER NOT NULL,
		  value DOUBLE NOT NULL,
		  node INTEGER NOT NULL,
		  confirm INTEGER DEFAULT NULL
#endif
#if 0

		bool fail = false;
		db.query("BEGIN;");
		for( int i=0; i<20; i++ )
		{
			stringstream qi;

			qi	<< " INSERT INTO main_history VALUES(NULL,"
				<< "date('now'), "
				<< "date('now'), "
				<< i << ","
				<< i << ","
				<< i << ","
				<<"0,0);";
				
			if( !db.insert(qi.str()) )
			{
				cerr << "db insert error: " << db.error() << endl;
				fail = true;
				break;
			}
		}
		if( fail )
			db.query("ROLLBACK;");
    	else
			db.query("COMMIT;");
#endif
		// ---------------------------------------
		
		stringstream q;
		q << "SELECT * from main_history";
		
		SQLiteResult r = db.query(q.str());
		if( !r )
		{
			cerr << "db connect error: " << db.error() << endl;
			return 1;
		}

		for( SQLiteResult::iterator it=r.begin(); it!=r.end(); it++ )
		{
			cout << "ROW: ";
			SQLiteResult::COL col(*it);
			for( SQLiteResult::COL::iterator cit = it->begin(); cit!=it->end(); cit++ )
				cout << as_string(cit) << "(" << as_double(cit) << ")  |  ";
			cout << endl;
		}

		db.close();
	}
	catch(Exception& ex)
	{
		cerr << "(test): " << ex << endl;
	}
	catch(...)
	{
		cerr << "(test): catch ..." << endl;
	}

	return 0;
}
