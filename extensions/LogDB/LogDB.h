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
/*! \file
 *  \author Pavel Vainerman
*/
// --------------------------------------------------------------------------
#ifndef LogDB_H_
#define LogDB_H_
// --------------------------------------------------------------------------
#include <queue>
#include <memory>
#include <chrono>
#include <ev++.h>
#include <sigc++/sigc++.h>
#include <Poco/JSON/Object.h>
#include "UniSetTypes.h"
#include "LogAgregator.h"
#include "DebugStream.h"
#include "SQLiteInterface.h"
#include "EventLoopServer.h"
#include "UTCPStream.h"
#include "LogReader.h"
#include "UHttpRequestHandler.h"
#include "UHttpServer.h"
#include "UTCPCore.h"
// -------------------------------------------------------------------------
namespace uniset
{
	//------------------------------------------------------------------------------------------
	/*!
		  \page page_LogDB База логов

		  - \ref sec_LogDB_Comm
		  - \ref sec_LogDB_Conf
		  - \ref sec_LogDB_REST


		\section sec_LogDB_Comm Общее описание работы LogDB
			LogDB это сервис, работа которого заключается
		в подключении к указанным лог-серверам, получении от них логов
		и сохранении их в БД (sqlite). Помимо этого LogDB выступает в качестве
		REST сервиса, позволяющего получать логи за указанный период в виде json.

		Реализация намеренно простая, т.к. пока неясно нужно ли это и в каком виде.
		Ожидается что контролируемых логов будет не очень много (максимум несколько десятков)
		и каждый лог будет генерировать не более 2-5 мегабайт записей. Поэтому sqlite должно хватить.

		\section sec_LogDB_Conf Конфигурирвание LogDB

		<LogDB name="LogDB" ...>
			<logserver name="" ip=".." port=".." cmd=".." description=".."/>
			<logserver name="" ip=".." port=".." cmd=".." description=".."/>
			<logserver name="" ip=".." port=".." cmd=".."/>
		</LogDB>


		\section sec_LogDB_REST LogDB REST API
			LogDB предоставляет возможность получения логов через REST API. Для этого запускается
		http-сервер. Параметры запуска можно указать при помощи:
		--prefix-httpserver-host и --prefix-httpserver-port.

		Запросы принимаются по: api/version/logdb/...

		/help                            - Получение списка доступных команд
		/list                            - список доступных логов
		/logs?logname&offset=N&limit=M   - получение логов 'logname'
										   Не обязательные параметры:
										   offset  - начиная с,
										   limit   - количество в ответе.

		/count?logname                   - Получить текущее количество записей


		\todo Добавить настройки таймаутов, размера буфера, размера для резервирования под строку, количество потоков для http и т.п.
		\todo Добавить ротацию БД
		\todo REST API: продумать команды и реализовать
		\todo Сделать настройку, для формата даты и времени при выгрузке из БД (при формировании json).
		\todo Продумать поддержку websocket
		\todo Возможно в последствии оптимизировать таблицы (нормализовать) если будет тормозить. Сейчас пока прототип.
	*/
	class LogDB:
		public EventLoopServer
#ifndef DISABLE_REST_API
		, public Poco::Net::HTTPRequestHandler
		, public Poco::Net::HTTPRequestHandlerFactory
#endif
	{
		public:
			LogDB( const std::string& name, const std::string& prefix = "" );
			virtual ~LogDB();

			/*! глобальная функция для инициализации объекта */
			static std::shared_ptr<LogDB> init_logdb( int argc, const char* const* argv, const std::string& prefix = "logdb-" );

			/*! глобальная функция для вывода help-а */
			static void help_print();

			inline std::shared_ptr<DebugStream> log()
			{
				return dblog;
			}

			void run( bool async );
#ifndef DISABLE_REST_API
			Poco::Net::HTTPRequestHandler* createRequestHandler( const Poco::Net::HTTPServerRequest& req );
			virtual void handleRequest( Poco::Net::HTTPServerRequest& req, Poco::Net::HTTPServerResponse& resp ) override;
#endif

		protected:

			class Log;

			virtual void evfinish() override;
			virtual void evprepare() override;
			void onTimer( ev::timer& t, int revents );
			void onCheckBuffer( ev::timer& t, int revents );
			void addLog( Log* log, const std::string& txt );
#ifndef DISABLE_REST_API
			Poco::JSON::Object::Ptr respError( Poco::Net::HTTPServerResponse& resp, Poco::Net::HTTPResponse::HTTPStatus s, const std::string& message );
			Poco::JSON::Object::Ptr httpGetRequest( const std::string& cmd, const Poco::URI::QueryParameters& p );
			Poco::JSON::Object::Ptr httpGetList( const Poco::URI::QueryParameters& p );
			Poco::JSON::Object::Ptr httpGetLogs( const Poco::URI::QueryParameters& p );
			Poco::JSON::Object::Ptr httpGetCount( const Poco::URI::QueryParameters& p );
#endif
			std::string myname;
			std::unique_ptr<SQLiteInterface> db;

			bool activate = { false };

			typedef std::queue<std::string> QueryBuffer;
			QueryBuffer qbuf;
			size_t qbufSize = { 200 }; // размер буфера сообщений.

			void flushBuffer();

			class Log
			{
				public:
					std::string name;
					std::string ip;
					int port = { 0 };
					std::string cmd;
					std::string peername;
					std::string description;

					std::shared_ptr<DebugStream> dblog;

					bool connect() noexcept;
					bool isConnected() const;
					void ioprepare( ev::dynamic_loop& loop );
					void event( ev::io& watcher, int revents );
					void read( ev::io& watcher);
					void write( ev::io& io );
					void close();

					typedef sigc::signal<void, Log*, const std::string&> ReadSignal;
					ReadSignal signal_on_read();

				private:
					ReadSignal sigRead;
					ev::io io;
					std::shared_ptr<UTCPStream> tcp;
					static const int bufsize = { 10001 };
					char buf[bufsize];

					static const size_t reservsize = { 1000 };
					std::string text;

					// буфер для посылаемых данных (write buffer)
					std::queue<UTCPCore::Buffer*> wbuf;
			};

			std::vector< std::shared_ptr<Log> > logservers;
			std::shared_ptr<DebugStream> dblog;

			ev::timer connectionTimer;
			timeout_t tmConnection_msec = { 5000 }; // пауза между попытками установить соединение
			double tmConnection_sec = { 0.0 };

			ev::timer checkBufferTimer;
			double tmCheckBuffer_sec = { 1.0 };


#ifndef DISABLE_REST_API
			std::shared_ptr<Poco::Net::HTTPServer> httpserv;
			std::string httpHost = { "" };
			int httpPort = { 0 };
#endif

		private:
	};
	// ----------------------------------------------------------------------------------
} // end of namespace uniset
//------------------------------------------------------------------------------------------
#endif
