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
// --------------------------------------------------------------------------
#include <sstream>
#include "UniSetTypes.h"
#include "MySQLInterface.h"
using namespace std;
using namespace uniset;
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
	try
	{
		close();
	}
	catch( ... ) // пропускаем все необработанные исключения, если требуется обработать нужно вызывать close() до деструктора
	{
		cerr << "MySQLInterface::~MySQLInterface(): an error occured while closing connection!" << endl;
	}

	delete mysql;
}

// -----------------------------------------------------------------------------------------
bool MySQLInterface::nconnect(const string& host, const string& user, const string& pswd, const string& dbname, unsigned int port )
{
	if( !mysql_real_connect(mysql, host.c_str(), user.c_str(), pswd.c_str(), dbname.c_str(), port, NULL, 0) )
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
DBResult MySQLInterface::query( const std::string& q )
{
	if( !mysql )
		return DBResult();

	if( mysql_query(mysql, q.c_str()) )
	{
		cerr << error() << endl;
		return DBResult();
	}

	lastQ = q;
	MYSQL_RES* res = mysql_store_result(mysql); // _use_result - некорректно работает с _num_rows

	if( !res || mysql_num_rows(res) == 0 )
		return DBResult();

	return makeResult(res, true);
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
double MySQLInterface::insert_id()
{
	if( !mysql )
		return 0;

	return mysql_insert_id(mysql);
}
// -----------------------------------------------------------------------------------------
const char* MySQLInterface::gethostinfo() const
{
	return mysql_get_host_info(mysql);
}
// -----------------------------------------------------------------------------------------
bool MySQLInterface::ping() const
{
	if( !mysql || !connected )
		return false;

	// внимание mysql_ping возвращает 0
	// если всё хорошо.... (поэтому мы инвертируем)
	return !mysql_ping(mysql);
}
// -----------------------------------------------------------------------------------------
bool MySQLInterface::isConnection() const
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
DBResult MySQLInterface::makeResult( MYSQL_RES* myres, bool finalize )
{
	DBResult result;

	if( !myres )
	{
		if( finalize )
			mysql_free_result(myres);

		return result;
	}

	MYSQL_ROW mysql_row;
	unsigned int nfields = mysql_num_fields(myres);

	while( (mysql_row = mysql_fetch_row(myres)) )
	{
		DBResult::COL c;

		for( unsigned int i = 0; i < nfields; i++ )
		{
			MYSQL_FIELD* field_info = mysql_fetch_field_direct(myres, i);
			result.setColName( i, std::string(field_info->name) );

			c.emplace_back( (mysql_row[i] != 0 ? string(mysql_row[i]) : "") );
		}

		result.row().emplace_back(c);
	}

	if( finalize )
		mysql_free_result(myres);

	return result;
}
// -----------------------------------------------------------------------------------------
extern "C" std::shared_ptr<DBInterface> create_mysqlinterface()
{
	return std::shared_ptr<DBInterface>(new MySQLInterface(), DBInterfaceDeleter());
}
// -----------------------------------------------------------------------------------------
