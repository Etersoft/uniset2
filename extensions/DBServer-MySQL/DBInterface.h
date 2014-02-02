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
#ifndef DBInterface_H_
#define DBInterface_H_
// ---------------------------------------------------------------------------
#include <string>
#include <iostream>
//#warning Для использования mysql_create нужен define USE_OLD_FUNCTIONS
//#define USE_OLD_FUNCTIONS
#include <mysql/mysql.h>
// ----------------------------------------------------------------------------
class DBInterface
{
    public:

            DBInterface();
            ~DBInterface();

//            bool createDB(const std::string dbname);
//            bool dropDB(const std::string dbname);
            MYSQL_RES * listFields(const std::string& table, const std::string& wild );


            bool connect( const std::string& host, const std::string& user, const std::string& pswd,
                            const std::string& dbname);
            bool close();

            bool query(const std::string& q);
            const std::string lastQuery();
            bool insert(const std::string& q);

            std::string addslashes(const std::string& str);

            /*! 
                проверка связи с БД.
                в случае отсутсвия попытка восстановить...
            */
            bool ping();

            /*! связь с БД установлена (была) */
            bool isConnection();

            bool nextRecord();
            void freeResult();

            unsigned int numCols();
            unsigned int numRows();

            bool moveToRow(int ind);

            int insert_id();

            const MYSQL_ROW getRow();
            const std::string error();

            MYSQL_ROW Row;

            // *******************
            const char* gethostinfo();
    protected:

    private:

        MYSQL_RES *result;
        MYSQL *mysql;
        std::string lastQ;
        bool queryok;    // успешность текущего запроса
        bool connected;
};
// ----------------------------------------------------------------------------------
#endif
