#include <iostream>
#include <sstream>
#include "Exceptions.h"
#include "SQLiteInterface.h"
// --------------------------------------------------------------------------
using namespace uniset;
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

		DBResult r = db.query(q.str());

		if( !r )
		{
			cerr << "db connect error: " << db.error() << endl;
			return 1;
		}

		for( DBResult::iterator it = r.begin(); it != r.end(); it++ )
		{
			cout << "ROW: ";
			DBResult::COL col(*it);

			for( DBResult::COL::iterator cit = it->begin(); cit != it->end(); cit++ )
				cout << DBResult::as_string(cit) << "(" << DBResult::as_double(cit) << ")  |  ";

			cout << endl;
			
//			for( int i=0; i<col.size(); i++ )
//				cerr << "[" << i << "]: " << r.getColName(i) << endl;

			cout << "ID: " << r.as_string(it,"id") << endl;
			cout << "date: " << it.as_string("date") << endl;
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
