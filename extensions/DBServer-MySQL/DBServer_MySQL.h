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
#include <queue>
#include "UniSetTypes.h"
#include "MySQLInterface.h"
#include "DBServer.h"
//------------------------------------------------------------------------------------------
/*!
      \page page_DBServer_MySQL (DBServer_MySQL) Реализация сервиса ведения БД на основе MySQL
  
      - \ref sec_DBS_Comm
      - \ref sec_DBS_Conf
      - \ref sec_DBS_Tables
      - \ref sec_DBS_Buffer


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
    В случае если связь пропала (или не была установлена при старте) DBServer пытается вновь каждые DBServer::ReconnectTimer
    произвести соединение.    При этом все запросы которые поступают для запии в БД, но не мгут быть записаны складываются
    в буфер (см. \ref sec_DBS_Buffer).
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
    
    \section sec_DBS_Buffer Защита от потери данных
     Для того, чтобы на момент отсутствия связи с БД данные по возможности не потерялись,
    сделан "кольцевой" буфер. Размер которго можно регулировать параметром "--dbserver-buffer-size"
    или параметром \b bufferSize=".." в конфигурационном файле секции "<LocalDBSErver...>".

    Механизм построен на том, что если связь с mysql сервером отсутствует или пропала,
    то сообщения помещаются в колевой буфер, который "опустошается" как только она восстановится.
    Если связь не восстановилась, а буфер достиг максимального заданного размера, то удаляются
    более ранние сообщения. Эту логику можно сменить, если указать параметр "--dbserver-buffer-last-remove" 
    или \b bufferLastRemove="1", то терятся будут сообщения добавляемые в конец.

    \section sec_DBS_Tables Таблицы MySQL
      К основным таблицам относятся следующие:
\code
DROP TABLE IF EXISTS `main_history`;
CREATE TABLE `main_history` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `date` date NOT NULL,
  `time` time NOT NULL,
  `time_usec` int(10) unsigned NOT NULL,
  `sensor_id` int(10) unsigned NOT NULL,
  `value` double NOT NULL,
  `node` int(10) unsigned NOT NULL,
  `confirm` int(11) DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `main_history_sensor_id` (`sensor_id`),
  CONSTRAINT `sensor_id_refs_id_3d679168` FOREIGN KEY (`sensor_id`) REFERENCES `main_sensor` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

DROP TABLE IF EXISTS `main_emergencylog`;
CREATE TABLE `main_emergencylog` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `date` date NOT NULL,
  `time` time NOT NULL,
  `time_usec` int(10) unsigned NOT NULL,
  `type_id` int(10) unsigned NOT NULL,
  PRIMARY KEY (`id`),
  KEY `main_emergencylog_type_id` (`type_id`),
  CONSTRAINT `type_id_refs_id_a3133ca` FOREIGN KEY (`type_id`) REFERENCES `main_emergencytype` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;


DROP TABLE IF EXISTS `main_emergencyrecords`;
CREATE TABLE `main_emergencyrecords` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `date` date NOT NULL,
  `time` time NOT NULL,
  `time_usec` int(10) unsigned NOT NULL,
  `log_id` int(11) NOT NULL,
  `sensor_id` int(10) unsigned NOT NULL,
  `value` double NOT NULL,
  PRIMARY KEY (`id`),
  KEY `main_emergencyrecords_log_id` (`log_id`),
  KEY `main_emergencyrecords_sensor_id` (`sensor_id`),
  CONSTRAINT `log_id_refs_id_77a37ea9` FOREIGN KEY (`log_id`) REFERENCES `main_emergencylog` (`id`),
  CONSTRAINT `sensor_id_refs_id_436bab5e` FOREIGN KEY (`sensor_id`) REFERENCES `main_sensor` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

\endcode
*/
class DBServer_MySQL: 
    public DBServer
{
    public:
        DBServer_MySQL( UniSetTypes::ObjectId id );
        DBServer_MySQL();
        ~DBServer_MySQL();

        static const Debug::type DBLogInfoLevel = Debug::LEVEL9;

    protected:
        typedef std::map<int, std::string> DBTableMap;

        virtual void initDB(MySQLInterface *db){};
        virtual void initDBTableMap(DBTableMap& tblMap){};

        virtual void timerInfo( const UniSetTypes::TimerMessage* tm ) override;
        virtual void sysCommand( const UniSetTypes::SystemMessage* sm ) override;
        virtual void sensorInfo( const UniSetTypes::SensorMessage* sm ) override;
        virtual void confirmInfo( const UniSetTypes::ConfirmMessage* cmsg ) override;

        bool writeToBase( const string& query );
        virtual void init_dbserver();
        void createTables( MySQLInterface* db );

        inline const char* tblName(int key)
        {
            return tblMap[key].c_str();
        }

        enum Timers
        {
            PingTimer,        /*!< таймер на переодическую проверку соединения  с сервером БД */
            ReconnectTimer,   /*!< таймер на повторную попытку соединения с сервером БД (или восстановления связи) */
            lastNumberOfTimer
        };


        MySQLInterface *db;
        int PingTime;
        int ReconnectTime;
        bool connect_ok;     /*! признак наличия соеднинения с сервером БД */

        bool activate;

        typedef std::queue<std::string> QueryBuffer;

        QueryBuffer qbuf;
        unsigned int qbufSize; // размер буфера сообщений.
        bool lastRemove;

        void flushBuffer();
        UniSetTypes::uniset_rwmutex mqbuf;

    private:
        DBTableMap tblMap;

};
//------------------------------------------------------------------------------------------
#endif
