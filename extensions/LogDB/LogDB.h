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
#include <deque>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <atomic>
#include <ev++.h>
#include <sigc++/sigc++.h>
#include <Poco/JSON/Object.h>
#include <Poco/Net/WebSocket.h>
#include <Poco/ObjectPool.h>
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
    /*! \ingroup extensions
        Реализация LogDB.
        \sa \ref page_LogDB
    */
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
            void handleOverload(Poco::Net::HTTPServerRequest& req, Poco::Net::HTTPServerResponse& resp);
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
            Poco::JSON::Object::Ptr httpGetStatus( Poco::Net::HTTPServerResponse& resp, const Poco::URI::QueryParameters& p );
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
            std::chrono::steady_clock::time_point startTime;

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
            std::atomic<size_t> httpActiveRequests{0};
            size_t httpMaxThreads = { 15 };
            size_t httpMaxQueued = { 0 };

            double wsHeartbeatTime_sec = { 3.0 };
            double wsSendTime_sec = { 0.5 };
            size_t wsMaxSend = { 200 };
            double wsBackpressureTime_sec = { 15.0 };
            size_t wsQueueBytesLimit = { 2 * 1024 * 1024 };
            size_t wsFrameBytesLimit = { 64 * 1024 };
            bool httpEnabledLogControl = { false };
            bool httpEnabledDownload = { false };
            double wsPongTimeout_sec = { 10.0 };
            double wsMaxLifetime_sec = { 0 }; // 0 = unlimited

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
                    void setPendingNotice( const std::string& msg );
                    void setQueueBytesLimit( size_t bytes );
                    void setMaxFrameBytes( size_t bytes );
                    void setPongTimeout( const double& sec );
                    void setMaxLifetime( const double& sec );

                protected:
                    void read( ev::io& w, int revents );
                    void checkPongTimeout( ev::timer& t, int revents );

                    void enqueueMessage( const std::string& msg );
                    void buildFramesFromMessages();
                    void clearFrames();
                    void logQueueStats( const std::string& reason );
                    void write();
                    void handleBackpressure();

                    ev::timer iosend;
                    double send_sec = { 0.5 };
                    size_t maxsend = { 200 };

                    ev::timer ioping;
                    double ping_sec = { 3.0 };

                    ev::io ioread;
                    ev::timer iopongcheck;
                    double pongTimeout_sec = { 10.0 };
                    std::atomic_bool waitingPong = { false };
                    std::chrono::steady_clock::time_point lastPingSent;
                    std::chrono::steady_clock::time_point sessionStart;
                    double maxLifetime_sec = { 0 }; // 0 = unlimited

                    std::mutex              finishmut;
                    std::condition_variable finish;

                    std::atomic_bool cancelled = { false };

                    sigc::connection con; // подписка на появление логов..

                    Poco::Net::HTTPServerRequest* req;
                    Poco::Net::HTTPServerResponse* resp;

                    // очередь данных на посылку..
                    std::queue<UTCPCore::Buffer*> wbuf;
                    std::deque<std::string> msgQueue;
                    size_t queuedBytes = { 0 };
                    size_t queueBytesLimit = { 2 * 1024 * 1024 }; // предел буфера сообщений
                    size_t maxFrameBytes = { 64 * 1024 }; // ограничение на размер одного фрейма
                    std::chrono::steady_clock::time_point lastDiag;
                    size_t lostByOverflow = { 0 };
                    size_t backpressureCount = { 0 };
                    std::chrono::steady_clock::time_point backpressureStart;
                    bool backpressureActive = { false };
                    double backpressureTimeout_sec = { 5.0 };
                    std::string pendingNotice;

                    std::unique_ptr<Poco::ObjectPool<uniset::UTCPCore::Buffer>> bufPool;
                    size_t bufPoolCapacity = { 256 };
                    size_t bufPoolPeak = { 2000 };

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

            friend class LogDBOverloadRequestHandler;
#endif

        private:
    };
    // ----------------------------------------------------------------------------------
} // end of namespace uniset
//------------------------------------------------------------------------------------------
#endif
