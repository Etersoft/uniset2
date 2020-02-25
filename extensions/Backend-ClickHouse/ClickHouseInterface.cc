/*
 * Copyright (c) 2020 Pavel Vainerman.
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
#include <sstream>
#include <cstdio>
#include "unisetstd.h"
#include <UniSetTypes.h>
#include "ClickHouseInterface.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// --------------------------------------------------------------------------

ClickHouseInterface::ClickHouseInterface():
	lastQ(""),
	lastE("")
{
}

ClickHouseInterface::~ClickHouseInterface()
{
	try
	{
		close();
	}
	catch( ... ) // пропускаем все необработанные исключения, если требуется обработать нужно вызывать close() до деструктора
	{
		cerr << "ClickHouseInterface::~ClickHouseInterface(): an error occured while closing connection!" << endl;
	}
}

// -----------------------------------------------------------------------------------------
bool ClickHouseInterface::ping() const
{
	if( !db )
		return false;

	try
	{
		db->Ping();
		return true;
	}
	catch( const std::exception& e )
	{
//		lastE = string(e.what());
	}

	return false;

//	return db && db->is_open();
}
// -----------------------------------------------------------------------------------------
bool ClickHouseInterface::reconnect(const string& host, const string& user, const string& pswd, const string& dbname, unsigned int port )
{
	if( db )
		close();

	return nconnect(host,user,pswd, dbname, port);
}
// -----------------------------------------------------------------------------------------
bool ClickHouseInterface::nconnect(const string& host, const string& user, const string& pswd, const string& dbname, unsigned int port )
{
	if( db )
		return true;

	auto opts = clickhouse::ClientOptions().SetHost(host).SetPort(port).SetPassword(pswd);

	if( !user.empty() )
		opts.SetUser(user);

	opts.SetSendRetries(2);
	opts.SetPingBeforeQuery(true);

	try
	{
		db = unisetstd::make_unique<clickhouse::Client>(opts);
		db->Ping();
		return true;
	}
	catch( const std::exception& e )
	{
		cerr << "connect " << opts
			 << " ERROR: " << e.what() << std::endl;
	}

	db = nullptr;

	return false;
}
// -----------------------------------------------------------------------------------------
bool ClickHouseInterface::close()
{
	if( db )
		db.reset();

	return true;
}
// -----------------------------------------------------------------------------------------
bool ClickHouseInterface::insert( const std::string& tblname, const clickhouse::Block& data )
{
	if( !db )
	{
		lastE = "no connection";
		return false;
	}

	try
	{
		db->Insert(tblname, data);
		return true;
	}
	catch( const clickhouse::ServerException& e )
	{
		lastE = string(e.what());
	}
	catch( const std::exception& e )
	{
		//cerr << e.what() << std::endl;
		lastE = string(e.what());
	}

	return false;
}

// -----------------------------------------------------------------------------------------
DBResult ClickHouseInterface::makeResult( const clickhouse::Block& block )
{
	DBResult result;

	for( clickhouse::Block::Iterator it(block); it.IsValid(); it.Next() )
	{
		DBResult::COL col;
	}

//	for( size_t i = 0; i < block.GetRowCount(); ++i) {
//		DBResult::COL col;
//		for(

//		std::cout << block[0]->As<ColumnUInt64>()->At(i) << " "
//				  << block[1]->As<ColumnString>()->At(i) << "\n";
//	}


//	for( result::const_iterator c = res.begin(); c != res.end(); ++c )
//	{
//		DBResult::COL col;

//		for( pqxx::result::tuple::const_iterator i = c.begin(); i != c.end(); i++ )
//		{
//			if( i.is_null() )
//				col.push_back("");
//			else
//			{
//				result.setColName(i.num(), i.name());
//				col.push_back( i.as<string>() );
//			}
//		}

//		result.row().push_back( std::move(col) );
//	}

	return result;
}
// -----------------------------------------------------------------------------------------
bool ClickHouseInterface::execute( const std::string& q )
{
	if( !db )
		return false;

	try
	{
		db->Execute(q);
		return true;
	}
	catch( const std::exception& e )
	{
		lastE = string(e.what());
	}

	return false;
}
// -----------------------------------------------------------------------------------------
DBResult ClickHouseInterface::query( const std::string& q )
{
	if( !db )
		return DBResult();

	try
	{
		DBResult ret;
		db->Select(q, [] (const clickhouse::Block& block){
				for (size_t i = 0; i < block.GetRowCount(); ++i) {
					std::cout << block[0]->As<clickhouse::ColumnUInt64>()->At(i) << " "
							  << block[1]->As<clickhouse::ColumnString>()->At(i) << "\n";
				}
		});

		return ret;
	}
	catch( const std::exception& e )
	{
		lastE = string(e.what());
	}

	return DBResult();
}
// -----------------------------------------------------------------------------------------
const string ClickHouseInterface::error()
{
	return lastE;
}
// -----------------------------------------------------------------------------------------
const string ClickHouseInterface::lastQuery()
{
	return lastQ;
}
// -----------------------------------------------------------------------------------------
bool ClickHouseInterface::isConnection() const
{
	return db && ping();
//	return (db && db->is_open())
}
// -----------------------------------------------------------------------------------------
extern "C" std::shared_ptr<DBInterface> create_postgresqlinterface()
{
	return std::shared_ptr<DBInterface>(new ClickHouseInterface(), DBInterfaceDeleter());
}
// -----------------------------------------------------------------------------------------
