/*
 * Copyright (c) 2017 Pavel Vainerman.
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
#ifndef UWebSocketGate_H_
#define UWebSocketGate_H_
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
#include <Poco/ObjectPool.h>
#include "UniSetTypes.h"
#include "LogAgregator.h"
#include "UniSetObject.h"
#include "DebugStream.h"
#include "SharedMemory.h"
#include "SMInterface.h"
#include "EventLoopServer.h"
#include "UTCPStream.h"
#include "UHttpRequestHandler.h"
#include "UHttpServer.h"
#include "UTCPCore.h"
#include "RunLock.h"
// -------------------------------------------------------------------------
namespace uniset
{
    //------------------------------------------------------------------------------------------
    class UWebSocketGate:
        public UniSetObject,
        public EventLoopServer
#ifndef DISABLE_REST_API
        , public Poco::Net::HTTPRequestHandler
#endif
    {
        public:
            UWebSocketGate( uniset::ObjectId id, xmlNode* cnode
                            , uniset::ObjectId shmID
                            , const std::shared_ptr<SharedMemory>& ic = nullptr
                            , const std::string& prefix = "-ws" );

            virtual ~UWebSocketGate();

            /*! глобальная функция для инициализации объекта */
            static std::shared_ptr<UWebSocketGate> init_wsgate( int argc, const char* const* argv
                    , uniset::ObjectId shmID
                    , const std::shared_ptr<SharedMemory>& ic = nullptr
                    , const std::string& prefix = "ws-" );

            /*! глобальная функция для вывода help-а */
            static void help_print();

            inline std::shared_ptr<DebugStream> log()
            {
                return mylog;
            }
            inline std::shared_ptr<uniset::LogAgregator> logAgregator() noexcept
            {
                return loga;
            }

#ifndef DISABLE_REST_API
            virtual void handleRequest( Poco::Net::HTTPServerRequest& req, Poco::Net::HTTPServerResponse& resp ) override;
            void onWebSocketSession( Poco::Net::HTTPServerRequest& req, Poco::Net::HTTPServerResponse& resp );
            Poco::JSON::Object::Ptr httpStatus();
            Poco::JSON::Object::Ptr httpList();
            Poco::JSON::Object::Ptr httpHelpApi();
#endif

            static Poco::JSON::Object::Ptr error_to_json( std::string_view err );
            static void fill_error_json( Poco::JSON::Object::Ptr& p, std::string_view err );

        protected:

            class UWebSocket;

            virtual bool activateObject() override;
            virtual bool deactivateObject() override;
            virtual void sysCommand( const uniset::SystemMessage* sm ) override;
            void run( bool async );
            virtual void evfinish() override;
            virtual void evprepare() override;
            void onCheckBuffer( ev::timer& t, int revents );
            void onActivate( ev::async& watcher, int revents ) ;
            void onCommand( ev::async& watcher, int revents );

#ifndef DISABLE_REST_API
            void httpWebSocketPage( std::ostream& out, Poco::Net::HTTPServerRequest& req, Poco::Net::HTTPServerResponse& resp );
            void httpWebSocketConnectPage(std::ostream& out, Poco::Net::HTTPServerRequest& req,
                                          Poco::Net::HTTPServerResponse& resp, const std::string& params );

            std::shared_ptr<UWebSocket> newWebSocket(Poco::Net::HTTPServerRequest* req, Poco::Net::HTTPServerResponse* resp, const Poco::URI::QueryParameters& qp );
            void delWebSocket( std::shared_ptr<UWebSocket>& ws );

            Poco::JSON::Object::Ptr respError( Poco::Net::HTTPServerResponse& resp, Poco::Net::HTTPResponse::HTTPStatus s, const std::string& message );
            void makeResponseAccessHeader( Poco::Net::HTTPServerResponse& resp );
