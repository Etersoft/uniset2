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
#include <cstdio>
#include "UniSetTypes.h"
#include "SQLiteInterface.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
// --------------------------------------------------------------------------

SQLiteInterface::SQLiteInterface():
db(0),
curStmt(0),
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
	if( db )
		delete db;
}

// -----------------------------------------------------------------------------------------
bool SQLiteInterface::connect( const string dbfile )
{
	int rc = sqlite3_open(dbfile.c_str(), &db);

	if( rc==SQLITE_BUSY || rc==SQLITE_LOCKED || rc==SQLITE_INTERRUPT || rc==SQLITE_IOERR )
	{
		cerr << "SQLiteInterface::connect): rc=" << rc << " error: " << sqlite3_errmsg(db) << endl;
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
	if( db )
	{
		sqlite3_close(db);
		db = 0;
	}
	
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
	curStmt = pStmt;
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
	
	// sqlite3_finalize(pStmt);
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
void SQLiteInterface::freeResult()
{
	sqlite3_finalize(curStmt);
	curStmt = 0;
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
SQLiteInterface::iterator SQLiteInterface::begin()
{
	return SQLiteIterator(curStmt);
}
// -----------------------------------------------------------------------------------------
SQLiteInterface::iterator SQLiteInterface::end()
{
	return SQLiteIterator();
}
// -----------------------------------------------------------------------------------------
SQLiteInterface::SQLiteIterator SQLiteInterface::SQLiteIterator::operator ++(int c)
{
	if( row == -1 || c<0 )
		return *this;

	if( c==0 )
		c = 1;

	for( int i=0; i<c; i++ )
	{
		int rc = sqlite3_step(stmt);
		// cerr << "**** ++:  rc=" << rc  << " err: " << sqlite3_errmsg( sqlite3_db_handle(stmt) ) << endl;
		if( rc != SQLITE_ROW )
		{
			row = -1;
			break;
		}

		row++;
	}
	
	return *this;
}
// -----------------------------------------------------------------------------------------
#if 0
SQLiteInterface::SQLiteIterator SQLiteInterface::SQLiteIterator::operator --()
{
	
}
#endif
// -----------------------------------------------------------------------------------------
std::string SQLiteInterface::SQLiteIterator::get_text( int col )
{
	return string( (char*)sqlite3_column_text(stmt,col) );
}
// -----------------------------------------------------------------------------------------
int SQLiteInterface::SQLiteIterator::get_int( int col )
{
	return sqlite3_column_int(stmt,col);
}
// -----------------------------------------------------------------------------------------
double SQLiteInterface::SQLiteIterator::get_double( int col )
{
	return sqlite3_column_double(stmt,col);
}
// -----------------------------------------------------------------------------------------
int SQLiteInterface::SQLiteIterator::get_num_cols()
{
	return sqlite3_data_count(stmt);
}
// -----------------------------------------------------------------------------------------
bool SQLiteInterface::SQLiteIterator::is_end()
{
	return ( row == -1 );
}
// -----------------------------------------------------------------------------------------
void SQLiteInterface::SQLiteIterator::free_result()
{
	sqlite3_finalize(stmt);
	stmt = 0;
}
// -----------------------------------------------------------------------------------------
