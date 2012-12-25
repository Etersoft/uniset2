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
#include <iostream>
#include <sqlite3.h>
#include "PassiveTimer.h"
// ----------------------------------------------------------------------------
class SQLiteInterface
{
	public:

		SQLiteInterface();
		~SQLiteInterface();

		bool connect( const std::string dbfile );
		bool close();

		inline void setOperationTimeout( timeout_t msec ){ opTimeout = msec; }
		inline timeout_t getOperationTimeout(){ return opTimeout; }

		inline void setOperationCheckPause( timeout_t msec ){ opCheckPause = msec; }
		inline timeout_t getOperationCheckPause(){ return opCheckPause; }

		bool query( const std::string q );
		const std::string lastQuery();
		bool insert( const std::string q );

		bool isConnection();

		int insert_id();

		const std::string error();

		void freeResult();

		class SQLiteIterator
		{
			public:
				SQLiteIterator( sqlite3_stmt* s ):stmt(s),row(0){}
				SQLiteIterator():stmt(0),row(-1){}
				~SQLiteIterator(){};

				SQLiteIterator operator ++(int);
				// SQLiteIterator operator --();
			inline bool operator==(const SQLiteIterator& other) const
                {
					if( row == -1 && other.stmt==0 && other.row == -1 )
						return true;

					return ( stmt == other.stmt && row == other.row );
                }

			inline bool operator!=(const SQLiteIterator& other) const
                {
					return !operator==(other);
                }

				inline int row_num(){ return row; }

				std::string get_text( int col );
				int get_int( int col );
				double get_double( int col );

				int get_num_cols();

				void free_result();

				bool is_end();

			protected:
				sqlite3_stmt* stmt;
				int row;
		};


		typedef SQLiteIterator iterator;

		iterator begin();
		iterator end();

	protected:

		bool wait( sqlite3_stmt* stmt, int result );

	private:

		sqlite3* db;
		sqlite3_stmt* curStmt;

		std::string lastQ;
		bool queryok;	// успешность текущего запроса
		bool connected;

		timeout_t opTimeout;
		timeout_t opCheckPause;
};
// ----------------------------------------------------------------------------------
#endif
// ----------------------------------------------------------------------------------
