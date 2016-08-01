/*
 * Copyright (c) 2015 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Lesser Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
//----------------------------------------------------------------------------
#ifndef PostgreSQLInterface_H_
#define PostgreSQLInterface_H_
// ---------------------------------------------------------------------------
#include <string>
#include <list>
#include <vector>
#include <queue>
#include <iostream>
#include <pqxx/pqxx>
#include <PassiveTimer.h>
#include <DBInterface.h>
// ----------------------------------------------------------------------------
class PostgreSQLInterface:
	public DBNetInterface
{
	public:

		PostgreSQLInterface();
		~PostgreSQLInterface();

		virtual bool nconnect( const std::string& host, const std::string& user,
							   const std::string& pswd, const std::string& dbname,
							   unsigned int port = 5432) override;
		virtual bool close() override;
		virtual bool isConnection() override;
		virtual bool ping() override; // проверка доступности БД

		virtual DBResult query( const std::string& q ) override;
		virtual const std::string lastQuery() override;

		virtual bool insert( const std::string& q ) override;
		bool insertAndSaveRowid( const std::string& q );
		virtual double insert_id() override;
		void save_inserted_id( const pqxx::result& res );

		typedef std::list<std::string> Record;
		typedef std::vector<Record> Data;

		// fast insert: Use COPY..from SDTIN..
		bool copy( const std::string& tblname, const std::list<std::string>& cols, const Data& data );

		virtual const std::string error() override;

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
