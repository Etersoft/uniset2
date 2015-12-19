/* This file is part of the UniSet project
 * Copyright (c) 2002 Free Software Foundation, Inc.
 * Copyright (c) 2002 Pavel Vainerman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
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
							   const std::string& dbname) override;
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
