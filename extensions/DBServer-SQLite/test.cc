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

		if( !db.query(q.str()) )
		{
			cerr << "db connect error: " << db.error() << endl;
			return 1;
		}

		SQLiteInterface::iterator it = db.begin();
		for( ; it!=db.end(); it++ )
		{
			cout << "get result: row=" << it.row_num() << " coln=" << it.get_num_cols() << endl;
			for( int i=0; i<it.get_num_cols(); i++ )
				cout << it.get_text(i) << "(" << it.get_double(i) << ")  |  ";
			cout << endl;
		}
		db.freeResult();
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
