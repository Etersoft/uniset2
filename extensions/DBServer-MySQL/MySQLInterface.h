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
// ----------------------------------------------------------------------------
class MySQLResult;
// ----------------------------------------------------------------------------
class MySQLInterface
{
    public:

            MySQLInterface();
            ~MySQLInterface();

//            MySQLResult listFields( const std::string& table, const std::string& wild );

            bool connect( const std::string& host, const std::string& user, const std::string& pswd,
                            const std::string& dbname);
            bool close();

            bool query_ok( const std::string& q );

            // \param finalize - освободить буфер после запроса
            MySQLResult query( const std::string& q );

            const std::string lastQuery();
            bool insert( const std::string& q );

            std::string addslashes(const std::string& str);

            /*! 
                проверка связи с БД.
                в случае отсутсвия попытка восстановить...
            */
            bool ping();

            /*! связь с БД установлена (была) */
            bool isConnection();

            int insert_id();

            const std::string error();

            // *******************
            const char* gethostinfo();
    protected:

    private:

        MYSQL *mysql;
        std::string lastQ;
        bool connected;
};
// ----------------------------------------------------------------------------------
class MySQLResult
{
    public:
        MySQLResult(){}
        MySQLResult( MYSQL_RES* r, bool finalize=true );
        ~MySQLResult();

        typedef std::vector<std::string> COL;
        typedef std::deque<COL> ROW;

        typedef ROW::iterator iterator;

        inline iterator begin(){ return res.begin(); }
        inline iterator end(){ return res.end(); }

        inline operator bool(){ return !res.empty(); }

        inline size_t size(){ return res.size(); }
        inline bool empty(){ return res.empty(); }

    protected:

        ROW res;
};
// ----------------------------------------------------------------------------------
int num_cols( MySQLResult::iterator& );
// ROW
int as_int( MySQLResult::iterator&, int col );
double as_double( MySQLResult::iterator&, int col );
std::string as_text( MySQLResult::iterator&, int col );
// ----------------------------------------------------------------------------
// COL
int as_int( MySQLResult::COL::iterator& );
double as_double( MySQLResult::COL::iterator& );
std::string as_string( MySQLResult::COL::iterator& );
// ----------------------------------------------------------------------------
#endif
