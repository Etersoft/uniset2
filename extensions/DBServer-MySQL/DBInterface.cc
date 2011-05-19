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
#include "DBInterface.h"
using namespace std;

// -----------------------------------------------------------------------------------------

DBInterface::DBInterface():
result(0),
lastQ(""),
queryok(false)
{ 
	mysql = new MYSQL();
	mysql_init(mysql);
//	mysql_options(mysql,MYSQL_READ_DEFAULT_GROUP,"your_prog_name");
	mysql_options(mysql,MYSQL_OPT_COMPRESS,0);
}

DBInterface::~DBInterface()
{ 
	close();
	delete mysql;
}

// -----------------------------------------------------------------------------------------
bool DBInterface::connect( const string host, const string user, const string pswd, const string dbname)
{
	if (!mysql_real_connect(mysql,host.c_str(), user.c_str(),pswd.c_str(),dbname.c_str(),0,NULL,0))
	{
		cout << error() << endl;
		mysql_close(mysql);
		return false;
	}
	
	return true;
}
// -----------------------------------------------------------------------------------------
bool DBInterface::close()
{
	mysql_close(mysql);
	return true;
}
// -----------------------------------------------------------------------------------------			
bool DBInterface::insert(const string q)
{
	if( !mysql )
		return false;

	if( mysql_query(mysql,q.c_str()) )
	{
		queryok=false;
		return false;
	}
	
	queryok=true;
	return true;
}
// -----------------------------------------------------------------------------------------			
bool DBInterface::query(const string q)
{
	if( !mysql )
		return false;

	if( mysql_query(mysql,q.c_str()) )
	{
		queryok=false;
		return false;
	}

		
	lastQ = q;
	result = mysql_store_result(mysql); // _use_result - некорректно работает с _num_rows
	if( numRows()==0 )
	{
		queryok=false;
		return false;
	}

	queryok=true;	
	return true;
}
// -----------------------------------------------------------------------------------------
bool DBInterface::nextRecord()
{
	if( !mysql || !result || !queryok )
		return false;

	Row = mysql_fetch_row(result);
	if( Row )
		return true;

	return false;
}

// -----------------------------------------------------------------------------------------
const string DBInterface::error()
{
	return mysql_error(mysql);
}
// -----------------------------------------------------------------------------------------
const string DBInterface::lastQuery()
{
	return lastQ;
}
// -----------------------------------------------------------------------------------------
void DBInterface::freeResult()
{
	if( !mysql || !result || !queryok )
		return;

	queryok = false;
	mysql_free_result( result );
}
// -----------------------------------------------------------------------------------------
int DBInterface::insert_id()
{
	if( !mysql )
		return 0;

	return mysql_insert_id(mysql);
}
// -----------------------------------------------------------------------------------------
unsigned int DBInterface::numCols()
{
	if( result )
		return mysql_num_fields(result);
	return 0;
}

// -----------------------------------------------------------------------------------------			
unsigned int DBInterface::numRows()
{
	if( result )
		return mysql_num_rows(result);
	return 0;
}
// -----------------------------------------------------------------------------------------			
const MYSQL_ROW DBInterface::getRow()
{
	return Row;
}

// -----------------------------------------------------------------------------------------			
const char* DBInterface::gethostinfo()
{
	return mysql_get_host_info(mysql);
}
// -----------------------------------------------------------------------------------------			
/*
bool DBInterface::createDB(const string dbname)
{
	if(!mysql)
		return false;
	if( mysql_create_db(mysql, dbname.c_str()) )
		return true;
	return false;
}
// -----------------------------------------------------------------------------------------			

bool DBInterface::dropDB(const string dbname)
{
	if( mysql_drop_db(mysql, dbname.c_str()))
		return true;
	return false;
}
*/
// -----------------------------------------------------------------------------------------			
MYSQL_RES* DBInterface::listFields(const string table, const string wild )
{
	if( !mysql || !result )
		return false;

	MYSQL_RES *res = mysql_list_fields(mysql, table.c_str(),wild.c_str());
	unsigned int cols = mysql_num_fields(result); // numCols();

	MYSQL_ROW row=mysql_fetch_row(res);
//	MYSQL_FIELD *field = mysql_fetch_fields(res);
//	cout << field << " | ";
	for( unsigned int i = 0; i<cols; i++)
	{	
		cout << row[i] << " | ";
	}

	return  res; // mysql_list_fields(mysql, table,wild);
}
// -----------------------------------------------------------------------------------------			
bool DBInterface::moveToRow(int ind)
{
	if(!mysql || !result) 
		return false;	

	mysql_data_seek(result, ind);
	return true;
}
// -----------------------------------------------------------------------------------------			
bool DBInterface::ping()
{
	if(!mysql)
		return false;

	// внимание mysql_ping возвращает 0 
	// если всё хорошо.... (поэтому мы инвертируем)
	return !mysql_ping(mysql);
}
// -----------------------------------------------------------------------------------------			
bool DBInterface::isConnection()
{
	return ping(); //!mysql;
}
// -----------------------------------------------------------------------------------------			
string DBInterface::addslashes(const string& str)
{
	ostringstream tmp;
	for( unsigned int i=0; i<str.size(); i++ )
	{
//		if( !strcmp(str[i],'\'') )
		if( str[i] == '\'' )
			tmp << "\\";
		tmp << str[i];
	}
	
	return tmp.str();	
}
