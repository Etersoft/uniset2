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
// -------------------------------------------------------------------------- 
#include <sstream>
#include "UniSetTypes.h"
#include "SQLiteInterface.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
// --------------------------------------------------------------------------

SQLiteInterface::SQLiteInterface():
db(0),
lastQ(""),
queryok(false),
connected(false),
opTimeout(300),
opCheckPause(50)
{ 
}

SQLiteInterface::~SQLiteInterface()
{ 
	close();
	delete db;
}

// -----------------------------------------------------------------------------------------
bool SQLiteInterface::connect( const string dbfile )
{
	int rc = sqlite3_open(dbfile.c_str(), &db);
	if( !rc )
	{
		cerr << sqlite3_errmsg(db) << endl;
		sqlite3_close(db);
		db = 0;
		connected = false;
		return false;
	}

	if( rc==SQLITE_BUSY || rc==SQLITE_LOCKED || rc==SQLITE_INTERRUPT || rc==SQLITE_IOERR )
	{
		cerr << sqlite3_errmsg(db) << endl;
		sqlite3_close(db);
		db = 0;
		connected = false;
		return false;
	}

	
	connected = true;
	return true;
}
// -----------------------------------------------------------------------------------------
bool SQLiteInterface::close()
{
	if(db) 
		sqlite3_close(db);
	
	return true;
}
// -----------------------------------------------------------------------------------------
bool SQLiteInterface::insert( const string q )
{
	if( !db )
		return false;

	// char* errmsg;
	sqlite3_stmt* pStmt;

	// Компилируем SQL запрос
	sqlite3_prepare(db, q.c_str(), -1, &pStmt, NULL);
	int rc = sqlite3_step(pStmt);
	
	if( rc==SQLITE_BUSY || rc==SQLITE_LOCKED || rc==SQLITE_INTERRUPT || rc==SQLITE_IOERR )
	{
		if( !wait(pStmt, SQLITE_DONE) )
		{ 
			sqlite3_finalize(pStmt);
			queryok=false;
			return false;
		}
	}

	sqlite3_finalize(pStmt);
	queryok=true;
	return true;
}
// -----------------------------------------------------------------------------------------
bool SQLiteInterface::query( const string q )
{
	if( !db )
		return false;

	// char* errmsg = 0;
	sqlite3_stmt* pStmt;

	// Компилируем SQL запрос
	sqlite3_prepare(db, q.c_str(), -1, &pStmt, NULL);
	int rc = sqlite3_step(pStmt);
	if( rc==SQLITE_BUSY || rc==SQLITE_LOCKED || rc==SQLITE_INTERRUPT || rc==SQLITE_IOERR )
	{
		if( !wait(pStmt, SQLITE_ROW) )
		{
			sqlite3_finalize(pStmt);
			queryok=false;
			return false;
		}
	}
	
	lastQ = q;

//	int cnum = sqlite3_column_count(pStmt);

/*	
	while( (rc = sqlite3_step(pStmt)) == SQLITE_ROW )
	{
		int coln = sqlite3_data_count(pStmt);
		for( int j=0; j<coln; j++ )
		{
			
		}
	}
*/
	
	sqlite3_finalize(pStmt);
	queryok=true;
	return true;
}
// -----------------------------------------------------------------------------------------
bool SQLiteInterface::wait( sqlite3_stmt* stmt, int result )
{
	PassiveTimer ptTimeout(opTimeout);
	while( !ptTimeout.checkTime() )
	{
		sqlite3_reset(stmt);
		int rc = sqlite3_step(stmt);
		if(  rc == result || rc == SQLITE_DONE )
			return true;

		msleep(opCheckPause);
	}

	return false;
}
// -----------------------------------------------------------------------------------------
const string SQLiteInterface::error()
{
	if( !db )
		return "";
	
	return sqlite3_errmsg(db);
}
// -----------------------------------------------------------------------------------------
const string SQLiteInterface::lastQuery()
{
	return lastQ;
}
// -----------------------------------------------------------------------------------------
int SQLiteInterface::insert_id()
{
	if( !db )
		return 0;

	return sqlite3_last_insert_rowid(db);
}
// -----------------------------------------------------------------------------------------
bool SQLiteInterface::isConnection()
{
	return connected;
}
// -----------------------------------------------------------------------------------------
