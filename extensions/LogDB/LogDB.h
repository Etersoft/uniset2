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
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <ev++.h>
#include <sigc++/sigc++.h>
#include <Poco/JSON/Object.h>
#include <Poco/Net/WebSocket.h>
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
		  \page page_LogDB База логов (LogDB)

		  - \ref sec_LogDB_Comm
		  - \ref sec_LogDB_Conf
		  - \ref sec_LogDB_DB
		  - \ref sec_LogDB_REST
		  - \ref sec_LogDB_WEBSOCK
		  - \ref sec_LogDB_DETAIL
		  - \ref sec_LogDB_ADMIN


		\section sec_LogDB_Comm Общее описание работы LogDB
			LogDB это сервис, работа которого заключается
		в подключении к указанным лог-серверам, получении от них логов
		и сохранении их в БД (sqlite). Помимо этого LogDB выступает в качестве
		REST сервиса, позволяющего получать логи за указанный период в виде json.

		Реализация намеренно простая, т.к. пока неясно нужно ли это и в каком виде.
		Ожидается что контролируемых логсерверов будет не очень много (максимум несколько десятков)
		и каждый лог будет генерировать не более 2-5 мегабайт записей. Поэтому sqlite должно хватить.

		\section sec_LogDB_Conf Конфигурирование LogDB
		Для конфигурования необходимо создать секцию вида:
		\code
		<LogDB name="LogDB" ...>
			<logserver name="" ip=".." port=".." cmd=".." description=".."/>
			<logserver name="" ip=".." port=".." cmd=".." description=".."/>
			<logserver name="" ip=".." port=".." cmd=".."/>
		</LogDB>
		\endcode

		При этом доступно два способа:

		* Первый - это использование секции в общем файле проекта (cofigure.xml).

		* Второй способ - позволят просто создать xml-файл с одной настроечной секцией и указать его
		в аргументах командной строки
		\code
		uniset2-logdb --single-confile logdbconf.xml
		\endcode


		\section sec_LogDB_DB LogDB Работа с БД
		Для оптимизации, запись в БД сделана не по каждому сообщению, а через промежуточнй буффер.
		Т.е. только после того как в буфере скапливается \a qbufSize сообщений (строк) буфер скидывается в базу.
		Помимо этого, встроен механизм "ротации БД". Если задан параметр maxRecords (--prefix-max-records),
		то в БД будет поддерживаться ограниченное количество записей. При этом введён "гистерезис",
		т.е. фактически удаление старых записей начинается при переполнении БД определяемом коэффициентом
		переполнения overflowFactor (--prefix-overflow-factor). По умолчанию 1.3.

		\section sec_LogDB_REST LogDB REST API
			LogDB предоставляет возможность получения логов через REST API. Для этого запускается
		http-сервер. Параметры запуска можно указать при помощи:
		--prefix-httpserver-host и --prefix-httpserver-port.

		Запросы обрабатываются по пути: api/version/logdb/...
		\code
		/help                            - Получение списка доступных команд
		/list                            - список доступных логов
		/logs?logname&..parameters..     - получение логов 'logname'
			  Не обязательные параметры:
				  offset=N               - начиная с N-ой записи,
				  limit=M                - количество в ответе.
				  from='YYYY-MM-DD'      - 'с' указанной даты
				  to='YYYY-MM-DD'        - 'по' указанную дату
				  last=XX[m|h|d|M]       - за последние XX m-минут, h-часов, d-дней, M-месяцев
										   По умолчанию: минут

		/count?logname                   - Получить текущее количество записей
		\endcode


		\section sec_LogDB_WEBSOCK LogDB Поддержка web socket

		 В LogDB встроена возможность просмотра логов в реальном времени, через websocket.
		 Список доступных для подключения лог-серверов доступен по адресу:
		 \code
		 ws://host:port/logdb/ws/
		 \endcode
		 Прямое подключение к websocket-у доступно по адресу:
		 \code
		 ws://host:port/logdb/ws/logname
		 \endcode
		 Где \a logname - это имя логсервера от которого мы хотим получать логи (см. \ref sec_LogDB_Conf).

		Количество создаваемых websocket-ов можно ограничить при помощи параметр maxWebsockets (--prefix-max-websockets).


		\section sec_LogDB_DETAIL LogDB Технические детали
		   Вся релизация построена на "однопоточном" eventloop. В нём происходит,
		 чтение данных от логсерверов, посылка сообщений в websockets, запись в БД.
		 При этом обработка запросов REST API реализуется отдельными потоками контролируемыми libpoco.


		\section sec_LogDB_ADMIN LogDB Вспомогательная утилита (uniset2-logdb-adm).

		   Для "обслуживания БД" (создание, конвертирование, ротация) имеется специальная утилита uniset2-logdb-adm.
		Т.к. logdb при своём запуске подразумевает, что БД уже создана. То для создания БД можно воспользоваться
		командой
		\code
			 uniset2-logdb-adm create dbfilename
		\endcode

		Для того, чтобы конвертировать (загрузить) отдельные лог-файлы в базу, можно воспользоваться командой
		\code
			 uniset2-logdb-adm load logfile1 logfile2...logfileN
		\endcode

		Более детальное описание параметров см. \b uniset2-logdb-adm \b help


		\todo conf: может быть даже добавить поддержку конфигурирования в формате yaml.
		\todo Добавить настройки таймаутов, размера буфера, размера для резервирования под строку, количество потоков для http и т.п.
		\todo db: Сделать настройку, для формата даты и времени при выгрузке из БД (при формировании json).
		\todo rest: Возможно в /logs стоит в ответе сразу возвращать и общее количество в БД (это один лишний запрос, каждый раз).
		\todo db: Возможно в последствии оптимизировать таблицы (нормализовать) если будет тормозить. Сейчас пока прототип.
		\todo db: Пока не очень эффективная работа с датой и временем (заодно подумать всё-таки в чём хранить)
		\todo WebSocket: доделать настройку всевозможных timeout-ов
		\todo db: возможно всё-таки стоит парсить логи на предмет loglevel, и тогда уж и дату с временем вынимать
		\todo web: генерировать html-страничку со списком подключения к логам с использованием готового шаблона
		\todo db: сделать в RESET API команду включения или отключения запись логов в БД, для управления "на ходу"
	*/
	class LogDB:
		public EventLoopServer
