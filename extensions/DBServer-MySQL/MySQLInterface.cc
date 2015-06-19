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
#include "MySQLInterface.h"
using namespace std;
using namespace UniSetTypes;
// -----------------------------------------------------------------------------------------

MySQLInterface::MySQLInterface():
	lastQ(""),
	connected(false)
{
	mysql = new MYSQL();
	mysql_init(mysql);
	//    mysql_options(mysql,MYSQL_READ_DEFAULT_GROUP,"your_prog_name");
	mysql_options(mysql, MYSQL_OPT_COMPRESS, 0);
}

MySQLInterface::~MySQLInterface()
{
	close();
	delete mysql;
}

// -----------------------------------------------------------------------------------------
bool MySQLInterface::connect( const string& host, const string& user, const string& pswd, const string& dbname)
{
	if( !mysql_real_connect(mysql, host.c_str(), user.c_str(), pswd.c_str(), dbname.c_str(), 0, NULL, 0) )
	{
		cout << error() << endl;
		mysql_close(mysql);
		connected = false;
		return false;
	}

	connected = true;
	return true;
}
// -----------------------------------------------------------------------------------------
bool MySQLInterface::close()
{
	mysql_close(mysql);
	return true;
}
// -----------------------------------------------------------------------------------------
bool MySQLInterface::insert( const string& q )
{
	if( !mysql )
		return false;

	if( mysql_query(mysql, q.c_str()) )
		return false;

	return true;
}
// -----------------------------------------------------------------------------------------
MySQLResult MySQLInterface::query( const std::string& q )
{
	if( !mysql )
		return MySQLResult();

	if( mysql_query(mysql, q.c_str()) )
	{
		cerr << error() << endl;
		return MySQLResult();
	}

	lastQ = q;
	MYSQL_RES* res = mysql_store_result(mysql); // _use_result - некорректно работает с _num_rows

	if( !res || mysql_num_rows(res) == 0 )
		return MySQLResult();

	return MySQLResult(res, true);
}
// -----------------------------------------------------------------------------------------
bool MySQLInterface::query_ok( const string& q )
{
	if( !mysql )
		return false;

	if( mysql_query(mysql, q.c_str()) )
		return false;

	lastQ = q;
	MYSQL_RES* res = mysql_store_result(mysql); // _use_result - некорректно работает с _num_rows

	if( !res || mysql_num_rows(res) == 0 )
	{
		if( res )
			mysql_free_result(res);

		return false;
	}

	mysql_free_result(res);
	return true;
}
// -----------------------------------------------------------------------------------------
const string MySQLInterface::error()
{
	return mysql_error(mysql);
}
// -----------------------------------------------------------------------------------------
const string MySQLInterface::lastQuery()
{
	return lastQ;
}
// -----------------------------------------------------------------------------------------
int MySQLInterface::insert_id()
{
	if( !mysql )
		return 0;

	return mysql_insert_id(mysql);
}
// -----------------------------------------------------------------------------------------
const char* MySQLInterface::gethostinfo()
{
	return mysql_get_host_info(mysql);
}
// -----------------------------------------------------------------------------------------
bool MySQLInterface::ping()
{
	if( !mysql || !connected )
		return false;

	// внимание mysql_ping возвращает 0
	// если всё хорошо.... (поэтому мы инвертируем)
	return !mysql_ping(mysql);
}
// -----------------------------------------------------------------------------------------
bool MySQLInterface::isConnection()
{
	return ping(); //!mysql;
}
// -----------------------------------------------------------------------------------------
string MySQLInterface::addslashes( const string& str )
{
	ostringstream tmp;

	for( unsigned int i = 0; i < str.size(); i++ )
	{
		//        if( !strcmp(str[i],'\'') )
		if( str[i] == '\'' )
			tmp << "\\";

		tmp << str[i];
	}

	return tmp.str();
}
// -----------------------------------------------------------------------------------------
int MySQLResult::num_cols( const MySQLResult::iterator& it )
{
	return it->size();
}
// -----------------------------------------------------------------------------------------
int MySQLResult::as_int( const MySQLResult::iterator& it, int col )
{
	//    if( col<0 || col >it->size() )
	//        return 0;
	return uni_atoi( (*it)[col] );
}
// -----------------------------------------------------------------------------------------
double as_double( const MySQLResult::iterator& it, int col )
{
	return atof( ((*it)[col]).c_str() );
}
// -----------------------------------------------------------------------------------------
string MySQLResult::as_string( const MySQLResult::iterator& it, int col )
{
	return ((*it)[col]);
}
// -----------------------------------------------------------------------------------------
int MySQLResult::as_int( const MySQLResult::COL::iterator& it )
{
	return uni_atoi( (*it) );
}
// -----------------------------------------------------------------------------------------
double MySQLResult::as_double( const MySQLResult::COL::iterator& it )
{
	return atof( (*it).c_str() );
}
// -----------------------------------------------------------------------------------------
std::string MySQLResult::as_string( const MySQLResult::COL::iterator& it )
{
	return (*it);
}
// -----------------------------------------------------------------------------------------
#if 0
MySQLResult::COL get_col( MySQLResult::ROW::iterator& it )
{
	return (*it);
}
#endif
// -----------------------------------------------------------------------------------------
MySQLResult::~MySQLResult()
{

}
// -----------------------------------------------------------------------------------------
MySQLResult::MySQLResult( MYSQL_RES* myres, bool finalize )
{
	MYSQL_ROW row;
	unsigned int nfields = mysql_num_fields(myres);

	while( (row = mysql_fetch_row(myres)) )
	{
		COL c;

		for( unsigned int i = 0; i < nfields; i++ )
			c.push_back( (row[i] != 0 ? string(row[i]) : "") );

		res.push_back(c);
	}

	if( finalize )
		mysql_free_result(myres);
}
// -----------------------------------------------------------------------------------------
