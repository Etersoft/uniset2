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
#include <iostream>
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
#define CASE_TYPE(type, i) \
case clickhouse::Type::type: \
{ \
	auto c = col->As<clickhouse::Column##type>(); \
	if( c ) \
	{ \
		s << c->At(i); \
	} \
}\

static std::string ColumnAsString( clickhouse::ColumnRef col, size_t idx )
{
	ostringstream s;

	switch(col->Type()->GetCode())
	{
		CASE_TYPE(Int8, idx)
		CASE_TYPE(Int16, idx)
		CASE_TYPE(Int32, idx)
		CASE_TYPE(Int64, idx)
		CASE_TYPE(UInt8, idx)
		CASE_TYPE(UInt16, idx)
		CASE_TYPE(UInt32, idx)
		CASE_TYPE(UInt64, idx)
		CASE_TYPE(Float32, idx)
		CASE_TYPE(Float64, idx)
		CASE_TYPE(String, idx)
		CASE_TYPE(FixedString, idx)
//		CASE_TYPE(Array, idx)
//		CASE_TYPE(Nullable, idx)
//		CASE_TYPE(Tuple, idx)

		case clickhouse::Type::Enum8:
		{
			s << col->As<clickhouse::ColumnEnum8>()->NameAt(idx);
		}

		case clickhouse::Type::Enum16:
		{
			s << col->As<clickhouse::ColumnEnum16>()->NameAt(idx);
		}

		// as number
		CASE_TYPE(DateTime, idx)
		CASE_TYPE(Date, idx)

//		case clickhouse::Type::DateTime:
//		{
//			std::time_t t = col->As<clickhouse::ColumnDateTime>()->At(idx);
//			s << std::asctime(std::localtime(&t));
//		}

//		case clickhouse::Type::Date:
//		{
//			std::time_t t = col->As<clickhouse::ColumnDate>()->At(idx);
//			s << std::asctime(std::localtime(&t));
//		}

		case clickhouse::Type::UUID:
		{
			auto v = col->As<clickhouse::ColumnUUID>()->At(idx);
			s << v.first << v.second;
		}

		case clickhouse::Type::Void:
		{

		}
	}

	return s.str();
}
// -----------------------------------------------------------------------------------------
DBResult ClickHouseInterface::makeResult( const clickhouse::Block& block )
{
	DBResult result;
	appendResult(result, block);
	return result;
}
// -----------------------------------------------------------------------------------------
void ClickHouseInterface::appendResult( DBResult& result, const clickhouse::Block& block )
{
	for( size_t c=0; c < block.GetColumnCount(); ++c )
		result.setColName(c, block.GetColumnName(c));

	for( size_t r=0; r < block.GetRowCount(); ++r )
	{
		DBResult::COL col;
		for( size_t c=0; c < block.GetColumnCount(); ++c )
			col.push_back( ColumnAsString(block[c], r));

		result.row().push_back( std::move(col) );
	}
}
// -----------------------------------------------------------------------------------------
DBResult ClickHouseInterface::query( const std::string& q )
{
	if( !db )
		return DBResult();

	try
	{
		DBResult ret;

		db->Select(q, [this, &ret] (const clickhouse::Block& block){
			appendResult(ret, block);
		});

		lastQ = q;
		return ret;
	}
	catch( const std::exception& e )
	{
		lastE = string(e.what());
	}

	return DBResult();
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
const std::vector<clickhouse::Block> ClickHouseInterface::bquery( const std::string& q )
{
	std::vector<clickhouse::Block> ret;

	if( !db )
		return ret;

	try
	{
		db->Select(q, [this, &ret] (const clickhouse::Block& block){
			ret.push_back(block);
		});

		lastQ = q;
		return ret;
	}
	catch( const std::exception& e )
	{
		lastE = string(e.what());
	}

	return ret;
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
