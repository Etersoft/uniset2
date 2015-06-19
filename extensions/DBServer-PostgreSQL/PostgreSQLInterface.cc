// --------------------------------------------------------------------------
#include <sstream>
#include <cstdio>
#include <UniSetTypes.h>
#include "PostgreSQLInterface.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace pqxx;
// --------------------------------------------------------------------------

PostgreSQLInterface::PostgreSQLInterface():
	lastQ(""),
	lastE(""),
	last_inserted_id(0)
{
	//db = make_shared<pqxx::connection>();
}

PostgreSQLInterface::~PostgreSQLInterface()
{
	close();
}

// -----------------------------------------------------------------------------------------
bool PostgreSQLInterface::ping()
{
	return db && db->is_open();
}
// -----------------------------------------------------------------------------------------
bool PostgreSQLInterface::connect( const string& host, const string& user, const string& pswd, const string& dbname)
{
	if( db )
		return true;

	std::string conninfo = "dbname=" + dbname + " host=" + host + " user=" + user + " password=" + pswd;

	try
	{
		db = make_shared<pqxx::connection>(conninfo);
		return db->is_open();

	}
	catch( const std::exception& e )
	{
		cerr << e.what() << std::endl;
	}

	return false;
}
// -----------------------------------------------------------------------------------------
bool PostgreSQLInterface::close()
{
	if( db )
	{
		db->disconnect();
		db.reset();
	}

	return true;
}
// -----------------------------------------------------------------------------------------
bool PostgreSQLInterface::insert( const string& q )
{
	if( !db )
		return false;

	try
	{
		work w( *(db.get()) );
		w.exec(q);
		w.commit();
		return true;
	}
	catch( const std::exception& e )
	{
		//cerr << e.what() << std::endl;
		lastE = string(e.what());
	}

	return false;
}
// -----------------------------------------------------------------------------------------
bool PostgreSQLInterface::insertAndSaveRowid( const string& q )
{
	if( !db )
		return false;

	std::string qplus = q + " RETURNING id";

	try
	{
		work w( *(db.get()) );
		pqxx::result res = w.exec(qplus);
		w.commit();
		save_inserted_id(res);
		return true;
	}
	catch( const std::exception& e )
	{
		//cerr << e.what() << std::endl;
		lastE = string(e.what());
	}

	return false;
}
// -----------------------------------------------------------------------------------------
PostgreSQLResult PostgreSQLInterface::query( const string& q )
{
	if( !db )
		return PostgreSQLResult();

	try
	{
		nontransaction n(*(db.get()));

		/* Execute SQL query */
		result res( n.exec(q) );
		return PostgreSQLResult(res);
	}
	catch( const std::exception& e )
	{
		lastE = string(e.what());
	}

	return PostgreSQLResult();
}
// -----------------------------------------------------------------------------------------
string PostgreSQLInterface::error()
{
	return lastE;
}
// -----------------------------------------------------------------------------------------
const string PostgreSQLInterface::lastQuery()
{
	return lastQ;
}
// -----------------------------------------------------------------------------------------
double PostgreSQLInterface::insert_id()
{
	return last_inserted_id;
}
// -----------------------------------------------------------------------------------------
void PostgreSQLInterface::save_inserted_id( const pqxx::result& res )
{
	if( res.size() > 0 && res[0].size() > 0 )
		last_inserted_id = res[0][0].as<int>();
}
// -----------------------------------------------------------------------------------------
bool PostgreSQLInterface::isConnection()
{
	return (db && db->is_open());
}
// -----------------------------------------------------------------------------------------
int PostgreSQLResult::num_cols( const PostgreSQLResult::iterator& it )
{
	return it->size();
}
// -----------------------------------------------------------------------------------------
int PostgreSQLResult::as_int( const PostgreSQLResult::iterator& it, int col )
{
	//    if( col<0 || col >it->size() )
	//        return 0;
	return uni_atoi( (*it)[col] );
}
// -----------------------------------------------------------------------------------------
double PostgreSQLResult::as_double( const PostgreSQLResult::iterator& it, int col )
{
	return atof( ((*it)[col]).c_str() );
}
// -----------------------------------------------------------------------------------------
std::string PostgreSQLResult::as_string( const PostgreSQLResult::iterator& it, int col )
{
	return ((*it)[col]);
}
// -----------------------------------------------------------------------------------------
int PostgreSQLResult::as_int( const PostgreSQLResult::COL::iterator& it )
{
	return uni_atoi( (*it) );
}
// -----------------------------------------------------------------------------------------
double PostgreSQLResult::as_double( const PostgreSQLResult::COL::iterator& it )
{
	return atof( (*it).c_str() );
}
// -----------------------------------------------------------------------------------------
std::string PostgreSQLResult::as_string( const PostgreSQLResult::COL::iterator& it )
{
	return (*it);
}
// -----------------------------------------------------------------------------------------
#if 0
PostgreSQLResult::COL PostgreSQLResult::get_col( const PostgreSQLResult::ROW::iterator& it )
{
	return (*it);
}
#endif
// -----------------------------------------------------------------------------------------
PostgreSQLResult::~PostgreSQLResult()
{

}
// -----------------------------------------------------------------------------------------
PostgreSQLResult::PostgreSQLResult( const pqxx::result& res )
{
	for (result::const_iterator c = res.begin(); c != res.end(); ++c)
	{
		COL col;

		for( int i = 0; i < c.size(); i++ )
			col.push_back( c[i].as<string>() );

		row.push_back(col);
	}
}
// -----------------------------------------------------------------------------------------