#ifndef DISABLE_REST_API
		, public Poco::Net::HTTPRequestHandler
		, public Poco::Net::HTTPRequestHandlerFactory
#endif
	{
		public:
			LogDB( const std::string& name, int argc, const char* const* argv, const std::string& prefix );
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
			void onWebSocketSession( Poco::Net::HTTPServerRequest& req, Poco::Net::HTTPServerResponse& resp );
#endif

		protected:

			class Log;
			class LogWebSocket;

			virtual void evfinish() override;
			virtual void evprepare() override;
			void onCheckBuffer( ev::timer& t, int revents );
			void onActivate( ev::async& watcher, int revents ) ;
			void addLog( Log* log, const std::string& txt );

			size_t getCountOfRecords( const std::string& logname = "" );
			size_t getFirstOfOldRecord( size_t maxnum );

#ifndef DISABLE_REST_API
			Poco::JSON::Object::Ptr respError( Poco::Net::HTTPServerResponse& resp, Poco::Net::HTTPResponse::HTTPStatus s, const std::string& message );
			Poco::JSON::Object::Ptr httpGetRequest( const std::string& cmd, const Poco::URI::QueryParameters& p );
			Poco::JSON::Object::Ptr httpGetList( const Poco::URI::QueryParameters& p );
			Poco::JSON::Object::Ptr httpGetLogs( const Poco::URI::QueryParameters& p );
			Poco::JSON::Object::Ptr httpGetCount( const Poco::URI::QueryParameters& p );
			void httpWebSocketPage( std::ostream& out, Poco::Net::HTTPServerRequest& req, Poco::Net::HTTPServerResponse& resp );
			void httpWebSocketConnectPage( std::ostream& out, Poco::Net::HTTPServerRequest& req, Poco::Net::HTTPServerResponse& resp, const std::string& logname );

			// формирование условия where для строки XX[m|h|d|M]
			// XX m - минут, h-часов, d-дней, M - месяцев
			static std::string qLast( const std::string& p );

			// преобразование в дату 'YYYY-MM-DD' из строки 'YYYYMMDD' или 'YYYY/MM/DD'
			static std::string qDate(const std::string& p , const char sep = '-');

			// экранирование кавычек (удваивание для sqlite)
			static std::string qEscapeString( const std::string& s );

			std::shared_ptr<LogWebSocket> newWebSocket(Poco::Net::HTTPServerRequest* req, Poco::Net::HTTPServerResponse* resp, const std::string& logname );
			void delWebSocket( std::shared_ptr<LogWebSocket>& ws );
#endif
			std::string myname;
			std::unique_ptr<SQLiteInterface> db;

			bool activate = { false };

			typedef std::queue<std::string> QueryBuffer;
			QueryBuffer qbuf;
			size_t qbufSize = { 1000 }; // размер буфера сообщений.

			ev::timer flushBufferTimer;
			double tmFlushBuffer_sec = { 1.0 };
			void flushBuffer();
			void rotateDB();

			size_t maxdbRecords = { 200 * 1000 };
			size_t numOverflow = { 0 }; // вычисляется из параметра "overflow factor"(float)

			ev::async wsactivate; // активация LogWebSocket-ов

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

					bool isConnected() const;

					void set( ev::dynamic_loop& loop );
					void check( ev::timer& t, int revents );
					void event( ev::io& watcher, int revents );
					void read( ev::io& watcher );
					void write( ev::io& io );
					void close();

					typedef sigc::signal<void, Log*, const std::string&> ReadSignal;
					ReadSignal signal_on_read();

				protected:
					void ioprepare();
					bool connect() noexcept;

				private:
					ReadSignal sigRead;
					ev::io io;
					ev::timer iocheck;

					double checkConnection_sec = { 5.0 };

					std::shared_ptr<UTCPStream> tcp;
					static const int bufsize = { 10001 };
					char buf[bufsize]; // буфер для чтения сообщений

					static const size_t reservsize = { 1000 };
					std::string text;

					// буфер для посылаемых данных (write buffer)
					std::queue<UTCPCore::Buffer*> wbuf;
			};

			std::vector< std::shared_ptr<Log> > logservers;
			std::shared_ptr<DebugStream> dblog;