#endif
            void terminate();

            ev::async wsactivate; // активация WebSocket-ов
            std::shared_ptr<ev::async> wscmd;

            void checkMessages( ev::timer& t, int revents );
            virtual void sensorInfo( const uniset::SensorMessage* sm ) override;
            virtual uniset::SimpleInfo* getInfo( const char* userparam = 0 ) override;
            ev::timer iocheck;
            double check_sec = { 0.05 };
            int maxMessagesProcessing  = { 200 };

            std::shared_ptr<DebugStream> mylog;
            std::shared_ptr<uniset::LogAgregator> loga;
            std::shared_ptr<SMInterface> shm;
            std::unique_ptr<uniset::RunLock> runlock;

            std::shared_ptr<uniset::LogServer> logserv;
            std::string logserv_host = {""};
            int logserv_port = { 0 };

#ifndef DISABLE_REST_API
            std::shared_ptr<Poco::Net::HTTPServer> httpserv;
            std::string httpHost = { "" };
            int httpPort = { 0 };
            std::string httpCORS_allow = { "*" };

            double wsHeartbeatTime_sec = { 3.0 };
            double wsSendTime_sec = { 0.2 };
            size_t wsMaxSend = { 5000 };
            size_t wsMaxCmd = { 200 };
            double wsPongTimeout_sec = { 5.0 };
            double wsMaxLifetime_sec = { 0 }; // 0 = unlimited

            int jpoolCapacity = { 200 };
            int jpoolPeakCapacity = { 5000 };

            /*! класс реализует работу с websocket через eventloop
             * Из-за того, что поступление событий может быть достаточно быстрым
             * чтобы не "завалить" клиента кучей сообщений,
             * сделана посылка не по факту приёма сообщения, а раз в send_sec,
             * не более maxsend сообщений.
             * \todo websocket: может стоит объединять сообщения в одну посылку (пока считаю преждевременной оптимизацией)
             */
            class UWebSocket:
                public Poco::Net::WebSocket
            {
                public:
                    UWebSocket( Poco::Net::HTTPServerRequest* req,
                                Poco::Net::HTTPServerResponse* resp,
                                int jpoolCapacity = 100,
                                int jpoolPeakCapacity = 500 );

                    virtual ~UWebSocket();

                    std::string getInfo() const noexcept;

                    bool isActive();
                    void set( ev::dynamic_loop& loop, std::shared_ptr<ev::async> a );

                    void send( ev::timer& t, int revents );
                    void ping( ev::timer& t, int revents );
                    void read( ev::io& io, int revents );
                    void pong( ev::timer& t, int revents );
                    void onLifetimeExpired( ev::timer& t, int revents );

                    struct sinfo
                    {
                        sinfo( const std::string& _cmd, uniset::ObjectId _id ): id(_id), cmd(_cmd) {}

                        std::string err; // ошибка при работе с датчиком (например при заказе)
                        uniset::ObjectId id = { uniset::DefaultObjectId };
                        std::string cmd = "";
                        long value = { 0 }; // set value
                        // cache
                        std::string name;
                    };

                    void ask( uniset::ObjectId id );
                    void del( uniset::ObjectId id );
                    void get( uniset::ObjectId id );
                    void set( uniset::ObjectId id, long value );
                    void freeze( uniset::ObjectId id, long value );
                    void unfreeze( uniset::ObjectId id );
                    void sensorInfo( const uniset::SensorMessage* sm );
                    void doCommand( const std::shared_ptr<SMInterface>& ui );
                    static Poco::JSON::Object::Ptr to_short_json( const std::shared_ptr<sinfo>& si );
                    static Poco::JSON::Object::Ptr to_json( const uniset::SensorMessage* sm, const std::shared_ptr<sinfo>& si );
                    static void fill_short_json( Poco::JSON::Object::Ptr& p, const std::shared_ptr<sinfo>& si );
                    static void fill_json( Poco::JSON::Object::Ptr& p, const uniset::SensorMessage* sm, const std::shared_ptr<sinfo>& si );

                    void term();
                    void waitCompletion();

                    // настройка
                    void setHearbeatTime( const double& sec );
                    void setSendPeriod( const double& sec );
                    void setMaxSendCount( size_t val );
                    void setMaxCmdCount( size_t val );
                    void setPongTimeout( const double& sec );
                    void setMaxLifetime( const double& sec );

                    std::shared_ptr<DebugStream> mylog;

                protected:

                    void write();
                    void sendResponse( const std::shared_ptr<sinfo>& si );
                    void sendShortResponse( const std::shared_ptr<sinfo>& si );
                    void onCommand( std::string_view cmd );
                    void sendError( std::string_view message );
                    void returnObjectToPool( Poco::JSON::Object::Ptr& json );

                    ev::timer iosend;
                    double send_sec = { 0.5 };
                    size_t maxsend = { 5000 };
                    size_t maxcmd = { 200 };
                    const int Kbuf = { 10 }; // коэффициент для буфера сообщений (maxsend умножается на Kbuf)
                    static const size_t sbufLen = 100 * 1024;
                    // специальный предел (меньше максимального)
                    // чтобы гарантировать что объект полностью влез в буфер
                    static const size_t sbufLim = (size_t)(0.8 * sbufLen);
                    char sbuf[sbufLen]; // буфер используемый для преобразования json в потом байт (см. send)

                    ev::timer ioping;
                    double ping_sec = { 3.0 };
                    static const std::string ping_str;
                    ev::timer iopong;
                    double pongTimeout_sec = { 5.0 };
                    size_t pongCounter = { 0 };

                    ev::timer iolifetime;
                    double maxLifetime_sec = { 0 }; // 0 = unlimited
                    std::chrono::steady_clock::time_point sessionStart;

                    ev::io iorecv;
                    char rbuf[64 * 1024]; //! \todo сделать настраиваемым или применить Poco::FIFOBuffer
                    timeout_t recvTimeout = { 200 }; // msec
                    std::shared_ptr<ev::async> cmdsignal;

                    std::mutex              finishmut;
                    std::condition_variable finish;
                    std::mutex              dataMutex; // защита smap/jbuf от параллельных потоков

                    std::atomic_bool cancelled = { false };

                    std::unordered_map<uniset::ObjectId, std::shared_ptr<sinfo> > smap;
                    std::queue< std::shared_ptr<sinfo> > qcmd; // очередь команд

                    Poco::Net::HTTPServerRequest* req;
                    Poco::Net::HTTPServerResponse* resp;

                    // очередь json-на отправку
                    std::queue<Poco::JSON::Object::Ptr> jbuf;
                    std::unique_ptr<Poco::ObjectPool< Poco::JSON::Object, Poco::JSON::Object::Ptr >> jpoolSM;
                    std::unique_ptr<Poco::ObjectPool< Poco::JSON::Object, Poco::JSON::Object::Ptr >> jpoolErr;
                    std::unique_ptr<Poco::ObjectPool< Poco::JSON::Object, Poco::JSON::Object::Ptr >> jpoolShortSM;

                    // очередь данных на посылку..
                    std::unique_ptr<Poco::ObjectPool< uniset::UTCPCore::Buffer >> wbufpool;
                    std::queue<uniset::UTCPCore::Buffer*> wbuf;
                    size_t maxsize; // рассчитывается сходя из max_send (см. конструктор)
            };

            class UWebSocketGuard
            {
                public:

                    UWebSocketGuard( std::shared_ptr<UWebSocket>& s, UWebSocketGate* g ):
                        ws(s), wsgate(g) {}

                    ~UWebSocketGuard()
                    {
                        wsgate->delWebSocket(ws);
                    }

                private:
                    std::shared_ptr<UWebSocket> ws;
                    UWebSocketGate* wsgate;
            };

            friend class UWebSocketGuard;

            std::list<std::shared_ptr<UWebSocket>> wsocks;
            uniset::uniset_rwmutex wsocksMutex;
            size_t maxwsocks = { 50 }; // максимальное количество websocket-ов

            class UWebSocketGateRequestHandlerFactory:
                public Poco::Net::HTTPRequestHandlerFactory
            {
                public:
                    UWebSocketGateRequestHandlerFactory( UWebSocketGate* l ): wsgate(l) {}
                    virtual ~UWebSocketGateRequestHandlerFactory() {}

                    virtual Poco::Net::HTTPRequestHandler* createRequestHandler( const Poco::Net::HTTPServerRequest& req ) override;

                private:
                    UWebSocketGate* wsgate;
            };
#endif

        private:

    };
    // ----------------------------------------------------------------------------------
} // end of namespace uniset
//------------------------------------------------------------------------------------------
#endif
