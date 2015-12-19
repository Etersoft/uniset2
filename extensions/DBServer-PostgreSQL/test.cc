#include <iostream>
#include <sstream>
#include "Exceptions.h"
#include "PostgreSQLInterface.h"
// --------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
// --------------------------------------------------------------------------
int main(int argc, char** argv)
{
	std::string dbname("test-db");

	if( argc > 1 )
		dbname = string(argv[1]);

	try
	{
		PostgreSQLInterface db;

		if( !db.nconnect("localhost", "dbadmin", "dbadmin", dbname) )
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
		}

		db.close();
	}
	catch( const Exception& ex )
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
