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
#include <DBInterface.h>
// ----------------------------------------------------------------------------
class PostgreSQLInterface : public DBNetInterface
{
	public:

		PostgreSQLInterface();
		~PostgreSQLInterface();

		bool nconnect( const std::string& host, const std::string& user, const std::string& pswd, const std::string& dbname ) override;
		bool close() override;
		bool isConnection() override;
		bool ping() override; // проверка доступности БД

		DBResult query( const std::string& q ) override;
		const std::string lastQuery() override;

		bool insert( const std::string& q ) override;
		bool insertAndSaveRowid( const std::string& q );
		double insert_id() override;
		void save_inserted_id( const pqxx::result& res );

		const std::string error() override;

	protected:

	private:

		void makeResult(DBResult& dbres, const pqxx::result& res );
		std::shared_ptr<pqxx::connection> db;
		std::string lastQ;
		std::string lastE;
		double last_inserted_id;
};
// ----------------------------------------------------------------------------
#endif
// ----------------------------------------------------------------------------------
