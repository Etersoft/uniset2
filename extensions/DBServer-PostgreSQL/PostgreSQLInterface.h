//----------------------------------------------------------------------------
#ifndef PostgreSQLInterface_H_
#define PostgreSQLInterface_H_
// ---------------------------------------------------------------------------
#include <string>
#include <list>
#include <vector>
#include <iostream>
#include <pqxx/pqxx>
#include <PassiveTimer.h>
// ----------------------------------------------------------------------------
class PostgreSQLResult;
// ----------------------------------------------------------------------------
class PostgreSQLInterface
{
	public:

		PostgreSQLInterface();
		~PostgreSQLInterface();

		bool connect( const std::string& host, const std::string& user, const std::string& pswd, const std::string& dbname );
		bool close();
		bool isConnection();
		bool ping(); // проверка доступности БД

		PostgreSQLResult query( const std::string& q );
		const std::string lastQuery();

		bool insert( const std::string& q );
		bool insertAndSaveRowid( const std::string& q );
		double insert_id();
		void save_inserted_id( const pqxx::result& res );

		std::string error();

	protected:

	private:

		std::shared_ptr<pqxx::connection> db;
		std::string lastQ;
		std::string lastE;
		double last_inserted_id;
};
// ----------------------------------------------------------------------------------
class PostgreSQLResult
{
	public:
		PostgreSQLResult() {}
		PostgreSQLResult( const pqxx::result& res );
		~PostgreSQLResult();

		typedef std::vector<std::string> COL;
		typedef std::list<COL> ROW;

		typedef ROW::iterator iterator;

		inline iterator begin()
		{
			return row.begin();
		}
		inline iterator end()
		{
			return row.end();
		}

		inline operator bool()
		{
			return !row.empty();
		}

		inline int size()
		{
			return row.size();
		}
		inline bool empty()
		{
			return row.empty();
		}

	protected:

		ROW row;
};
// ----------------------------------------------------------------------------
int num_cols( PostgreSQLResult::iterator& );
// ROW
int as_int( PostgreSQLResult::iterator&, int col );
double as_double( PostgreSQLResult::iterator&, int col );
std::string as_text( PostgreSQLResult::iterator&, int col );
// ----------------------------------------------------------------------------
// COL
int as_int( PostgreSQLResult::COL::iterator& );
double as_double( PostgreSQLResult::COL::iterator& );
std::string as_string( PostgreSQLResult::COL::iterator& );
//----------------------------------------------------------------------------
#endif
// ----------------------------------------------------------------------------------
