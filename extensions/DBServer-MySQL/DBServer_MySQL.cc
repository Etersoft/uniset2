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
 *  \brief файл реализации DB-сервера
 *  \author Pavel Vainerman
*/
// --------------------------------------------------------------------------
#include <sys/time.h>
#include <sstream>
#include <iomanip>
#include <cmath>

#include "ORepHelpers.h"
#include "DBServer_MySQL.h"
#include "Configuration.h"
#include "Debug.h"
#include "UniXML.h"
// --------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
// --------------------------------------------------------------------------
#define dblog if( ulog()->debugging(DBLogInfoLevel) ) (*(ulog().get()))[DBLogInfoLevel]
// --------------------------------------------------------------------------
DBServer_MySQL::DBServer_MySQL(ObjectId id):
	DBServer(id),
	db(new MySQLInterface()),
	PingTime(300000),
	ReconnectTime(180000),
	connect_ok(false),
	activate(true),
	qbufSize(200),
	lastRemove(false)
{
	if( getId() == DefaultObjectId )
	{
		ostringstream msg;
		msg << "(DBServer_MySQL): init failed! Unknown ID!" << endl;
		throw Exception(msg.str());
	}

	mqbuf.setName(myname  + "_qbufMutex");
}

DBServer_MySQL::DBServer_MySQL():
	DBServer(uniset_conf()->getDBServer()),
	db(new MySQLInterface()),
	PingTime(300000),
	ReconnectTime(180000),
	connect_ok(false),
	activate(true),
	qbufSize(200),
	lastRemove(false)
{
	//    init();
	if( getId() == DefaultObjectId )
	{
		ostringstream msg;
		msg << "(DBServer_MySQL): init failed! Unknown ID!" << endl;
		throw Exception(msg.str());
	}

	mqbuf.setName(myname  + "_qbufMutex");
}
//--------------------------------------------------------------------------------------------
DBServer_MySQL::~DBServer_MySQL()
{
	if( db != NULL )
	{
		db->close();
		delete db;
	}
}
//--------------------------------------------------------------------------------------------
void DBServer_MySQL::sysCommand( const UniSetTypes::SystemMessage* sm )
{
	switch( sm->command )
	{
		case SystemMessage::StartUp:
			break;

		case SystemMessage::Finish:
		{
			activate = false;
			db->close();
		}
		break;

		case SystemMessage::FoldUp:
		{
			activate = false;
			db->close();
		}
		break;

		default:
			break;
	}
}

