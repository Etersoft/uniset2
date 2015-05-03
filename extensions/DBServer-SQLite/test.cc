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

		stringstream q;
		q << "SELECT * from main_history";

		SQLiteResult r = db.query(q.str());

		if( !r )
		{
			cerr << "db connect error: " << db.error() << endl;
			return 1;
		}

		for( SQLiteResult::iterator it = r.begin(); it != r.end(); it++ )
		{
			cout << "ROW: ";
			SQLiteResult::COL col(*it);

			for( SQLiteResult::COL::iterator cit = it->begin(); cit != it->end(); cit++ )
				cout << as_string(cit) << "(" << as_double(cit) << ")  |  ";

			cout << endl;
		}

		db.close();
	}
	catch( const std::exception& ex )
	{
		cerr << "(test): " << ex.what() << endl;
	}
	catch(...)
	{
		cerr << "(test): catch ..." << endl;
	}

	return 0;
}