#ifndef DISABLE_REST_API
			std::shared_ptr<Poco::Net::HTTPServer> httpserv;
			std::string httpHost = { "" };
			int httpPort = { 0 };

			/*! класс реализует работу с websocket через eventloop
			 * Из-за того, что поступление логов может быть достаточно быстрым
			 * чтобы не "завалить" браузер кучей сообщений,
			 * сделана посылка не по факту приёма сообщения, а раз в send_sec,
			 * не более maxsend сообщений.
			 * \todo websocket: может стоит объединять сообщения в одну посылку (пока считаю преждевременной оптимизацией)
			 */
			class LogWebSocket:
				public Poco::Net::WebSocket
			{
				public:
					LogWebSocket(Poco::Net::HTTPServerRequest* req,
								 Poco::Net::HTTPServerResponse* resp,
								 std::shared_ptr<Log>& log );

					virtual ~LogWebSocket();

					// конечно некрасиво что это в public
					std::shared_ptr<DebugStream> dblog;

					bool isActive();
					void set( ev::dynamic_loop& loop );

					void send( ev::timer& t, int revents );
					void ping( ev::timer& t, int revents );

					void add( Log* log, const std::string& txt );

					void term();

					void waitCompletion();

				protected:

					void write();

					ev::timer iosend;
					double send_sec = { 0.5 };
					size_t maxsend = { 200 };

					ev::timer ioping;
					double ping_sec = { 3.0 };

					std::mutex              finishmut;
					std::condition_variable finish;

					std::atomic_bool cancelled = { false };

					sigc::connection con; // подписка на появление логов..

					Poco::Net::HTTPServerRequest* req;
					Poco::Net::HTTPServerResponse* resp;

					// очередь данных на посылку..
					std::queue<UTCPCore::Buffer*> wbuf;
					size_t maxsize; // рассчитывается сходя из max_send (см. конструктор)
			};

			class LogWebSocketGuard
			{
				public:

					LogWebSocketGuard( std::shared_ptr<LogWebSocket>& s, LogDB* l ):
						ws(s), logdb(l) {}

					~LogWebSocketGuard()
					{
						logdb->delWebSocket(ws);
					}


				private:
					std::shared_ptr<LogWebSocket> ws;
					LogDB* logdb;
			};

			friend class LogWebSocketGuard;

			std::list<std::shared_ptr<LogWebSocket>> wsocks;
			uniset::uniset_rwmutex wsocksMutex;
			size_t maxwsocks = { 50 }; // максимальное количество websocket-ов
#endif

		private:
	};
	// ----------------------------------------------------------------------------------
} // end of namespace uniset
//------------------------------------------------------------------------------------------
#endif