//--------------------------------------------------------------------------------------------
void DBServer_MySQL::confirmInfo( const UniSetTypes::ConfirmMessage* cem )
{
	try
	{
		ostringstream data;

		data << "UPDATE " << tblName(cem->type)
			 << " SET confirm='" << cem->confirm << "'"
			 << " WHERE sensor_id='" << cem->sensor_id << "'"
			 << " AND date='" << dateToString(cem->time, "-") << " '"
			 << " AND time='" << timeToString(cem->time, ":") << " '"
			 << " AND time_usec='" << cem->time_usec << " '";

		dblog << myname << "(update_confirm): " << data.str() << endl;

		if( !writeToBase(data.str()) )
		{
			ucrit << myname << "(update_confirm):  db error: " << db->error() << endl;
		}
	}
	catch( const Exception& ex )
	{
		ucrit << myname << "(update_confirm): " << ex << endl;
	}
	catch( const std::exception& ex )
	{
		ucrit << myname << "(update_confirm): exception: " << ex.what() << endl;
	}
}
//--------------------------------------------------------------------------------------------
bool DBServer_MySQL::writeToBase( const string& query )
{
	dblog << myname << "(writeToBase): " << query << endl;

	//    cout << "DBServer_MySQL: " << query << endl;
	if( !db || !connect_ok )
	{
		uniset_rwmutex_wrlock l(mqbuf);
		qbuf.push(query);

		if( qbuf.size() > qbufSize )
		{
			std::string qlost;

			if( lastRemove )
				qlost = qbuf.back();
			else
				qlost = qbuf.front();

			qbuf.pop();

			ucrit << myname << "(writeToBase): DB not connected! buffer(" << qbufSize
				  << ") overflow! lost query: " << qlost << endl;
		}

		return false;
	}

	// На всякий скидываем очередь
	flushBuffer();

	// А теперь собственно запрос..
	db->query(query);

	// Дело в том что на INSERT И UPDATE запросы
	// db->query() может возвращать false и надо самому
	// отдельно проверять действительно ли произошла ошибка
	// см. MySQLInterface::query.
	string err(db->error());

	if( err.empty() )
		return true;

	return false;
}
//--------------------------------------------------------------------------------------------
void DBServer_MySQL::flushBuffer()
{
	uniset_rwmutex_wrlock l(mqbuf);

	// Сперва пробуем очистить всё что накопилось в очереди до этого...
	while( !qbuf.empty() )
	{
		db->query( qbuf.front() );

		// Дело в том что на INSERT И UPDATE запросы
		// db->query() может возвращать false и надо самому
		// отдельно проверять действительно ли произошла ошибка
		// см. MySQLInterface::query.
		string err(db->error());

		if( !err.empty() )
			ucrit << myname << "(writeToBase): error: " << err <<
				  " lost query: " << qbuf.front() << endl;

		qbuf.pop();
	}
}
//--------------------------------------------------------------------------------------------
void DBServer_MySQL::sensorInfo( const UniSetTypes::SensorMessage* si )
{
	try
	{
		// если время не было выставлено (указываем время сохранения в БД)
		if( !si->tm.tv_sec )
		{
			struct timezone tz;
			gettimeofday( const_cast<struct timeval*>(&si->tm), &tz);
		}

		float val = (float)si->value / (float)pow10(si->ci.precision);

		// см. DBTABLE AnalogSensors, DigitalSensors
		ostringstream data;
		data << "INSERT INTO " << tblName(si->type)
			 << "(date, time, time_usec, sensor_id, value, node) VALUES( '"
			 // Поля таблицы
			 << dateToString(si->sm_tv_sec, "-") << "','"   //  date
			 << timeToString(si->sm_tv_sec, ":") << "','"   //  time
			 << si->sm_tv_usec << "','"                //  time_usec
			 << si->id << "','"                    //  sensor_id
			 << val << "','"                //  value
			 << si->node << "')";                //  node

		dblog << myname << "(insert_main_history): " << data.str() << endl;

		if( !writeToBase(data.str()) )
		{
			ucrit << myname <<  "(insert) sensor msg error: " << db->error() << endl;
		}
	}
	catch( const Exception& ex )
	{
		ucrit << myname << "(insert_main_history): " << ex << endl;
	}
	catch( const std::exception& ex )
	{
		ucrit << myname << "(insert_main_history): catch: " << ex.what() << endl;
	}
}
//--------------------------------------------------------------------------------------------
void DBServer_MySQL::init_dbserver()
{
	DBServer::init_dbserver();
	dblog << myname << "(init): ..." << endl;

	if( connect_ok )
	{
		initDBTableMap(tblMap);
		initDB(db);
		return;
	}

	auto conf = uniset_conf();

	if( conf->getDBServer() == UniSetTypes::DefaultObjectId )
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

	dblog << myname << "(init): init connection.." << endl;
	string dbname(conf->getProp(node, "dbname"));
	string dbnode(conf->getProp(node, "dbnode"));
	string user(conf->getProp(node, "dbuser"));
	string password(conf->getProp(node, "dbpass"));

	tblMap[UniSetTypes::Message::SensorInfo] = "main_history";
	tblMap[UniSetTypes::Message::Confirm] = "main_history";

	PingTime = conf->getIntProp(node, "pingTime");
	ReconnectTime = conf->getIntProp(node, "reconnectTime");
	qbufSize = conf->getArgPInt("--dbserver-buffer-size", it.getProp("bufferSize"), 200);

	if( findArgParam("--dbserver-buffer-last-remove", conf->getArgc(), conf->getArgv()) != -1 )
		lastRemove = true;
	else if( it.getIntProp("bufferLastRemove" ) != 0 )
		lastRemove = true;
	else
		lastRemove = false;

	if( dbnode.empty() )
		dbnode = "localhost";

	dblog << myname << "(init): connect dbnode=" << dbnode
		  << "\tdbname=" << dbname
		  << " pingTime=" << PingTime
		  << " ReconnectTime=" << ReconnectTime << endl;

	if( !db->connect(dbnode, user, password, dbname) )
	{
		//        ostringstream err;
		ucrit << myname
			  << "(init): DB connection error: "
			  << db->error() << endl;
		//        throw Exception( string(myname+"(init): не смогли создать соединение с БД "+db->error()) );
		askTimer(DBServer_MySQL::ReconnectTimer, ReconnectTime);
	}
	else
	{
		dblog << myname << "(init): connect [OK]" << endl;
		connect_ok = true;
		askTimer(DBServer_MySQL::ReconnectTimer, 0);
		askTimer(DBServer_MySQL::PingTimer, PingTime);
		//        createTables(db);
		initDB(db);
		initDBTableMap(tblMap);
		flushBuffer();
	}
}
//--------------------------------------------------------------------------------------------
void DBServer_MySQL::createTables( MySQLInterface* db )
{
	auto conf = uniset_conf();

	UniXML::iterator it( conf->getNode("Tables") );

	if(!it)
	{
		ucrit << myname << ": section <Tables> not found.." << endl;
		throw Exception();
	}

	for( it.goChildren(); it; it.goNext() )
	{
		if( it.getName() != "comment" )
		{
			dblog << myname  << "(createTables): create " << it.getName() << endl;
			ostringstream query;
			query << "CREATE TABLE " << conf->getProp(it, "name") << "(" << conf->getProp(it, "create") << ")";

			if( !db->query(query.str()) )
				ucrit << myname << "(createTables): error: \t\t" << db->error() << endl;
		}
	}
}
//--------------------------------------------------------------------------------------------
void DBServer_MySQL::timerInfo( const UniSetTypes::TimerMessage* tm )
{
	switch( tm->id )
	{
		case DBServer_MySQL::PingTimer:
		{
			if( !db->ping() )
			{
				uwarn << myname << "(timerInfo): DB lost connection.." << endl;
				connect_ok = false;
				askTimer(DBServer_MySQL::PingTimer, 0);
				askTimer(DBServer_MySQL::ReconnectTimer, ReconnectTime);
			}
			else
			{
				connect_ok = true;
				dblog << myname << "(timerInfo): DB ping ok" << endl;
			}
		}
		break;

		case DBServer_MySQL::ReconnectTimer:
		{
			dblog << myname << "(timerInfo): reconnect timer" << endl;

			if( db->isConnection() )
			{
				if( db->ping() )
				{
					connect_ok = true;
					askTimer(DBServer_MySQL::ReconnectTimer, 0);
					askTimer(DBServer_MySQL::PingTimer, PingTime);
				}

				connect_ok = false;
				uwarn << myname << "(timerInfo): DB no connection.." << endl;
			}
			else
				init_dbserver();
		}
		break;

		default:
			uwarn << myname << "(timerInfo): Unknown TimerID=" << tm->id << endl;
			break;
	}
}
//--------------------------------------------------------------------------------------------
