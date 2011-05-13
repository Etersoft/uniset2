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
#ifndef DBServer_MySQL_H_
#define DBServer_MySQL_H_
// --------------------------------------------------------------------------
#include <map>
#include "UniSetTypes.h"
#include "DBInterface.h"
#include "DBServer.h"
//------------------------------------------------------------------------------------------
/*!
      \page page_DBServer_MySQL (DBServer_MySQL) Реализация сервиса ведения БД на основе MySQL
  
      - \ref sec_DBS_Comm
      - \ref sec_DBS_Conf
      - \ref sec_DBS_Tables


	\section sec_DBS_Comm Общее описание работы DBServer_MySQL
	    Сервис предназначен для работы с БД MySQL. В его задачи входит
	сохранение всех событий происходищих в системе в БД. К этим
	событиям относятся изменение состояния датчиков, различные логи 
	работы процессов и т.п.
	   К моменту запуска, подразумевается, что неободимые таблицы уже 
	созданы, все необходимые настройки mysql сделаны.
	\par
	При работе с БД, сервис в основном пишет в БД. Обработка накопленных данных
	ведётся уже другими программами (web-интерфейс).
	
	\par
		Для повышения надежности DBServer переодически ( DBServer_MySQL::PingTimer ) проверяет наличие связи с сервером БД.
	В случае если связь пропала (или не была установлена при старте) DBServer пытается вновь переодически ( DBServer::ReconnectTimer )
	произвести соединение.	При этом все запросы которые поступают для запии в БД, пишутся в лог-файл.
	\warning При каждой попытке восстановить соединение DBServer заново читает конф. файл. Поэтому он может подхватить
	новые настройки.

	\todo Может не сохранять текст, если задан код... (для экономии в БД)
	
	\section sec_DBS_Conf Настройка DBServer
	    Объект DBServer берёт настройки из конфигурационного файла из секции \b<LocalDBServer>.
	Возможно задать следующие параметры:
	
	- \b dbname - название БД
	- \b dbnode - узел БД
	- \b dbuser - пользователь 
	- \b dbpass - пароль для доступа к БД
	- \b pingTime - период проверки связи с сервером MySQL
	- \b reconnectTime - время повторной попытки соединения с БД
	
	
	\section sec_DBS_Tables Таблицы MySQL
	  К основным таблицам относятся следующие:
\code

DROP TABLE IF EXISTS ObjectsMap;
CREATE TABLE ObjectsMap (
  name varchar(80) NOT NULL default '',
  rep_name varchar(80) default NULL,
  id int(4) NOT NULL default '0',
  msg int(1) default 0,
  PRIMARY KEY  (id),
  KEY rep_name (rep_name),
  KEY msg (msg)
) TYPE=MyISAM;


DROP TABLE IF EXISTS AnalogSensors;
CREATE TABLE AnalogSensors (
  num int(11) NOT NULL auto_increment,
  node int(3) default NULL,
  id int(4) default NULL,
  date date NOT NULL default '0000-00-00',
  time time NOT NULL default '00:00:00',
  time_usec int(3) unsigned default '0',
  value int(6) default NULL,
  PRIMARY KEY  (num),
  KEY date (date,time,time_usec),
  KEY node (node,id)
) TYPE=MyISAM;


--
-- Table structure for table `DigitalSensors`
--
DROP TABLE IF EXISTS DigitalSensors;
CREATE TABLE DigitalSensors (
  num int(11) NOT NULL auto_increment,
  node int(3) default NULL,
  id int(4) default NULL,
  date date NOT NULL default '0000-00-00',
  time time NOT NULL default '00:00:00',
  time_usec int(3) unsigned default '0',
  state char(1) default NULL,
  confirm time NOT NULL default '00:00:00',
  PRIMARY KEY  (num),
  KEY date (date,time,time_usec),
  KEY node (node,id),
  KEY confirm(confirm)
) TYPE=MyISAM;


DROP TABLE IF EXISTS SensorsThreshold;
CREATE TABLE SensorsThreshold (
  sid int(11) NOT NULL default '0',
  alarm int(8) NOT NULL default '0',
  warning int(8) NOT NULL default '0'
) TYPE=MyISAM;

\endcode
	
*/
class DBServer_MySQL: 
	public DBServer
{
	public:
		DBServer_MySQL( UniSetTypes::ObjectId id );
		DBServer_MySQL();
		~DBServer_MySQL();

	protected:
		typedef std::map<int, std::string> DBTableMap;

		virtual void initDB(DBInterface *db){};
		virtual void initDBTableMap(DBTableMap& tblMap){};

		virtual void processingMessage( UniSetTypes::VoidMessage *msg );
		virtual void timerInfo( UniSetTypes::TimerMessage* tm );
		virtual void sysCommand( UniSetTypes::SystemMessage* sm );

		// Функции обработки пришедших сообщений
		virtual void parse( UniSetTypes::SensorMessage* sm );
		virtual void parse( UniSetTypes::DBMessage* dbmsg );
		virtual void parse( UniSetTypes::InfoMessage* imsg );
		virtual void parse( UniSetTypes::AlarmMessage* amsg );
		virtual void parse( UniSetTypes::ConfirmMessage* cmsg );

		bool writeToBase( const string& query );
		virtual void init_dbserver();
		void createTables( DBInterface* db );
		
		inline const char* tblName(int key)
		{
			return tblMap[key].c_str();
		}

		enum Timers
		{
			PingTimer,  	/*!< таймер на переодическую проверку соединения  с сервером БД */
			ReconnectTimer 	/*!< таймер на повторную попытку соединения с сервером БД (или восстановления связи) */
		};


		DBInterface *db;
		int PingTime;
		int ReconnectTime;
		bool connect_ok; 	/*! признак наличия соеднинения с сервером БД */

		bool activate;

	private:
		DBTableMap tblMap;

};
//------------------------------------------------------------------------------------------
#endif
