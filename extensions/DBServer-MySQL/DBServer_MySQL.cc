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
const Debug::type DBLEVEL = Debug::LEVEL1;
// --------------------------------------------------------------------------
DBServer_MySQL::DBServer_MySQL(ObjectId id): 
	DBServer(id),
	db(new DBInterface()),
	PingTime(300000),
	ReconnectTime(180000),
	connect_ok(false),
	activate(true),
	lastRemove(false),
	flushBufferTime(120000),
	qbufSize(200)
{
	if( getId() == DefaultObjectId )
	{
		ostringstream msg;
		msg << "(DBServer_MySQL): init failed! Unknown ID!" << endl;
		throw Exception(msg.str());
	}
}

DBServer_MySQL::DBServer_MySQL(): 
	DBServer(conf->getDBServer()),
	db(new DBInterface()),
	PingTime(300000),
	ReconnectTime(180000),
	connect_ok(false),
	activate(true),
	lastRemove(false)
{
//	init();
	if( getId() == DefaultObjectId )
	{
		ostringstream msg;
		msg << "(DBServer_MySQL): init failed! Unknown ID!" << endl;
		throw Exception(msg.str());
	}
}
//--------------------------------------------------------------------------------------------
DBServer_MySQL::~DBServer_MySQL()
{
	if( db != NULL )
	{
		db->freeResult();
		db->close();
		delete db;
	}
}
//--------------------------------------------------------------------------------------------
void DBServer_MySQL::processingMessage( UniSetTypes::VoidMessage *msg )
{
	switch(msg->type)
	{
		case Message::Timer:
		{
			TimerMessage tm(msg);
			timerInfo(&tm);
			break;
		}
	
		default:
			DBServer::processingMessage(msg);
			break; 	
	}
}
//--------------------------------------------------------------------------------------------
void DBServer_MySQL::sysCommand( UniSetTypes::SystemMessage *sm )
{
	switch( sm->command )
	{
		case SystemMessage::StartUp:
			break;

		case SystemMessage::Finish:
		{
			activate = false;
			db->freeResult();
			db->close();
		}
		break;

		case SystemMessage::FoldUp:
		{
			activate = false;
			db->freeResult();
			db->close();
		}
		break;

		default:
			break;
	}
}

