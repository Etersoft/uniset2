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
#include "LogServer.h"
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
      - \ref sec_LogDB_CONTROL
      - \ref sec_LogDB_WEBSOCK
      - \ref sec_LogDB_DETAIL
      - \ref sec_LogDB_ADMIN
      - \ref sec_LogDB_LOGFILE


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
      <logserver name="" ip=".." port=".." cmd=".." logfile=".."/>
    </LogDB>
    \endcode

    При этом доступно два способа:
    * Первый - это использование секции в общем файле проекта (configure.xml).
    * Второй способ - позволяет создать отдельный xml-файл с одной настроечной секцией и указать его в аргументах командной строки
    \code
    uniset2-logdb --single-confile logdbconf.xml
    \endcode


    \section sec_LogDB_DB LogDB: Работа с БД
    Для оптимизации, запись в БД сделана не по каждому сообщению, а через промежуточный буфер.
    Т.е. только после того как в буфере скапливается \a qbufSize сообщений (строк) буфер скидывается в базу.
    Помимо этого, встроен механизм "ротации БД". Если задан параметр maxRecords (--prefix-db-max-records),
    то в БД будет поддерживаться ограниченное количество записей. При этом введён "гистерезис",
    т.е. фактически удаление старых записей начинается при переполнении БД определяемом коэффициентом
    переполнения overflowFactor (--prefix-db-overflow-factor). По умолчанию 1.3.

    \section sec_LogDB_REST LogDB: REST API
    LogDB предоставляет возможность получения логов через REST API. Для этого запускается
    http-сервер. Параметры запуска можно указать при помощи:
    --prefix-httpserver-host и --prefix-httpserver-port.

    А так же --prefix-httpserver-max-queued для указания максимального размера очереди запросов к серверу
    и  --prefix-httpserver-max-threads количество потоков обработки запросов.

    REST API доступен по пути: api/version/logdb/...  (текущая версия v01)
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

    /count?logname           - Получить текущее количество записей
    /download                - Загрузить файл БД. По умолчанию выключено (см. httpEnabledDownload)
    \endcode

    \section sec_LogDB_CONTROL LogDB: CONTROL API
    CONTROL API доступен по пути: api/version/logcontrol/...  (текущая версия v01)

    По умолчанию отключено (см httpEnabledLogControl)
    \code
      /logcontrol/logname?set=info,warn,crit  - Включить уровень логов для logname
      /logcontrol/logname?reset               - Вернуть логи к настройкам по умолчанию (из конфига)
      /logcontrol/logname?get                 - Получить текущие выставленные настройки
    \endcode

    \section sec_LogDB_WEBSOCK LogDB: Поддержка web socket

    В LogDB встроена возможность просмотра логов в реальном времени, через websocket.
    Список лог-серверов доступен по адресу:
    \code
    http://host:port/ws/
    \endcode
    Прямое подключение к websocket-у доступно по адресу:
    \code
    ws://host:port/ws/connect/logname?set=info,warn,crit,-level2
    \endcode
    Где \a logname - это имя логсервера от которого мы хотим получать логи (см. \ref sec_LogDB_Conf).
    \a set - позволяет установить желаемый уровень логов (не обязательный параметр)

    Количество создаваемых websocket-ов можно ограничить при помощи параметр maxWebsockets (--prefix-ws-max).

    Управлять уровнем логов можно через API
    \code
      http://host:port/ws/logname?set=info,warn,crit,-level2
      set=loglevel - Установить loglevel для logname. Разрешение на управление должно быть включено (см. httpEnabledLogControl)
      'loglevel'   - задание в формате команды. Пример: info,warn,+level1,-level2
    \endcode


    \section sec_LogDB_LOGFILE LogDB: Файлы логов
    Несмотря на то, что все логи сохраняются в БД, их так же можно писать в файлы.
    Для этого каждому логу достаточно указать свойство \b logfile в настройках (см. \ref sec_LogDB_Conf)


    \section sec_LogDB_DETAIL LogDB: Технические детали
       Вся реализация построена на "однопоточном" eventloop. В нём происходит,
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
    \todo db: Сделать настройку, для формата даты и времени при выгрузке из БД (при формировании json).
    \todo rest: Возможно в /logs стоит в ответе сразу возвращать и общее количество в БД (это один лишний запрос, каждый раз).
    \todo db: Возможно в последствии оптимизировать таблицы (нормализовать) если будет тормозить. Сейчас пока прототип.
    \todo db: Пока не очень эффективная работа с датой и временем (заодно подумать всё-таки в чём хранить)
    \todo db: возможно всё-таки стоит парсить логи на предмет loglevel, и тогда уж и дату с временем вынимать
    \todo web: генерировать html-страничку со списком подключения к логам с использованием готового шаблона
    \todo db: сделать в RESET API команду включения или отключения запись логов в БД, для управления "на ходу"
    \todo Продумать и реализовать тесты
    \todo rest: Добавить функцию получения значений внутренних переменных и настроек (для отладки LogDB)
    */
    //------------------------------------------------------------------------------------------
    /*! Реализация LogDB */
    class LogDB:
        public EventLoopServer
