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
#ifndef SQLiteInterface_H_
#define SQLiteInterface_H_
// ---------------------------------------------------------------------------
#include <string>
#include <list>
#include <vector>
#include <iostream>
#include <sqlite3.h>
#include "PassiveTimer.h"
// ----------------------------------------------------------------------------
class SQLiteResult;
// ----------------------------------------------------------------------------
// Включение режима для журнала - "вести в памяти" (чтобы поберечь CompactFlash)
// PRAGMA journal_mode = MEMORY
//
// ----------------------------------------------------------------------------

class SQLiteInterface
{
	public:

		SQLiteInterface();
		~SQLiteInterface();

		bool connect( const std::string dbfile, bool create = false );
		bool close();
		bool isConnection();
		bool ping(); // проверка доступности БД

		inline void setOperationTimeout( timeout_t msec ){ opTimeout = msec; }
		inline timeout_t getOperationTimeout(){ return opTimeout; }

		inline void setOperationCheckPause( timeout_t msec ){ opCheckPause = msec; }
		inline timeout_t getOperationCheckPause(){ return opCheckPause; }

		SQLiteResult query( const std::string q );
		const std::string lastQuery();

		bool insert( const std::string q );
		int insert_id();

		std::string error();

	protected:

		bool wait( sqlite3_stmt* stmt, int result );
		static bool checkResult( int rc );

	private:

		sqlite3* db;
		// sqlite3_stmt* curStmt;

		std::string lastQ;
		std::string lastE;
		bool queryok;	// успешность текущего запроса
		bool connected;

		timeout_t opTimeout;
		timeout_t opCheckPause;
};
// ----------------------------------------------------------------------------------
class SQLiteResult
{
	public:
		SQLiteResult(){}
		SQLiteResult( sqlite3_stmt* s, bool finalize=true );
		~SQLiteResult();

		typedef std::vector<std::string> COL;
		typedef std::list<COL> ROW;

		typedef ROW::iterator iterator;

		inline iterator begin(){ return res.begin(); }
		inline iterator end(){ return res.end(); }

		inline operator bool(){ return !res.empty(); }

		inline int size(){ return res.size(); }
		inline bool empty(){ return res.empty(); }

	protected:

		ROW res;
};
// ----------------------------------------------------------------------------
int num_cols( SQLiteResult::iterator& );
// ROW
int as_int( SQLiteResult::iterator&, int col );
double as_double( SQLiteResult::iterator&, int col );
std::string as_text( SQLiteResult::iterator&, int col );
// ----------------------------------------------------------------------------
// COL
int as_int( SQLiteResult::COL::iterator& );
double as_double( SQLiteResult::COL::iterator& );
std::string as_string( SQLiteResult::COL::iterator& );
// ----------------------------------------------------------------------------
#endif
// ----------------------------------------------------------------------------------
