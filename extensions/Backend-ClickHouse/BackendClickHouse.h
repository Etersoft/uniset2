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
#ifndef BackendClickHouse_H_
#define BackendClickHouse_H_
// --------------------------------------------------------------------------
#include <unordered_map>
#include <queue>
#include "ClickHouseInterface.h"
#include "UniSetTypes.h"
#include "DBServer.h"
// -------------------------------------------------------------------------
namespace uniset
{
	//------------------------------------------------------------------------------------------
	/*!
	 * \brief The BackendClickHouse class
	 * Реализация работы с ClickHouse.
	 *
	 * Т.к. основная работа сервера - это частая запись данных, то сделана следующая оптимизация:
	 * Создаётся insert-буфер настраиваемого размера (ibufMaxSize).
	 * Как только буфер заполняется, он пишется в БД одним "оптимизированным" запросом.
	 * Помимо этого буфер скидывается, если прошло ibufSyncTimeout мсек или если пришёл запрос
	 * на UPDATE данных.
	 *
	 * В случае если буфер переполняется (например нет связи с БД), то он чистится.
	 */
	class BackendClickHouse:
		public DBServer
	{
		public:
			BackendClickHouse( uniset::ObjectId id, const std::string& prefix );
			BackendClickHouse();
			virtual ~BackendClickHouse();

			/*! глобальная функция для инициализации объекта */
			static std::shared_ptr<BackendClickHouse> init_dbserver( int argc, const char* const* argv, const std::string& prefix = "clickhouse" );

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

			bool isConnectOk() const;

		protected:
			typedef std::unordered_map<int, std::string> DBTableMap;

			virtual void initDBServer() override;
			virtual void initDB( std::unique_ptr<ClickHouseInterface>& db ) {};
			virtual void initDBTableMap( DBTableMap& tblMap ) {};

			virtual void timerInfo( const uniset::TimerMessage* tm ) override;
			virtual void sysCommand( const uniset::SystemMessage* sm ) override;
			virtual void sensorInfo( const uniset::SensorMessage* sm ) override;
			virtual void confirmInfo( const uniset::ConfirmMessage* cmsg ) override;
			virtual void onTextMessage( const uniset::TextMessage* msg ) override;
			virtual bool deactivateObject() override;
			virtual std::string getMonitInfo( const std::string& params ) override;

			void createTables( const std::unique_ptr<ClickHouseInterface>& db );

			inline std::string tblName(int key)
			{
				return tblMap[key];
			}

			enum Timers
			{
				PingTimer,        /*!< таймер на пере одическую проверку соединения  с сервером БД */
				ReconnectTimer,   /*!< таймер на повторную попытку соединения с сервером БД (или восстановления связи) */
				FlushInsertBuffer, /*!< таймер на сброс Insert-буфера */
				lastNumberOfTimer
			};

			std::unique_ptr<ClickHouseInterface> db;

			void flushInsertBuffer();

		private:
			DBTableMap tblMap;

			int PingTime = { 15000 };
			int ReconnectTime = { 30000 };

			bool connect_ok = { false }; /*! признак наличия соединения с сервером БД */

//			clickhouse::Block qbuf;
//			size_t qbufSize = { 200 }; // размер буфера сообщений.
//			bool lastRemove = { false };
//			std::mutex mqbuf;

			std::shared_ptr<clickhouse::ColumnDateTime> colTimeStamp;
			std::shared_ptr<clickhouse::ColumnUInt64> colTimeUsec;
			std::shared_ptr<clickhouse::ColumnUInt64> colSensorID;
			std::shared_ptr<clickhouse::ColumnFloat64> colValue;
			std::shared_ptr<clickhouse::ColumnUInt64> colNode;
			std::shared_ptr<clickhouse::ColumnUInt64> colConfirm;
			std::shared_ptr<clickhouse::ColumnArray> arrTagKeys;
			std::shared_ptr<clickhouse::ColumnArray> arrTagValues;
			void createColumns();
			void clearData();

			std::string fullTableName;
			size_t ibufMaxSize = { 2000 };
			timeout_t ibufSyncTimeout = { 15000 };
	};
	// ----------------------------------------------------------------------------------
} // end of namespace uniset
//------------------------------------------------------------------------------------------
#endif