//--------------------------------------------------------------------------------------------
void DBServer_MySQL::parse( UniSetTypes::DBMessage* dbm )
{
	if( dbm->tblid == UniSetTypes::Message::Unused )
	{
		unideb[Debug::CRIT] << myname <<  "(dbmessage): не задан tblId...\n";
		return;
	}
		
	ostringstream query;
	switch( dbm->qtype )
	{
		case DBMessage::Query:
			query << dbm->data;
			break;

		case DBMessage::Update:
			query << "UPDATE " << tblName(dbm->tblid) << " SET " << dbm->data;
			break;

		case DBMessage::Insert:
			query << "INSERT INTO " << tblName(dbm->tblid) << " VALUES (" << dbm->data << ")";
			break;

	}

	if( !writeToBase(query.str()) )
	{
		unideb[Debug::CRIT] << myname <<  "(update): error: "<< db->error() << endl;
//		if( dbm->qtype == DBMessage::Query )
//			db->freeResult();
	}

}
//--------------------------------------------------------------------------------------------
void DBServer_MySQL::parse( UniSetTypes::ConfirmMessage* cem )
{
	try
	{
		ostringstream data;

		data << "UPDATE " << tblName(cem->type) 
			<< " SET confirm='" << cem->confirm << "'"
			<< " WHERE sensor_id='" << cem->sensor_id << "'"
			<< " AND date='" << ui.dateToString(cem->time, "-")<<" '"
			<< " AND time='" << ui.timeToString(cem->time, ":") <<" '"
			<< " AND time_usec='" << cem->time_usec <<" '";

		if( unideb.debugging(DBLEVEL) )
			unideb[DBLEVEL] << myname << "(update_confirm): " << data.str() << endl;

		if( !writeToBase(data.str()) )
		{
			if( unideb.debugging(Debug::CRIT) )
				unideb[Debug::CRIT] << myname << "(update_confirm):  db error: "<< db->error() << endl;
			db->freeResult();
		}
	}
	catch( Exception& ex )
	{
		if( unideb.debugging(Debug::CRIT) )
			unideb[Debug::CRIT] << myname << "(update_confirm): " << ex << endl;
	}
	catch( ... )
	{
		if( unideb.debugging(Debug::CRIT) )
			unideb[Debug::CRIT] << myname << "(update_confirm):  catch..." << endl;
	}
}
//--------------------------------------------------------------------------------------------
bool DBServer_MySQL::insertToBuffer( int key, const string& values )
{
	// Складываем в очередь
	DBTableMap::iterator it = tblMap.find(key);
	if( it == tblMap.end() )
	{
		if( unideb.debugging(DBLogInfoLevel) )
			unideb[DBLogInfoLevel] << myname << "(insertToBuffer): table key=" << key << " not found.." << endl;
		return false;
	}

	if( unideb.debugging(DBLogInfoLevel) )
		unideb[DBLogInfoLevel] << myname << "(insertToBuffer): " << it->second.tblname << " val: " << values << endl;

	TableInfo* ti = &it->second;
	
	ti->qbuf.push(values);
	ti->qbufByteSize += values.size();

	if( ti->qbuf.size() >= ti->qbufSize )
		flushTableBuffer(it);

	return true;
}
//--------------------------------------------------------------------------------------------
bool DBServer_MySQL::writeToBase( const string& query )
{
	if( unideb.debugging(DBLogInfoLevel) )
		unideb[DBLogInfoLevel] << myname << "(writeToBase): " << query << endl;

	if( !db || !connect_ok )
	{
		uniset_mutex_lock l(mqbuf,200);
		qbuf.push(query);
		if( qbuf.size() > qbufSize )
		{
			std::string qlost;
			if( lastRemove )
				qlost = qbuf.back();
			else
				qlost = qbuf.front();

			qbuf.pop();
			if( unideb.debugging(Debug::CRIT) )
				unideb[Debug::CRIT] << myname << "(writeToBase): DB not connected! buffer(" << qbufSize
						<< ") overflow! lost query: " << qlost << endl;
		}

		return false;
	}

    // На всякий скидываем очередь
    flushQBuffer();

	// А теперь собственно запрос..
	db->query(query); 

	// Дело в том что на INSERT И UPDATE запросы 
	// db->query() может возвращать false и надо самому
	// отдельно проверять действительно ли произошла ошибка
	// см. DBInterface::query.
	string err(db->error());
	if( err.empty() )
	{
		db->freeResult();
		return true;
	}
	
	return false;
}
//-------------------------------------------------------------------------------------------
void DBServer_MySQL::flushQBuffer()
{
    uniset_mutex_lock l(mqbuf);

    // Сперва пробуем очистить всё что накопилось в очереди до этого...
    while( !qbuf.empty() ) 
    {
        db->query( qbuf.front() ); 

        // Дело в том что на INSERT И UPDATE запросы 
        // db->query() может возвращать false и надо самому
        // отдельно проверять действительно ли произошла ошибка
        // см. MySQLInterface::query.
        string err(db->error());
        if( !err.empty() && unideb.debugging(Debug::CRIT) )
            unideb[Debug::CRIT] << myname << "(writeToBase): error: " << err <<
                " lost query: " << qbuf.front() << endl;

        qbuf.pop();
    }
}
//--------------------------------------------------------------------------------------------
void DBServer_MySQL::flushTableBuffer( DBTableMap::iterator& it )
{
	if( it == tblMap.end() )
		return;

	TableInfo* ti(&it->second);

	if( ti->qbuf.empty() )
		return;

	if( unideb.debugging(DBLEVEL) )
		unideb[DBLEVEL] << myname << "(flushBuffer): '" << ti->tblname << "' qbufSize=" << ti->qbufSize << " qbufByteSize=" << ti->qbufByteSize << endl;

	// buffer  чтобы записать всё одним запросом..
	// "ti->qbuf.size() - 1" - это количество добавляемых запятых..

	unsigned int cbufSize = ti->insHeader.size() + ti->qbuf.size() - 1 + ti->qbufByteSize + 1;

	char* cbuf = new char[cbufSize];
	unsigned int i= 0;

	if( !cbuf )
	{
		if( unideb.debugging(Debug::CRIT) )
			unideb[Debug::CRIT] << myname << "(flushBuffer): Can`t allocate memory for buffer (sz=" << ti->qbufByteSize + 1 << endl;
		return;
	}
	
	std::memcpy( &(cbuf[i]), ti->insHeader.c_str(), ti->insHeader.size() );
	i+=ti->insHeader.size();

	int n = 1;
	while( !ti->qbuf.empty() )
	{
		std::string q(ti->qbuf.front());
		
		unsigned int sz = q.size();
		if( i + sz  > cbufSize )
		{
			if( unideb.debugging(Debug::CRIT) )
				unideb[Debug::CRIT] << myname << "(flushBuffer): BUFFER OVERFLOW bytesize=" << cbufSize << " i=" << i << " sz=" << sz<< endl;

			break;
		}

		if( n > 1 ) // если запись не первая.. добавляем ',' в конец предыдущей записи
			cbuf[i++] = ',';
	
		std::memcpy( &(cbuf[i]), q.c_str(), sz );
		i += sz;

		ti->qbuf.pop();
		n++;
	}

	// последняя точка с запятой, вроде не нужна (по документации и примерам из MySQL)..
	if( cbuf[i-1] == ';' )
		cbuf[i-1] = '\0';
	else
		cbuf[i] = '\0';
	
	if( i < cbufSize )
	{
		if( unideb.debugging(Debug::CRIT) )
			unideb[Debug::CRIT] << myname << "(writeToBase): BAD qbufByteSize=" << ti->qbufByteSize << " LOST RECORDS..(i=" << i << ")" << endl;

		ti->qbufByteSize = 0;
		while( !ti->qbuf.empty() )
			ti->qbuf.pop();
	}

	try
	{
		if( i > 0 )
		{
			db->query( (const char*)cbuf, false );

			// Дело в том что на INSERT И UPDATE запросы 
			// db->query() может возвращать false и надо самому
			// отдельно проверять действительно ли произошла ошибка
			// см. DBInterface::queryh.
			string err(db->error());
			if( !err.empty() && unideb.debugging(Debug::CRIT) )
				unideb[Debug::CRIT] << myname << "(flushBuffer): error: " << err << endl;

			db->freeResult();
		}
	}
	catch( Exception& ex )
	{
		if( unideb.debugging(Debug::CRIT) )
			unideb[Debug::CRIT] << myname << "(flushBuffer): " << ex << endl;
	}

	delete[] cbuf;
	ti->qbufByteSize = 0;
}
//--------------------------------------------------------------------------------------------
void DBServer_MySQL::parse( UniSetTypes::SensorMessage *si )
{
	try
	{
		// если время не было выставлено (указываем время сохранения в БД)
		if( !si->tm.tv_sec )
		{
			struct timezone tz;
			gettimeofday(&si->tm,&tz);
		}
		
		float val = (float)si->value / (float)pow10(si->ci.precision);
		
		// см. DBTABLE AnalogSensors, DigitalSensors
		ostringstream data;
		data << "('"
											// Поля таблицы
			<< ui.dateToString(si->sm_tv_sec,"-") << "','"	//  date
			<< ui.timeToString(si->sm_tv_sec,":") << "','"	//  time
			<< si->sm_tv_usec << "','"				//  time_usec
			<< si->id << "','"					//  sensor_id
			<< val << "','"				//  value
			<< si->node << "')";		//  node

		if( unideb.debugging(DBLEVEL) )
			unideb[DBLEVEL] << myname << "(insert_main_history): " << data.str() << endl;

		insertToBuffer(si->type, data.str());
	}
	catch( Exception& ex )
	{
		unideb[Debug::CRIT] << myname << "(insert_main_history): " << ex << endl;
	}
	catch( ... )
	{
		unideb[Debug::CRIT] << myname << "(insert_main_history): catch ..." << endl;
	}
}
//--------------------------------------------------------------------------------------------
void DBServer_MySQL::init_dbserver()
{
	DBServer::init_dbserver();
	if( unideb.debugging(DBLogInfoLevel) )
		unideb[DBLogInfoLevel] << myname << "(init): ..." << endl;

	if( connect_ok )
	{
		initDBTableMap(tblMap);	
		initDB(db);
		return;
	}

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
		throw NameNotFound(string(myname+"(init): section <LocalDBServer> not found.."));

	UniXML::iterator it(node);

	unideb[DBLogInfoLevel] << myname << "(init): init connection.." << endl;
	string dbname(conf->getProp(node,"dbname"));
	string dbnode(conf->getProp(node,"dbnode"));
	string user(conf->getProp(node,"dbuser"));
	string password(conf->getProp(node,"dbpass"));

	PingTime = conf->getIntProp(node,"pingTime");
	ReconnectTime = conf->getIntProp(node,"reconnectTime");
	flushBufferTime = conf->getArgPInt("--dbserver-flush-buffer-time",it.getProp("flushBufferTime"),120000);

	qbufSize = conf->getArgPInt("--dbserver-lost-buffer-size",it.getProp("lostBufferSize"),200);

	int insbufSize = conf->getArgPInt("--dbserver-insert-buffer-size",it.getProp("insertBufferSize"),100000);

	TableInfo ti;
	ti.tblname = "main_history";
	ti.qbufSize = insbufSize;
	ti.qbufByteSize = 0;
	ti.insHeader = "INSERT INTO main_history(date, time, time_usec, sensor_id, value, node) VALUES";

	tblMap[UniSetTypes::Message::SensorInfo] = ti;
	tblMap[UniSetTypes::Message::Confirm] = ti;

	if( findArgParam("--dbserver-buffer-last-remove",conf->getArgc(),conf->getArgv()) != -1 )
		lastRemove = true;
	else if( it.getIntProp("bufferLastRemove" ) !=0 )
		lastRemove = true;
	else
		lastRemove = false;
	
	if( dbnode.empty() )
		dbnode = "localhost";

	if( unideb.debugging(DBLogInfoLevel) )
		unideb[DBLogInfoLevel] << myname << "(init): connect dbnode=" << dbnode
		<< "\tdbname=" << dbname
		<< " pingTime=" << PingTime 
		<< " ReconnectTime=" << ReconnectTime << endl;

	if( !db->connect(dbnode, user, password, dbname) )
	{
//		ostringstream err;
		if( unideb.debugging(Debug::CRIT) )
			unideb[Debug::CRIT] << myname
			<< "(init): DB connection error: "
			<< db->error() << endl;
//		throw Exception( string(myname+"(init): не смогли создать соединение с БД "+db->error()) );
		askTimer(DBServer_MySQL::ReconnectTimer,ReconnectTime);
		askTimer(DBServer_MySQL::FlushBufferTimer,0);
	}
	else
	{
		if( unideb.debugging(DBLogInfoLevel) )
			unideb[DBLogInfoLevel] << myname << "(init): connect [OK]" << endl;
		connect_ok = true;
		askTimer(DBServer_MySQL::ReconnectTimer,0);
		askTimer(DBServer_MySQL::PingTimer,PingTime);
		askTimer(DBServer_MySQL::FlushBufferTimer,flushBufferTime);

//		createTables(db);
		initDB(db);
		initDBTableMap(tblMap);	
		flushQBuffer();
	}
}
//--------------------------------------------------------------------------------------------
void DBServer_MySQL::createTables( DBInterface *db )
{
	UniXML_iterator it( conf->getNode("Tables") );
	if(!it)
	{
		if( unideb.debugging(Debug::CRIT) )
			unideb[Debug::CRIT] << myname << ": section <Tables> not found.."<< endl;
		throw Exception();
	}

	for( it.goChildren();it;it.goNext() )
	{
		if( it.getName() != "comment" )
		{
			if( unideb.debugging(DBLogInfoLevel) )
				unideb[DBLogInfoLevel] << myname  << "(createTables): create " << it.getName() << endl;
			ostringstream query;
			query << "CREATE TABLE " << conf->getProp(it,"name") << "(" << conf->getProp(it,"create") << ")";
			if( !db->query(query.str()) && unideb.debugging(Debug::CRIT) )
				unideb[Debug::CRIT] << myname << "(createTables): error: \t\t" << db->error() << endl;
		}
	}	
}
//--------------------------------------------------------------------------------------------
void DBServer_MySQL::timerInfo( UniSetTypes::TimerMessage* tm )
{
	switch( tm->id )
	{
		case DBServer_MySQL::PingTimer:
		{
			if( !db->ping() )
			{
				if( unideb.debugging(Debug::WARN) )
					unideb[Debug::WARN] << myname << "(timerInfo): DB lost connection.." << endl;
				connect_ok = false;
				askTimer(DBServer_MySQL::PingTimer,0);
				askTimer(DBServer_MySQL::ReconnectTimer,ReconnectTime);
				askTimer(DBServer_MySQL::FlushBufferTimer,0);
			}
			else
			{
				connect_ok = true;
				if( unideb.debugging(DBLogInfoLevel) )
					unideb[DBLogInfoLevel] << myname << "(timerInfo): DB ping ok" << endl;
			}
		}
		break;
	
		case DBServer_MySQL::ReconnectTimer:
		{
			if( unideb.debugging(DBLogInfoLevel) )
				unideb[DBLogInfoLevel] << myname << "(timerInfo): reconnect timer" << endl;
			if( db->isConnection() )
			{
				if( db->ping() )
				{
					connect_ok = true;
					askTimer(DBServer_MySQL::ReconnectTimer,0);
					askTimer(DBServer_MySQL::PingTimer,PingTime);
					askTimer(DBServer_MySQL::FlushBufferTimer,flushBufferTime);
				}
				connect_ok = false;
				if( unideb.debugging(Debug::WARN) )
					unideb[Debug::WARN] << myname << "(timerInfo): DB no connection.." << endl;
			}
			else
				init_dbserver();
		}
		break;

		case FlushBufferTimer:
		{
			for( DBTableMap::iterator it=tblMap.begin(); it!=tblMap.end(); ++it )
				flushTableBuffer(it);
		}
		break;

		default:
			if( unideb.debugging(Debug::WARN) )
				unideb[Debug::WARN] << myname << "(timerInfo): Unknown TimerID=" << tm->id << endl;
		break;
	}
}
//--------------------------------------------------------------------------------------------
