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
#include <sys/time.h>
#include <sstream>
#include <iomanip>
#include <cmath>

#include "unisetstd.h"
#include "ORepHelpers.h"
#include "BackendClickHouse.h"
#include "Configuration.h"
#include "Debug.h"
#include "UniXML.h"
#include "DBLogSugar.h"
// --------------------------------------------------------------------------
using namespace uniset;
using namespace std;
// --------------------------------------------------------------------------
BackendClickHouse::BackendClickHouse(ObjectId id, const std::string& prefix ):
	DBServer(id, prefix)
{
	db = unisetstd::make_unique<ClickHouseInterface>();

	if( getId() == DefaultObjectId )
	{
		ostringstream msg;
		msg << "(BackendClickHouse): init failed! Unknown ID!" << endl;
		throw Exception(msg.str());
	}

	createColumns();
}

BackendClickHouse::BackendClickHouse():
	DBServer(uniset_conf()->getDBServer())
{
	db = unisetstd::make_unique<ClickHouseInterface>();

	//    init();
	if( getId() == DefaultObjectId )
	{
		ostringstream msg;
		msg << "(BackendClickHouse): init failed! Unknown ID!" << endl;
		throw Exception(msg.str());
	}

	createColumns();
}
//--------------------------------------------------------------------------------------------
BackendClickHouse::~BackendClickHouse()
{
	if( db )
		db = nullptr;
}
//--------------------------------------------------------------------------------------------
void BackendClickHouse::createColumns()
{
	colTimeStamp = std::make_shared<clickhouse::ColumnDateTime>();
	colTimeUsec = std::make_shared<clickhouse::ColumnUInt64>();
	colSensorID = std::make_shared<clickhouse::ColumnUInt64>();
	colValue = std::make_shared<clickhouse::ColumnFloat64>();
	colNode = std::make_shared<clickhouse::ColumnUInt64>();
	colConfirm = std::make_shared<clickhouse::ColumnUInt64>();
	arrTagKeys = std::make_shared<clickhouse::ColumnArray>(std::make_shared<clickhouse::ColumnString>());
	arrTagValues = std::make_shared<clickhouse::ColumnArray>(std::make_shared<clickhouse::ColumnString>());
}
//--------------------------------------------------------------------------------------------
void BackendClickHouse::sysCommand( const uniset::SystemMessage* sm )
{
	DBServer::sysCommand(sm);

	switch( sm->command )
	{
		case SystemMessage::StartUp:
			askTimer(FlushInsertBuffer, ibufSyncTimeout);
			break;

		case SystemMessage::Finish:
			db->close();
		break;

		case SystemMessage::FoldUp:
			db->close();
		break;

		default:
			break;
	}
}

