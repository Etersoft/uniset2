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
#ifndef DBServer_PostgreSQL_H_
#define DBServer_PostgreSQL_H_
// --------------------------------------------------------------------------
#include <unordered_map>
#include <queue>
#include "UniSetTypes.h"
#include "PostgreSQLInterface.h"
#include "DBServer.h"
//------------------------------------------------------------------------------------------
class DBServer_PostgreSQL:
	public DBServer
{
	public:
		DBServer_PostgreSQL( UniSetTypes::ObjectId id, const std::string& prefix );
		DBServer_PostgreSQL();
		virtual ~DBServer_PostgreSQL();

		/*! глобальная функция для инициализации объекта */
		static std::shared_ptr<DBServer_PostgreSQL> init_dbserver( int argc, const char* const* argv, const std::string& prefix = "pgsql" );

		/*! глобальная функция для вывода help-а */
		static void help_print( int argc, const char* const* argv );

		inline std::shared_ptr<LogAgregator> logAggregator()
		{
			return loga;
		}
		inline std::shared_ptr<DebugStream> log()
		{
			return dblog;
		}

	protected:
		typedef std::unordered_map<int, std::string> DBTableMap;

		virtual void initDBServer() override;
		virtual void initDB( std::shared_ptr<PostgreSQLInterface>& db ) {};
		virtual void initDBTableMap(DBTableMap& tblMap) {};

		virtual void timerInfo( const UniSetTypes::TimerMessage* tm ) override;
		virtual void sysCommand( const UniSetTypes::SystemMessage* sm ) override;
		virtual void sensorInfo( const UniSetTypes::SensorMessage* sm ) override;
		virtual void confirmInfo( const UniSetTypes::ConfirmMessage* cmsg ) override;
		virtual void sigterm( int signo ) override;

		bool writeToBase( const string& query );
		void createTables( std::shared_ptr<PostgreSQLInterface>& db );

		inline std::string tblName(int key)
		{
			return tblMap[key];
		}

		enum Timers
		{
			PingTimer,        /*!< таймер на переодическую проверку соединения  с сервером БД */
			ReconnectTimer,   /*!< таймер на повторную попытку соединения с сервером БД (или восстановления связи) */
			lastNumberOfTimer
		};

		std::shared_ptr<PostgreSQLInterface> db;
		int PingTime;
		int ReconnectTime;
		bool connect_ok;     /*! признак наличия соеднинения с сервером БД */

		bool activate;

		typedef std::queue<std::string> QueryBuffer;

		QueryBuffer qbuf;
		unsigned int qbufSize; // размер буфера сообщений.
		bool lastRemove;

		void flushBuffer();
		UniSetTypes::uniset_mutex mqbuf;

	private:
		DBTableMap tblMap;

};
//------------------------------------------------------------------------------------------
#endif
