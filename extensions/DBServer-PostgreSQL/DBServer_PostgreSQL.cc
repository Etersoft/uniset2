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
#include <sys/time.h>
#include <sstream>
#include <iomanip>
#include <cmath>

#include "unisetstd.h"
#include "ORepHelpers.h"
#include "DBServer_PostgreSQL.h"
#include "Configuration.h"
#include "Debug.h"
#include "UniXML.h"
#include "DBLogSugar.h"
// --------------------------------------------------------------------------
using namespace uniset;
using namespace std;
// --------------------------------------------------------------------------
DBServer_PostgreSQL::DBServer_PostgreSQL(ObjectId id, const std::string& prefix ):
	DBServer(id, prefix)
{
	db = unisetstd::make_unique<PostgreSQLInterface>();

	if( getId() == DefaultObjectId )
	{
		ostringstream msg;
		msg << "(DBServer_PostgreSQL): init failed! Unknown ID!" << endl;
		throw Exception(msg.str());
	}
}

DBServer_PostgreSQL::DBServer_PostgreSQL():
	DBServer(uniset_conf()->getDBServer())
{
	db = unisetstd::make_unique<PostgreSQLInterface>();

	//    init();
	if( getId() == DefaultObjectId )
	{
		ostringstream msg;
		msg << "(DBServer_PostgreSQL): init failed! Unknown ID!" << endl;
		throw Exception(msg.str());
	}
}
//--------------------------------------------------------------------------------------------
DBServer_PostgreSQL::~DBServer_PostgreSQL()
{
	if( db )
		db->close();
}
//--------------------------------------------------------------------------------------------
void DBServer_PostgreSQL::sysCommand( const uniset::SystemMessage* sm )
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
void DBServer_PostgreSQL::confirmInfo( const uniset::ConfirmMessage* cem )
{
	DBServer::confirmInfo(cem);

	try
	{
		ostringstream data;

		data << "UPDATE " << tblName(cem->type)
			 << " SET confirm='" << cem->confirm_time.tv_sec << "'"
			 << " WHERE sensor_id='" << cem->sensor_id << "'"
			 << " AND date='" << dateToString(cem->sensor_time.tv_sec, "-") << " '"
			 << " AND time='" << timeToString(cem->sensor_time.tv_sec, ":") << " '"
			 << " AND time_usec='" << cem->sensor_time.tv_nsec << " '";

		dbinfo << myname << "(update_confirm): " << data.str() << endl;

		// перед UPDATE обязательно скинуть insertBuffer
		flushInsertBuffer();

		if( !writeToBase( std::move(data.str())) )
		{
			dbcrit << myname << "(update_confirm):  db error: " << db->error() << endl;
		}
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
void DBServer_PostgreSQL::onTextMessage( const TextMessage* msg )
{
	try
	{
		ostringstream data;
		data << "INSERT INTO " << tblName(msg->type)
			 << "(date, time, time_usec, text, mtype, node) VALUES( '"
			 << dateToString(msg->tm.tv_sec, "-") << "','"   //  date
			 << timeToString(msg->tm.tv_sec, ":") << "','"   //  time
			 << msg->tm.tv_nsec << "','"                //  time_usec
			 << msg->txt << "','"                    // text
			 << msg->mtype << "','"   // mtype
			 << msg->node << "')";                //  node

		dbinfo << myname << "(insert_main_messages): " << data.str() << endl;

		if( !writeToBase(data.str()) )
		{
			dbcrit << myname << "(insert_main_messages): error: " << db->error() << endl;
		}
	}
	catch( const uniset::Exception& ex )
	{
		dbcrit << myname << "(insert_main_messages): " << ex << endl;
	}
	catch( ... )
	{
		dbcrit << myname << "(insert_main_messages): catch..." << endl;
	}
}
//--------------------------------------------------------------------------------------------
bool DBServer_PostgreSQL::writeToBase( const string& query )
{
	dbinfo << myname << "(writeToBase): " << query << endl;

	if( !db || !connect_ok )
	{
		std::lock_guard<std::mutex> l(mqbuf);
		qbuf.push(query);

		if( qbuf.size() > qbufSize )
		{
			std::string qlost;

			if( lastRemove )
				qlost = qbuf.back();
			else
				qlost = qbuf.front();

			qbuf.pop();
			dbcrit << myname << "(writeToBase): DB not connected! buffer(" << qbufSize
				   << ") overflow! lost query: " << qlost << endl;
		}

		// Надо подумать, что тут лучше возвращать true или false
		// вроде бы фактически запись не сделали, но запрос поместили в буфер
		// А вызывающая сторона может посчитать, что раз вернули false, значит
		// можно/нужно потом запрос повторить..

		return false;
	}

	// На всякий скидываем очередь
	flushBuffer();

	// А теперь собственно запрос..
	if( db->insert(query) )
		return true;

	return false;
}
//--------------------------------------------------------------------------------------------
void DBServer_PostgreSQL::flushBuffer()
{
	std::lock_guard<std::mutex> l(mqbuf);

	// Сперва пробуем очистить всё что накопилось в очереди до этого...
	while( !qbuf.empty() )
	{
		if( !db->insert(qbuf.front()) )
		{
			dbcrit << myname << "(writeToBase): error: " << db->error() << " lost query: " << qbuf.front() << endl;
		}

		qbuf.pop();
	}
}
//--------------------------------------------------------------------------------------------
void DBServer_PostgreSQL::flushInsertBuffer()
{
	if( ibufSize > ibufMaxSize )
	{
		dbcrit << myname << "(flushWriteBuffer): "
			   << " buffer[" << ibufSize << "] overflow! LOST DATA..." << endl;

		// Чистим заданное число
		size_t delnum = lroundf(ibufSize * ibufOverflowCleanFactor);
		auto end = ibuf.end();
		auto beg = ibuf.begin();

		// Удаляем последние (новые)
		if( lastRemove )
		{
			end = std::prev(end, delnum);
		}
		else
		{
			// Удаляем первые (старые)
			beg = std::next(beg, delnum);
		}

		if( beg != ibuf.end() )
			ibuf.erase(beg, end);

		// ibufSize - беззнаковое, так что надо аккуратно
		ibufSize = (delnum < ibufSize) ? (ibufSize - delnum) : 0;

		dbwarn << myname << "(flushInsertBuffer): overflow: clear data " << delnum << " records." << endl;
	}

	if( ibufSize == 0 )
		return;

	if( !db || !connect_ok )
		return;

	dbinfo << myname << "(flushInsertBuffer): write insert buffer[" << ibufSize << "] to DB.." << endl;

	if( !writeInsertBufferToDB("main_history", tblcols, ibuf) )
	{
		dbcrit << myname << "(flushInsertBuffer): error: " << db->error() << endl;
	}
	else
	{
		ibuf.clear();
		ibufSize = 0;
	}
}
//--------------------------------------------------------------------------------------------
void DBServer_PostgreSQL::addRecord( const PostgreSQLInterface::Record&& rec )
{
	ibuf.emplace_back( std::move(rec) );
	ibufSize++;

	if( ibufSize >= ibufMaxSize )
		flushInsertBuffer();
}
//--------------------------------------------------------------------------------------------
bool DBServer_PostgreSQL::writeInsertBufferToDB( const std::string& tableName
		, const std::vector<std::string>& colNames
		, const InsertBuffer& wbuf )
{
	return db->copy(tableName, colNames, wbuf);
}
//--------------------------------------------------------------------------------------------
void DBServer_PostgreSQL::sensorInfo( const uniset::SensorMessage* si )
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

		// (date, time, time_usec, sensor_id, value, node)
		PostgreSQLInterface::Record rec =
		{
			dateToString(si->sm_tv.tv_sec, "-"), //  date
			timeToString(si->sm_tv.tv_sec, ":"), //  time
			std::to_string(si->sm_tv.tv_nsec),
			std::to_string(si->id),
			std::to_string(si->value),
			std::to_string(si->node),
		};

		addRecord( std::move(rec) );
	}
	catch( const uniset::Exception& ex )
	{
		dbcrit << myname << "(insert_main_history): " << ex << endl;
	}
	catch( ... )
	{
		dbcrit << myname << "(insert_main_history): catch ..." << endl;
	}
}
//--------------------------------------------------------------------------------------------
void DBServer_PostgreSQL::initDBServer()
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

	xmlNode* node = conf->getNode("LocalDBServer");

	if( !node )
		throw NameNotFound(string(myname + "(init): section <LocalDBServer> not found.."));

	UniXML::iterator it(node);

	dbinfo <<  myname << "(init): init connection.." << endl;

	string dbname( conf->getArgParam("--" + prefix + "-dbname", it.getProp("dbname")));
	string dbnode( conf->getArgParam("--" + prefix + "-dbnode", it.getProp("dbnode")));
	string dbuser( conf->getArgParam("--" + prefix + "-dbuser", it.getProp("dbuser")));
	string dbpass( conf->getArgParam("--" + prefix + "-dbpass", it.getProp("dbpass")));
	unsigned int dbport = conf->getArgPInt("--" + prefix + "-dbport", it.getProp("dbport"), 5432);

	ibufMaxSize = conf->getArgPInt("--" + prefix + "-ibuf-maxsize", it.getProp("ibufMaxSize"), 2000);
	ibuf.reserve(ibufMaxSize);

	ibufSyncTimeout = conf->getArgPInt("--" + prefix + "-ibuf-sync-timeout", it.getProp("ibufSyncTimeout"), 15000);
	std::string sfactor = conf->getArg2Param("--" + prefix + "-ibuf-overflow-cleanfactor", it.getProp("ibufOverflowCleanFactor"), "0.5");
	ibufOverflowCleanFactor = atof(sfactor.c_str());

	tblMap[uniset::Message::SensorInfo] = "main_history";
	tblMap[uniset::Message::Confirm] = "main_history";
	tblMap[uniset::Message::TextMessage] = "main_messages";

	PingTime = conf->getArgPInt("--" + prefix + "-pingTime", it.getProp("pingTime"), PingTime);
	ReconnectTime = conf->getArgPInt("--" + prefix + "-reconnectTime", it.getProp("reconnectTime"), ReconnectTime);

	qbufSize = conf->getArgPInt("--" + prefix + "-buffer-size", it.getProp("bufferSize"), qbufSize);

	if( findArgParam("--" + prefix + "-buffer-last-remove", conf->getArgc(), conf->getArgv()) != -1 )
		lastRemove = true;
	else if( it.getIntProp("bufferLastRemove" ) != 0 )
		lastRemove = true;
	else
		lastRemove = false;

	if( dbnode.empty() )
		dbnode = "localhost";

	dbinfo <<  myname << "(init): connect dbnode=" << dbnode
		   << "\tdbname=" << dbname
		   << " pingTime=" << PingTime
		   << " ReconnectTime=" << ReconnectTime << endl;

	if( !db->nconnect(dbnode, dbuser, dbpass, dbname, dbport) )
	{
		dbwarn << myname << "(init): DB connection error: " << db->error() << endl;
		askTimer(DBServer_PostgreSQL::ReconnectTimer, ReconnectTime);
	}
	else
	{
		dbinfo <<  myname << "(init): connect [OK]" << endl;
		connect_ok = true;
		askTimer(DBServer_PostgreSQL::ReconnectTimer, 0);
		askTimer(DBServer_PostgreSQL::PingTimer, PingTime);
		//        createTables(db);
		initDB(db);
		initDBTableMap(tblMap);
		flushBuffer();
	}
}
//--------------------------------------------------------------------------------------------
void DBServer_PostgreSQL::createTables( const std::shared_ptr<PostgreSQLInterface>& db )
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

			if( !db->query(query.str()) )
			{
				dbcrit << myname << "(createTables): error: \t\t" << db->error() << endl;
			}
		}
	}
}
//--------------------------------------------------------------------------------------------
void DBServer_PostgreSQL::timerInfo( const uniset::TimerMessage* tm )
{
	DBServer::timerInfo(tm);

	switch( tm->id )
	{
		case DBServer_PostgreSQL::PingTimer:
		{
			if( !db->ping() )
			{
				dbwarn << myname << "(timerInfo): DB lost connection.." << endl;
				connect_ok = false;
				askTimer(DBServer_PostgreSQL::PingTimer, 0);
				askTimer(DBServer_PostgreSQL::ReconnectTimer, ReconnectTime);
			}
			else
			{
				connect_ok = true;
				dbinfo <<  myname << "(timerInfo): DB ping ok" << endl;
			}
		}
		break;

		case DBServer_PostgreSQL::ReconnectTimer:
		{
			dbinfo <<  myname << "(timerInfo): reconnect timer" << endl;

			if( db->isConnection() )
			{
				if( db->ping() )
				{
					connect_ok = true;
					askTimer(DBServer_PostgreSQL::ReconnectTimer, 0);
					askTimer(DBServer_PostgreSQL::PingTimer, PingTime);
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
			dbinfo <<  myname << "(timerInfo): insert flush timer.." << endl;
			flushInsertBuffer();
		}
		break;

		default:
			dbwarn << myname << "(timerInfo): Unknown TimerID=" << tm->id << endl;
			break;
	}
}
//--------------------------------------------------------------------------------------------
bool DBServer_PostgreSQL::deactivateObject()
{
	if( db && connect_ok )
	{
		try
		{
			flushBuffer();
		}
		catch(...) {}
	}

	return DBServer::deactivateObject();
}
//--------------------------------------------------------------------------------------------
string DBServer_PostgreSQL::getMonitInfo( const string& params )
{
	ostringstream inf;

	inf << "Database: "
		<< "[ ping=" << PingTime
		<< " reconnect=" << ReconnectTime
		<< " qbufSize=" << qbufSize
		<< " ]" << endl
		<< "  connection: " << (connect_ok ? "OK" : "FAILED") << endl;
	{
		std::lock_guard<std::mutex> lock(mqbuf);
		inf << " buffer size: " << qbuf.size() << endl;
	}
	inf << "   lastError: " << db->error() << endl;

	inf	<< "Insert buffer: "
		<< "[ ibufMaxSize=" << ibufMaxSize
		<< " ibufSize=" << ibufSize
		<< " ibufSyncTimeout=" << ibufSyncTimeout
		<< " ]"	<< endl;

	return inf.str();
}
//--------------------------------------------------------------------------------------------
std::shared_ptr<DBServer_PostgreSQL> DBServer_PostgreSQL::init_dbserver( int argc, const char* const* argv,
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
			cerr << "(DBServer_PostgreSQL): Unknown ObjectID for '" << name << endl;
			return 0;
		}
	}

	uinfo << "(DBServer_PostgreSQL): name = " << name << "(" << ID << ")" << endl;
	return make_shared<DBServer_PostgreSQL>(ID, prefix);
}
// -----------------------------------------------------------------------------
void DBServer_PostgreSQL::help_print( int argc, const char* const* argv )
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
bool DBServer_PostgreSQL::isConnectOk() const
{
	return connect_ok;
}
// -----------------------------------------------------------------------------
