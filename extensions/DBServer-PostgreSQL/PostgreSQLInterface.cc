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
	try
	{
		close();
	}
	catch( ... ) // пропускаем все необработанные исключения, если требуется обработать нужно вызывать close() до деструктора
	{
		cerr << "MySQLInterface::~MySQLInterface(): an error occured while closing connection!" << endl;
	}
}

// -----------------------------------------------------------------------------------------
bool PostgreSQLInterface::ping()
{
	return db && db->is_open();
}
// -----------------------------------------------------------------------------------------
bool PostgreSQLInterface::nconnect( const string& host, const string& user, const string& pswd, const string& dbname)
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
DBResult PostgreSQLInterface::query( const string& q )
{
	if( !db )
		return DBResult();

	try
	{
		nontransaction n(*(db.get()));

		/* Execute SQL query */
		result res( n.exec(q) );
		DBResult dbres;
		makeResult(dbres, res);
		return dbres;
	}
	catch( const std::exception& e )
	{
		lastE = string(e.what());
	}

	return DBResult();
}
// -----------------------------------------------------------------------------------------
const string PostgreSQLInterface::error()
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
void PostgreSQLInterface::makeResult(DBResult& dbres, const pqxx::result& res )
{
	for( result::const_iterator c = res.begin(); c != res.end(); ++c )
	{
		DBResult::COL col;

		for( pqxx::result::tuple::size_type i = 0; i < c.size(); i++ )
			col.push_back( c[i].as<string>() );

		dbres.row().push_back(col);
	}
}
// -----------------------------------------------------------------------------------------
extern "C" std::shared_ptr<DBInterface> create_postgresqlinterface()
{
	return std::shared_ptr<DBInterface>(new PostgreSQLInterface(), DBInterfaceDeleter());
}
// -----------------------------------------------------------------------------------------
