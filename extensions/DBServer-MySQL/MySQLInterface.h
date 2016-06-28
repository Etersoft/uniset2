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
// --------------------------------------------------------------------------
/*! \file
 *  \author Pavel Vainerman
*/
//----------------------------------------------------------------------------
#ifndef MySQLInterface_H_
#define MySQLInterface_H_
// ---------------------------------------------------------------------------
#include <string>
#include <vector>
#include <deque>
#include <iostream>
//#warning Для использования mysql_create нужен define USE_OLD_FUNCTIONS
//#define USE_OLD_FUNCTIONS
#include <mysql/mysql.h>
#include <DBInterface.h>
// ----------------------------------------------------------------------------
class MySQLInterface:
	public DBNetInterface
{
	public:

		MySQLInterface();
		~MySQLInterface();

		//            DBResult listFields( const std::string& table, const std::string& wild );

		virtual bool nconnect( const std::string& host, const std::string& user, const std::string& pswd,
							   const std::string& dbname, unsigned int port=0 ) override;
		virtual bool close() override;

		bool query_ok( const std::string& q );

		// \param finalize - освободить буфер после запроса
		virtual DBResult query( const std::string& q ) override;

		virtual const std::string lastQuery() override;
		virtual bool insert( const std::string& q ) override;

		std::string addslashes(const std::string& str);

		/*!
		    проверка связи с БД.
		    в случае отсутсвия попытка восстановить...
		*/
		virtual bool ping() override;

		/*! связь с БД установлена (была) */
		virtual bool isConnection() override;

		virtual double insert_id() override;

		virtual const std::string error() override;

		// *******************
		const char* gethostinfo();
	protected:

	private:

		void makeResult(DBResult& dbres, MYSQL_RES* r, bool finalize = true );
		MYSQL* mysql;
		std::string lastQ;
		bool connected;
};
// ----------------------------------------------------------------------------------
#endif
