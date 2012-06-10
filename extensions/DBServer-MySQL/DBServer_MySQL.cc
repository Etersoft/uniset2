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

#include "ORepHelpers.h"
#include "DBServer_MySQL.h"
#include "Configuration.h"
#include "Debug.h"
#include "UniXML.h"
// --------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
// --------------------------------------------------------------------------

DBServer_MySQL::DBServer_MySQL(ObjectId id):
	DBServer(id),
	db(new DBInterface()),
	PingTime(300000),
	ReconnectTime(180000),
	connect_ok(false),
	activate(true),
	lastRemove(false)
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

void DBServer_MySQL::parse( UniSetTypes::InfoMessage* im )
{
	string message(im->message);
	if( message.empty() && im->infocode != DefaultMessageCode  )
		message = conf->mi->getMessage(im->infocode);

	if( !message.empty() )
		message = db->addslashes(message);

	// Прежде чем формировать строку обязательно смотрите формат базы данных(порядок полей таблицы)!!!
	ostringstream ostr;
	ostr << "INSERT INTO " << tblName(im->type);
	ostr << "(num,node,id,date,time,time_usec,code,text,haracter,type,confirm,causeid) VALUES(";
	ostr << "NULL,'"<< im->node << "','" << im->id;
	ostr << "','" << ui.dateToString(im->tm.tv_sec,"/") << "','" << ui.timeToString(im->tm.tv_sec,":");
	ostr << "','" << im->tm.tv_usec;
	ostr << "','" << im->infocode << "','"<< message << "','" << im->character;
	ostr << "','" << im->type << "','0','0')";

	if( !writeToBase(ostr.str()) )
	{
		unideb[Debug::CRIT] << myname <<  "(insert): info msg error: "<< db->error() << endl;
//		db->freeResult();
	}

}
//--------------------------------------------------------------------------------------------
void DBServer_MySQL::parse( UniSetTypes::AlarmMessage* am )
{
	string message(am->message);
	if( message.empty() && am->alarmcode != DefaultMessageCode  )
		message = conf->mi->getMessage(am->alarmcode);

	if( !message.empty() )
		message = db->addslashes(message);

	// Прежде чем формировать строку обязательно смотрите формат базы данных(порядок полей таблицы)!!!
	ostringstream ostr;
	ostr << "INSERT INTO " << tblName(am->type);
	ostr << "(num,node,id,date,time,time_usec,code,text,haracter,type,confirm,causeid) VALUES(";
	ostr << "NULL,'"<< am->node << "','" << am->id;
	ostr << "','" << ui.dateToString(am->tm.tv_sec,"/") << "','"
			<< ui.timeToString(am->tm.tv_sec,":")<< "','" << am->tm.tv_usec;
	ostr << "','" << am->alarmcode<< "','" << message;
	ostr << "','" << am->character << "','" << am->type << "',0,'" << am->causecode << "')";

	if( !writeToBase(ostr.str()) )
	{
		unideb[Debug::CRIT] << myname <<  "(insert): alarm msg error: "<< db->error() << endl;
//		db->freeResult();
	}
}
//--------------------------------------------------------------------------------------------
void DBServer_MySQL::parse( UniSetTypes::ConfirmMessage* am )
{
	// Сохраняем ПОДТВЕРЖДЕНИЕ в базу
	ostringstream query;
	query << "UPDATE " << tblName(am->orig_type) << " SET ";
	query << "confirm='" << ui.timeToString(am->tm.tv_sec,":") << "'";
	query << " where ";
	query << " id='" << am->orig_id << "'";
	query << " AND type='" << am->orig_type << "'";
	query << " AND node='" << am->orig_node << "'";
	query << " AND code='" << am->code << "'";
//	query << " AND cause='" << am->cause << "'";
	query << " AND date='" << ui.dateToString(am->orig_tm.tv_sec,"/") << "'";
	query << " AND time='" << ui.timeToString(am->orig_tm.tv_sec,":") << "'";
	query << " AND time_usec='" << am->orig_tm.tv_usec << "'";

	if( !writeToBase(query.str()) )
	{
		unideb[Debug::CRIT] << myname <<  "(insert): confirm msg error: "<< db->error() << endl;
//		db->freeResult();
	}
}
//--------------------------------------------------------------------------------------------
bool DBServer_MySQL::writeToBase( const string& query )
{
	if( unideb.debugging(DBLogInfoLevel) )
		unideb[DBLogInfoLevel] << myname << "(writeToBase): " << query << endl;
//	cout << "DBServer_MySQL: " << query << endl;
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
	flushBuffer();

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
//--------------------------------------------------------------------------------------------
void DBServer_MySQL::flushBuffer()
{
	uniset_mutex_lock l(mqbuf,400);

	// Сперва пробуем очистить всё что накопилось в очереди до этого...
	while( !qbuf.empty() )
	{
		db->query( qbuf.front() );

		// Дело в том что на INSERT И UPDATE запросы
		// db->query() может возвращать false и надо самому
		// отдельно проверять действительно ли произошла ошибка
		// см. DBInterface::query.
		string err(db->error());
		if( err.empty() )
			db->freeResult();
		else if( unideb.debugging(Debug::CRIT) )
		{
			unideb[Debug::CRIT] << myname << "(writeToBase): error: " << err <<
				" lost query: " << qbuf.front() << endl;
		}

		qbuf.pop();
	}
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

		// см. DBTABLE AnalogSensors, DigitalSensors
		ostringstream data;
		data << " VALUES( ";
												// Поля таблицы
		data << "NULL,'"<< si->node << "','";		// num, node
		data << si->id << "','";					// id (sensorid)
		data << ui.dateToString(si->sm_tv_sec,"/") << "','";	// date
		data << ui.timeToString(si->sm_tv_sec,":") << "','";	// time
		data << si->sm_tv_usec << "','";					// time_usec

//		data << ui.dateToString(si->tm.tv_sec) << "','";	// date
//		data << ui.timeToString(si->tm.tv_sec) << "','";	// time
//		data << si->tm.tv_usec << "','";					// time_usec


		string table;
		switch( si->sensor_type )
		{
			case UniversalIO::DigitalInput:
			case UniversalIO::DigitalOutput:
				table = "DigitalSensors(num,node,id,date,time,time_usec,state)";
				data << si->state;				// state
				break;

			case UniversalIO::AnalogInput:
			case UniversalIO::AnalogOutput:
				table = "AnalogSensors(num,node,id,date,time,time_usec,value)";
				data << si->value;				// value
				break;

			default:
				unideb[Debug::WARN] << myname << "(log sensor): Unknown iotype='"
						<< si->sensor_type << "'.. ignore SensorMessage..." << endl;
				return;
		}

		data << "')";

		if( !writeToBase("INSERT INTO "+table+data.str()) )
		{
			if( unideb.debugging(Debug::CRIT) )
				unideb[Debug::CRIT] << myname <<  "(insert) sensor msg error: "<< db->error() << endl;
			db->freeResult();
		}
	}
	catch( Exception& ex )
	{
		if( unideb.debugging(Debug::CRIT) )
			unideb[Debug::CRIT] << myname << "(parse SensorMessage): " << ex << endl;
	}
	catch( ...  )
	{
		if( unideb.debugging(Debug::CRIT) )
			unideb[Debug::CRIT] << myname << "(parse SensorMessage): catch..." << endl;
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

	tblMap[UniSetTypes::Message::Info] = "Messages";
	tblMap[UniSetTypes::Message::Alarm] = "Messages";
	tblMap[UniSetTypes::Message::SensorInfo] = "AnalogSensors";

	PingTime = conf->getIntProp(node,"pingTime");
	ReconnectTime = conf->getIntProp(node,"reconnectTime");
	qbufSize = conf->getArgPInt("--dbserver-buffer-size",it.getProp("bufferSize"),200);

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
	}
	else
	{
		if( unideb.debugging(DBLogInfoLevel) )
			unideb[DBLogInfoLevel] << myname << "(init): connect [OK]" << endl;
		connect_ok = true;
		askTimer(DBServer_MySQL::ReconnectTimer,0);
		askTimer(DBServer_MySQL::PingTimer,PingTime);
//		createTables(db);
		initDB(db);
		initDBTableMap(tblMap);	
		flushBuffer();
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
				}
				connect_ok = false;
				if( unideb.debugging(Debug::WARN) )
					unideb[Debug::WARN] << myname << "(timerInfo): DB no connection.." << endl;
			}
			else
				init_dbserver();
		}
		break;

		default:
			if( unideb.debugging(Debug::WARN) )
				unideb[Debug::WARN] << myname << "(timerInfo): Unknown TimerID=" << tm->id << endl;
		break;
	}
}
//--------------------------------------------------------------------------------------------