//--------------------------------------------------------------------------------------------
void BackendClickHouse::confirmInfo( const uniset::ConfirmMessage* cem )
{
	DBServer::confirmInfo(cem);

	try
	{
//		ostringstream data;

//		data << "UPDATE " << dbname << "." << tblName(cem->type)
//			 << " SET confirm=" << cem->confirm_time.tv_sec
//			 << " WHERE sensor_id=" << cem->sensor_id
//			 << " AND timestamp=" << cem->sensor_time.tv_sec
//			 << " AND time_usec=" << cem->sensor_time.tv_nsec;

//		dbinfo << myname << "(update_confirm): " << data.str() << endl;

//		// перед UPDATE обязательно скинуть insertBuffer
//		flushInsertBuffer();

//		if( !db->query(data.str()) )
//			dbcrit << myname << "(update_confirm):  db error: " << db->error() << endl;
	}
	catch( const uniset::Exception& ex )
	{
		dbcrit << myname << "(update_confirm): " << ex << endl;
	}
	catch( ... )
	{
		dbcrit << myname << "(update_confirm):  catch..." << endl;
	}
}
//--------------------------------------------------------------------------------------------
void BackendClickHouse::onTextMessage( const TextMessage* msg )
{
	try
	{
//		ostringstream data;
//		data << "INSERT INTO " << dbname << "." << tblName(msg->type)
//			 << "(timestamp, time_usec, text, mtype, node) VALUES("
//			 << msg->tm.tv_sec << ","   // timestamp
//			 << msg->tm.tv_nsec << ","  //  time_usec
//			 << "'" << msg->txt << "',"     // text
//			 << "'" << msg->mtype << "',"   // mtype
//			 << msg->node << ")";    //  node

//		dbinfo << myname << "(insert_main_messages): " << data.str() << endl;

//		if( !db->query(data.str()) )
//			dbcrit << myname << "(insert_main_messages): error: " << db->error() << endl;
	}
	catch( const uniset::Exception& ex )
	{
		dbcrit << myname << "(insert_main_messages): " << ex << endl;
	}
	catch( const std::exception& ex )
	{
		dbcrit << myname << "(insert_main_messages): " << ex.what() << endl;
	}
	catch( ... )
	{
		dbcrit << myname << "(insert_main_messages): catch..." << endl;
	}
}
//--------------------------------------------------------------------------------------------
void BackendClickHouse::flushInsertBuffer()
{
	if( colTimeStamp->Size() == 0 )
		return;

	if( !db || !connect_ok )
		return;

	dbinfo << myname << "(flushInsertBuffer): write insert buffer[" << colTimeStamp->Size() << "] to DB.." << endl;

	clickhouse::Block blk(8,colTimeStamp->Size());
	blk.AppendColumn("timestamp", colTimeStamp);
	blk.AppendColumn("time_usec", colTimeUsec);
	blk.AppendColumn("sensor_id", colSensorID);
	blk.AppendColumn("value", colValue);
	blk.AppendColumn("node", colNode);
	blk.AppendColumn("confirm", colConfirm);
	blk.AppendColumn("tags.name", arrTagKeys);
	blk.AppendColumn("tags.value", arrTagValues);

	if( !db->insert(fullTableName, blk) )
	{
		dbcrit << myname << "(flushInsertBuffer): error: " << db->error() << endl;
	}
	else
	{
		clearData();
	}
}
//--------------------------------------------------------------------------------------------
void BackendClickHouse::clearData()
{
	colTimeStamp->Clear();
	colTimeUsec->Clear();
	colSensorID->Clear();
	colValue->Clear();
	colNode->Clear();
	colConfirm->Clear();
	arrTagKeys->Clear();
	arrTagValues->Clear();
}
//--------------------------------------------------------------------------------------------
void BackendClickHouse::sensorInfo( const uniset::SensorMessage* si )
{
	try
	{
		if( !si->tm.tv_sec )
		{
			// Выдаём CRIT, но тем не менее сохраняем в БД

			dbcrit << myname << "(insert_main_history): UNKNOWN TIMESTAMP! (tm.tv_sec=0)"
				   << " for sid=" << si->id
				   << " supplier=" << uniset_conf()->oind->getMapName(si->supplier)
				   << endl;
		}

		colTimeStamp->Append(si->sm_tv.tv_sec);
		colTimeUsec->Append(si->sm_tv.tv_nsec);
		colSensorID->Append(si->id);
		colValue->Append(si->value);
		colNode->Append(si->node);
		colConfirm->Append(0);

		// TAGS
		auto k1 = std::make_shared<clickhouse::ColumnString>();
		k1->Append("tag1");
		arrTagKeys->AppendAsColumn(k1);

		auto v1 = std::make_shared<clickhouse::ColumnString>();
		v1->Append("val1");
		arrTagValues->AppendAsColumn(v1);

		if( colTimeStamp->Size() >= ibufMaxSize )
			flushInsertBuffer();

	}
	catch( const uniset::Exception& ex )
	{
		dbcrit << myname << "(insert_main_history): " << ex << endl;
	}
	catch( const std::exception& ex )
	{
		dbcrit << myname << "(insert_main_messages): " << ex.what() << endl;
	}

	catch( ... )
	{
		dbcrit << myname << "(insert_main_history): catch ..." << endl;
	}
}
//--------------------------------------------------------------------------------------------
void BackendClickHouse::initDBServer()
{
	DBServer::initDBServer();
	dbinfo <<  myname << "(init): ..." << endl;

	if( connect_ok )
	{
		initDBTableMap(tblMap);
		initDB(db);
		return;
	}

	auto conf = uniset_conf();

	if( conf->getDBServer() == uniset::DefaultObjectId )
	{
		ostringstream msg;
		msg << myname << "(init): DBServer OFF for this node.."
			<< " In " << conf->getConfFileName()
			<< " for this node dbserver=''";
		throw NameNotFound(msg.str());
	}

	std::string confNode = conf->getArgParam("--" + prefix + "-confnode", "LocalDBServer");
	xmlNode* node = conf->getNode(confNode);

	if( !node )
		throw NameNotFound(string(myname + "(init): section <" + confNode + "> not found.."));

	UniXML::iterator it(node);

	dbinfo <<  myname << "(init): init connection.." << endl;

	string dbname( conf->getArgParam("--" + prefix + "-dbname", it.getProp("dbname")));
	string dbnode( conf->getArgParam("--" + prefix + "-dbnode", it.getProp("dbnode")));
	string dbuser( conf->getArgParam("--" + prefix + "-dbuser", it.getProp("dbuser")));
	string dbpass( conf->getArgParam("--" + prefix + "-dbpass", it.getProp("dbpass")));
	unsigned int dbport = conf->getArgPInt("--" + prefix + "-dbport", it.getProp("dbport"), 9000);

	ibufMaxSize = conf->getArgPInt("--" + prefix + "-ibuf-maxsize", it.getProp("ibufMaxSize"), 2000);
	ibufSyncTimeout = conf->getArgPInt("--" + prefix + "-ibuf-sync-timeout", it.getProp("ibufSyncTimeout"), 15000);

	tblMap[uniset::Message::SensorInfo] = dbname + ".main_history";
	tblMap[uniset::Message::Confirm] = dbname + ".main_history";
	tblMap[uniset::Message::TextMessage] = dbname + ".main_messages";

	PingTime = conf->getArgPInt("--" + prefix + "-pingTime", it.getProp("pingTime"), PingTime);
	ReconnectTime = conf->getArgPInt("--" + prefix + "-reconnectTime", it.getProp("reconnectTime"), ReconnectTime);

	fullTableName = dbname.empty() ? "main_history" : dbname + ".main_history";

	if( dbnode.empty() )
		dbnode = "localhost";

	dbinfo <<  myname << "(init): connect dbnode=" << dbnode << ":" << dbport
		   << "\tdbname=" << dbname
		   << " pingTime=" << PingTime
		   << " ReconnectTime=" << ReconnectTime << endl;

	if( !db->reconnect(dbnode, dbuser, dbpass, dbname, dbport) )
	{
		dbwarn << myname << "(init): DB connection error: " << db->error() << endl;
		askTimer(BackendClickHouse::ReconnectTimer, ReconnectTime);
	}
	else
	{
		dbinfo <<  myname << "(init): connect [OK]" << endl;
		connect_ok = true;
		askTimer(BackendClickHouse::ReconnectTimer, 0);
		askTimer(BackendClickHouse::PingTimer, PingTime);
		//        createTables(db);
		initDB(db);
		initDBTableMap(tblMap);
		flushInsertBuffer();
	}
}
//--------------------------------------------------------------------------------------------
void BackendClickHouse::createTables( const std::unique_ptr<ClickHouseInterface>& db )
{
	auto conf = uniset_conf();

	UniXML_iterator it( conf->getNode("Tables") );

	if(!it)
	{
		dbcrit << myname << ": section <Tables> not found.." << endl;
		throw Exception();
	}

	if( !it.goChildren() )
		return;

	for( ; it; it.goNext() )
	{
		if( it.getName() != "comment" )
		{
			dbcrit << myname  << "(createTables): create " << it.getName() << endl;
			ostringstream query;
			query << "CREATE TABLE " << conf->getProp(it, "name") << "(" << conf->getProp(it, "create") << ")";
			if( !db->execute(query.str()) )
			{
				dbcrit << myname << "(createTables): error: \t\t" << db->error() << endl;
			}
		}
	}
}
//--------------------------------------------------------------------------------------------
void BackendClickHouse::timerInfo( const uniset::TimerMessage* tm )
{
	DBServer::timerInfo(tm);

	switch( tm->id )
	{
		case BackendClickHouse::PingTimer:
		{
			if( !db->ping() )
			{
				dbwarn << myname << "(timerInfo): DB lost connection.." << endl;
				connect_ok = false;
				askTimer(BackendClickHouse::PingTimer, 0);
				askTimer(BackendClickHouse::ReconnectTimer, ReconnectTime);
			}
			else
			{
				connect_ok = true;
				dbinfo <<  myname << "(timerInfo): DB ping ok" << endl;
			}
		}
		break;

		case BackendClickHouse::ReconnectTimer:
		{
			dbinfo <<  myname << "(timerInfo): reconnect timer" << endl;

			if( db->isConnection() )
			{
				if( db->ping() )
				{
					connect_ok = true;
					askTimer(BackendClickHouse::ReconnectTimer, 0);
					askTimer(BackendClickHouse::PingTimer, PingTime);
				}
				else
				{
					connect_ok = false;
					dbwarn << myname << "(timerInfo): DB no connection.." << endl;
				}
			}
			else
				initDBServer();
		}
		break;

		case FlushInsertBuffer:
		{
			dbinfo <<  myname << "(timerInfo): insert flush timer (" << colTimeStamp->Size() << " records)" << endl;
			flushInsertBuffer();
		}
		break;

		default:
			dbwarn << myname << "(timerInfo): Unknown TimerID=" << tm->id << endl;
			break;
	}
}
//--------------------------------------------------------------------------------------------
bool BackendClickHouse::deactivateObject()
{
	if( db && connect_ok )
	{
		try
		{
			flushInsertBuffer();
		}
		catch(...) {}
	}

	return DBServer::deactivateObject();
}
//--------------------------------------------------------------------------------------------
string BackendClickHouse::getMonitInfo( const string& params )
{
	ostringstream inf;

	inf << "Database: "
		<< "[ ping=" << PingTime
		<< " reconnect=" << ReconnectTime
		<< " ]" << endl
		<< "  connection: " << (connect_ok ? "OK" : "FAILED") << endl
		<< "  lastError: " << db->error() << endl;

	inf	<< "Insert buffer: "
		<< "[ ibufMaxSize=" << ibufMaxSize
		<< " ibufSize=" << colTimeStamp->Size()
		<< " ibufSyncTimeout=" << ibufSyncTimeout
		<< " ]"	<< endl;

	return inf.str();
}
//--------------------------------------------------------------------------------------------
std::shared_ptr<BackendClickHouse> BackendClickHouse::init_dbserver( int argc, const char* const* argv,
		const std::string& prefix )
{
	auto conf = uniset_conf();

	ObjectId ID = conf->getDBServer();

	string name = conf->getArgParam("--" + prefix + "-name", "");

	if( !name.empty() )
	{
		ObjectId ID = conf->getObjectID(name);

		if( ID == uniset::DefaultObjectId )
		{
			cerr << "(BackendClickHouse): Unknown ObjectID for '" << name << endl;
			return 0;
		}
	}

	uinfo << "(BackendClickHouse): name = " << name << "(" << ID << ")" << endl;
	return make_shared<BackendClickHouse>(ID, prefix);
}
// -----------------------------------------------------------------------------
void BackendClickHouse::help_print( int argc, const char* const* argv )
{
	cout << "Default: prefix='pgsql'" << endl;
	cout << "--prefix-name objectID     - ObjectID. Default: 'conf->getDBServer()'" << endl;

	cout << "Connection: " << endl;
	cout << "--prefix-dbname name   - database name" << endl;
	cout << "--prefix-dbnode host   - database host" << endl;
	cout << "--prefix-dbuser user   - database user" << endl;
	cout << "--prefix-dbpass pass   - database password" << endl;
	cout << "--prefix-dbport port   - database port. Default: 5432" << endl;

	cout << "Check connection: " << endl;
	cout << "--prefix-pingTime msec        - check connetcion time. Default: 15000 msec" << endl;
	cout << "--prefix-reconnectTime msec   - reconnect time. Default: 30000 msec " << endl;

	cout << "Insert buffer:" << endl;
	cout << "--prefix-ibuf-maxsize sz                   - INSERT-buffer size. Default: 2000" << endl;
	cout << "--prefix-ibuf-sync-timeout msec            - INSERT-buffer sync timeout. Default: 15000 msec" << endl;
	cout << "--prefix-ibuf-overflow-cleanfactor [0...1] - INSERT-buffer overflow clean factor. Default: 0.5" << endl;

	cout << "Query buffer:" << endl;
	cout << "--prefix-buffer-size sz      - The buffer in case the database is unavailable. Default: 200" << endl;
	cout << "--prefix-buffer-last-remove  - Delete the last recording buffer overflow." << endl;

	cout << DBServer::help_print() << endl;
}
// -----------------------------------------------------------------------------
bool BackendClickHouse::isConnectOk() const
{
	return connect_ok;
}
// -----------------------------------------------------------------------------