#ifndef DISABLE_REST_API
        , public Poco::Net::HTTPRequestHandler
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
            void log2File( Log* log, const std::string& txt );

            size_t getCountOfRecords( const std::string& logname = "" );
            size_t getFirstOfOldRecord( size_t maxnum );

            // экранирование кавычек (удваивание для sqlite)
            static std::string qEscapeString( const std::string& s );

#ifndef DISABLE_REST_API
            Poco::JSON::Object::Ptr respError( Poco::Net::HTTPServerResponse& resp, Poco::Net::HTTPResponse::HTTPStatus s, const std::string& message );
            Poco::JSON::Object::Ptr httpGetRequest( Poco::Net::HTTPServerResponse& resp, const std::string& cmd, const Poco::URI::QueryParameters& p );
            Poco::JSON::Object::Ptr httpGetList( Poco::Net::HTTPServerResponse& resp, const Poco::URI::QueryParameters& p );
            Poco::JSON::Object::Ptr httpGetLogs( Poco::Net::HTTPServerResponse& resp, const Poco::URI::QueryParameters& p );
            Poco::JSON::Object::Ptr httpGetCount( Poco::Net::HTTPServerResponse& resp, const Poco::URI::QueryParameters& p );
            Poco::JSON::Object::Ptr httpDownload( Poco::Net::HTTPServerRequest& req, Poco::Net::HTTPServerResponse& resp, const Poco::URI::QueryParameters& p );
            Poco::JSON::Object::Ptr httpLogControl(std::ostream& out, Poco::Net::HTTPServerRequest& req,
                                                   Poco::Net::HTTPServerResponse& resp, const std::string& logname, const Poco::URI::QueryParameters& params );

            void httpWebSocketPage( std::ostream& out, Poco::Net::HTTPServerRequest& req,
                                    Poco::Net::HTTPServerResponse& resp, const Poco::URI::QueryParameters& p );
            void httpWebSocketConnectPage( Poco::Net::HTTPServerRequest& req,
                                           Poco::Net::HTTPServerResponse& resp, const std::string& logname, const Poco::URI::QueryParameters& p );

            bool supportsGzip( Poco::Net::HTTPServerRequest& request );

            // формирование условия where для строки XX[m|h|d|M]
            // XX m - минут, h-часов, d-дней, M - месяцев
            static std::string qLast( const std::string& p );

            // преобразование в дату 'YYYY-MM-DD' из строки 'YYYYMMDD' или 'YYYY/MM/DD'
            static std::string qDate(const std::string& p, const char sep = '-');

            std::shared_ptr<LogWebSocket> newWebSocket(Poco::Net::HTTPServerRequest* req, Poco::Net::HTTPServerResponse* resp,
                    const std::string& logname, const Poco::URI::QueryParameters& p );
            void delWebSocket( std::shared_ptr<LogWebSocket>& ws );

