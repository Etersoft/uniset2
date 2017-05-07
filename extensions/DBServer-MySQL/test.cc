#include <iostream>
#include <sstream>
#include "Exceptions.h"
#include "MySQLInterface.h"
// --------------------------------------------------------------------------
using namespace uniset;
using namespace std;
// --------------------------------------------------------------------------
int main(int argc, char** argv)
{
	std::string dbname("test-db");
	std::string host("localhost");
	std::string table("main_history");

	if( argc > 1 )
		dbname = string(argv[1]);

	if( argc > 2 )
		host = string(argv[2]);

	if( argc > 3 )
		table = string(argv[3]);

	try
	{
		MySQLInterface db;

		if( !db.nconnect(host, "dbadmin", "dbadmin", dbname) )
		{
			cerr << "db connect error: " << db.error() << endl;
			return 1;
		}

		stringstream q;
		q << "SELECT * from " << table;

		DBResult r = db.query(q.str());

		if( !r )
		{
			cerr << "db connect error: " << db.error() << endl;
			return 1;
		}

		for( DBResult::iterator it = r.begin(); it != r.end(); it++ )
		{
			cout << "ROW: ";
//			DBResult::COL col(*it);
//			for( DBResult::COL::iterator cit = col.begin(); cit != col.end(); cit++ )
//				cout << DBResult::as_string(cit) << "(" << DBResult::as_double(cit) << ")  |  ";
//			cout << endl;

			for( DBResult::COL::iterator cit = it->begin(); cit != it->end(); cit++ )
				cout << DBResult::as_string(cit) << "(" << DBResult::as_double(cit) << ")  |  ";
			cout << endl;

			cout << "ID: " << r.as_string(it,"id") << endl;
			cout << "ID: " << it.as_string("id") << endl;
		}

		db.close();
	}
	catch( const uniset::Exception& ex )
	{
		cerr << "(test): " << ex << endl;
	}
	catch( const std::exception& ex )
	{
		cerr << "(test): " << ex.what() << endl;
	}

	return 0;
}
