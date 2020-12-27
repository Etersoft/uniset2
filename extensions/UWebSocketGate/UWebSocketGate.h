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
#include "UniSetTypes.h"
#include "LogAgregator.h"
#include "UniSetObject.h"
#include "DebugStream.h"
#include "EventLoopServer.h"
#include "UTCPStream.h"
#include "UHttpRequestHandler.h"
#include "UHttpServer.h"
#include "UTCPCore.h"
// -------------------------------------------------------------------------
namespace uniset
{
    //------------------------------------------------------------------------------------------
    /*!
          \page page_UWebSocketGate Шлюз для подключения через WebSocket (UWebSocketGate)

          - \ref sec_UWebSocketGate_Comm
          - \ref sec_UWebSocketGate_Conf
          - \ref sec_UWebSocketGate_Command

        \section sec_UWebSocketGate_Comm Общее описание работы UWebSocketGate
            UWebSocketGate это сервис, который позволяет подключаться через websocket и получать события
        об изменнии датчиков, а так же изменять состояние (см. \ref sec_UWebSocketGate_Command).
        Подключение к websocket-у доступно по адресу:
         \code
         ws://host:port/wsgate/?s1,s2,s3,s4&format=[json,txt,raw]
         \endcode

         Помимо этого UWebSocketGate работает в режиме мониторинга изменений датчиков.
         Для этого достаточно зайти на страничку по адресу:
         \code
         http://host:port/wsgate/?s1,s2,s3,s4&format=[json,txt,raw]
         \endcode

        \section sec_UWebSocketGate_Conf Конфигурирование UWebSocketGate
        Для конфигурования необходимо создать секцию вида:
        \code
        <UWebSocketGate name="UWebSocketGate" .../>
        \endcode

        Количество создаваемых websocket-ов можно ограничить при помощи параметра maxWebsockets (--prefix-ws-max).

        \section sec_UWebSocketGate_DETAIL UWebSocketGate: Технические детали
           Вся релизация построена на "однопоточном" eventloop. Если датчики долго не меняются, то периодически посылается "ping" сообщение.

        \section sec_UWebSocketGate_Command Команды
        Через websocket можно посылать команды.
        На текущий момент формат команды строковый.
        Т.е. для подачи команды, необходимо послать просто строку.
        Поддерживаются следующие команды:

        - "set:id1=val1,id2=val2,name3=val4,..." - выставить значение датчиков
        - "ask:id1,id2,name3,..." - подписаться на уведомления об изменении датчиков (sensorInfo)
        - "del:id1,id2,name3,..." - отказаться от уведомления об изменении датчиков

        \todo Разобраться с "ping" сообщением для формата json..
        \todo Настройка check_sec из командной строки и configure.xml
    */
    class UWebSocketGate:
        public UniSetObject,
        public EventLoopServer
#ifndef DISABLE_REST_API
        , public Poco::Net::HTTPRequestHandler
#endif
    {
        public:
            UWebSocketGate( uniset::ObjectId id, xmlNode* cnode, const std::string& prefix );
            virtual ~UWebSocketGate();

            /*! глобальная функция для инициализации объекта */
            static std::shared_ptr<UWebSocketGate> init_wsgate( int argc, const char* const* argv, const std::string& prefix = "logdb-" );

            /*! глобальная функция для вывода help-а */
            static void help_print();

            inline std::shared_ptr<DebugStream> log()
            {
                return mylog;
            }

#ifndef DISABLE_REST_API
            virtual void handleRequest( Poco::Net::HTTPServerRequest& req, Poco::Net::HTTPServerResponse& resp ) override;
            void onWebSocketSession( Poco::Net::HTTPServerRequest& req, Poco::Net::HTTPServerResponse& resp );
#endif

        protected:

            class UWebSocket;

            virtual bool activateObject() override;
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
            ev::sig sigTERM;
            ev::sig sigQUIT;
            ev::sig sigINT;
            void onTerminate( ev::sig& evsig, int revents );

            ev::async wsactivate; // активация WebSocket-ов
            std::shared_ptr<ev::async> wscmd;

            void checkMessages( ev::timer& t, int revents );
            virtual void sensorInfo( const uniset::SensorMessage* sm ) override;
            ev::timer iocheck;
            double check_sec = { 0.3 };

            std::shared_ptr<DebugStream> mylog;

#ifndef DISABLE_REST_API
            std::shared_ptr<Poco::Net::HTTPServer> httpserv;
            std::string httpHost = { "" };
            int httpPort = { 0 };
            std::string httpCORS_allow = { "*" };

            double wsHeartbeatTime_sec = { 3.0 };
            double wsSendTime_sec = { 0.5 };
            size_t wsMaxSend = { 200 };

            enum class RespondFormat
            {
                UNKNOWN,
                JSON,
                TXT,
                RAW
            };

            RespondFormat from_string( const std::string& str );

            static UTCPCore::Buffer* format( const uniset::SensorMessage* sm, const std::string& err, const RespondFormat fmt );
            static UTCPCore::Buffer* to_json( const uniset::SensorMessage* sm, const std::string& err );
            static UTCPCore::Buffer* to_txt( const uniset::SensorMessage* sm, const std::string& err );
            static UTCPCore::Buffer* to_raw( const uniset::SensorMessage* sm, const std::string& err );

            /*! класс реализует работу с websocket через eventloop
             * Из-за того, что поступление логов может быть достаточно быстрым
             * чтобы не "завалить" браузер кучей сообщений,
             * сделана посылка не по факту приёма сообщения, а раз в send_sec,
             * не более maxsend сообщений.
             * \todo websocket: может стоит объединять сообщения в одну посылку (пока считаю преждевременной оптимизацией)
             */
            class UWebSocket:
                public Poco::Net::WebSocket
            {
                public:
                    UWebSocket( Poco::Net::HTTPServerRequest* req,
                                Poco::Net::HTTPServerResponse* resp);

                    virtual ~UWebSocket();

                    bool isActive();
                    void set( ev::dynamic_loop& loop, std::shared_ptr<ev::async> a );

                    void send( ev::timer& t, int revents );
                    void ping( ev::timer& t, int revents );
                    void read( ev::io& io, int revents );

                    struct sinfo
                    {
                        std::string err; // ошибка при работе с датчиком (например при заказе)
                        uniset::ObjectId id = { uniset::DefaultObjectId };
                        std::string cmd = "";
                        long value = { 0 }; // set value
                    };

                    void add( const sinfo& si );
                    void del( uniset::ObjectId id );
                    void set( uniset::ObjectId id, long value );
                    void sensorInfo( const uniset::SensorMessage* sm );
                    void doCommand( const std::shared_ptr<UInterface>& ui );

                    void term();

                    void waitCompletion();

                    // настройка
                    void setHearbeatTime( const double& sec );
                    void setSendPeriod( const double& sec );
                    void setMaxSendCount( size_t val );
                    void setRespondFormat( RespondFormat f );

                    std::shared_ptr<DebugStream> mylog;

                protected:

                    void write();
                    void sendError( sinfo& si, const std::string& err );
                    void onCommand( const std::string& cmd );

                    ev::timer iosend;
                    double send_sec = { 0.5 };
                    size_t maxsend = { 200 };

                    ev::timer ioping;
                    double ping_sec = { 3.0 };
                    static const std::string ping_str;

                    ev::io iorecv;
                    char rbuf[512]; //! \todo сделать настраиваемым или применить Poco::FIFOBuffer
                    timeout_t recvTimeout = { 200 }; // msec
                    std::shared_ptr<ev::async> cmdsignal;

                    std::mutex              finishmut;
                    std::condition_variable finish;

                    std::atomic_bool cancelled = { false };

                    std::unordered_map<uniset::ObjectId, sinfo> smap;
                    RespondFormat fmt = { RespondFormat::JSON };

                    Poco::Net::HTTPServerRequest* req;
                    Poco::Net::HTTPServerResponse* resp;

                    // очередь данных на посылку..
                    std::queue<UTCPCore::Buffer*> wbuf;
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