#endif
            std::string myname;
            std::unique_ptr<SQLiteInterface> db;
            std::string dbfile;

            std::string tmsFormat = { "localtime" }; /*!< формат возвращаемого времени */

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

            ev::sig sigTERM;
            ev::sig sigQUIT;
            ev::sig sigINT;
            void onTerminate( ev::sig& evsig, int revents );

            ev::async wsactivate; // активация LogWebSocket-ов

            std::shared_ptr<uniset::LogServer> logserv;
            std::string logserv_host = {""};
            int logserv_port = {0};

            class Log
            {
                public:
                    std::string name;
                    std::string ip;
                    int port = { 0 };
                    std::string cmd;
                    std::string usercmd;
                    std::string lastcmd;
                    std::string peername;
                    std::string description;

                    std::shared_ptr<DebugStream> dblog;
                    std::shared_ptr<DebugStream> logfile;

                    bool isConnected() const;

                    void set( ev::dynamic_loop& loop );
                    void check( ev::timer& t, int revents );
                    void event( ev::io& watcher, int revents );
                    void read( ev::io& watcher );
                    void oncommand( ev::async& watcher, int revents );
                    void write( ev::io& io );
                    void close();

                    typedef sigc::signal<void, Log*, const std::string&> ReadSignal;
                    ReadSignal signal_on_read();

                    void setCheckConnectionTime( double sec );
                    void setReadBufSize( size_t sz );
                    void setCommand( const std::string& cmd );

                protected:
                    void ioprepare();
                    bool connect() noexcept;

                private:
                    ReadSignal sigRead;
                    ev::io io;
                    ev::timer iocheck;
                    ev::async iocmd;

                    double checkConnection_sec = { 5.0 };

                    std::shared_ptr<UTCPStream> tcp;
                    std::vector<char> buf; // буфер для чтения сообщений

                    static const size_t reservsize = { 1000 };
                    std::string text;

                    // буфер для посылаемых данных (write buffer)
                    std::queue<UTCPCore::Buffer*> wbuf;

                    // очередь команд для посылки
                    std::vector<UTCPCore::Buffer*> cmdbuf;
                    uniset::uniset_rwmutex cmdmut;
            };

            std::vector< std::shared_ptr<Log> > logservers;
            std::shared_ptr<DebugStream> dblog;

#ifndef DISABLE_REST_API
            std::shared_ptr<Poco::Net::HTTPServer> httpserv;
            std::string httpHost = { "" };
            int httpPort = { 0 };
            std::string httpCORS_allow = { "*" };
            std::string httpReplyAddr = { "" };
            std::string httpJsonContentType = {"text/json; charset=UTF-8" };
            std::string httpHtmlContentType = {"text/html; charset=UTF-8" };
            std::string utf8Code = "UTF-8";

            double wsHeartbeatTime_sec = { 3.0 };
            double wsSendTime_sec = { 0.5 };
            size_t wsMaxSend = { 200 };
            double wsBackpressureTime_sec = { 15.0 };
            bool httpEnabledLogControl = { false };
            bool httpEnabledDownload = { false };

            std::string wsPageTemplate = "";

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

                    // настройка
                    void setHearbeatTime( const double& sec );
                    void setSendPeriod( const double& sec );
                    void setMaxSendCount( size_t val );
                    void setBackpressureTimeout( const double& sec );

                protected:

                    void write();
                    void handleBackpressure();

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
                    size_t maxsize; // рассчитывается  исходя из max_send (см. конструктор)
                    size_t lostByOverflow = { 0 };
                    size_t backpressureCount = { 0 };
                    std::chrono::steady_clock::time_point backpressureStart;
                    bool backpressureActive = { false };
                    double backpressureTimeout_sec = { 5.0 };

                    std::shared_ptr<Log> log;
            };

            class LogWebSocketGuard
            {
                public:

                    LogWebSocketGuard( std::shared_ptr<LogWebSocket>& s, LogDB* l ):
                        ws(s), logdb(l) {}

                    ~LogWebSocketGuard()
                    {
                        ws->term();
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

            class LogDBRequestHandlerFactory:
                public Poco::Net::HTTPRequestHandlerFactory
            {
                public:
                    LogDBRequestHandlerFactory( LogDB* l ): logdb(l) {}
                    virtual ~LogDBRequestHandlerFactory() {}

                    virtual Poco::Net::HTTPRequestHandler* createRequestHandler( const Poco::Net::HTTPServerRequest& req ) override;

                private:
                    LogDB* logdb;
            };
#endif

        private:
    };
    // ----------------------------------------------------------------------------------
} // end of namespace uniset
//------------------------------------------------------------------------------------------
#endif
