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
#include <unistd.h>

#include "unisetstd.h"
#include <Poco/Net/NetException.h>
#include <Poco/Net/WebSocket.h>
#include <Poco/Net/ServerSocket.h>
#include "ujson.h"
#include "UWebSocketGate.h"
#include "Configuration.h"
#include "Exceptions.h"
#include "UHelpers.h"
#include "Debug.h"
#include "UniXML.h"
#include "ORepHelpers.h"
#include "UWebSocketGateSugar.h"
#include "SMonitor.h"
#include "UTCPSocket.h"
// --------------------------------------------------------------------------
using namespace uniset;
using namespace std;
// --------------------------------------------------------------------------
UWebSocketGate::UWebSocketGate( uniset::ObjectId id
                                , xmlNode* cnode
                                , uniset::ObjectId shmID
                                , const std::shared_ptr<SharedMemory>& ic
                                , const string& prefix ):
    UniSetObject(id)
{
    offThread(); // отключаем поток обработки, потому-что будем обрабатывать сами

    auto conf = uniset_conf();

    mylog = make_shared<DebugStream>();
    mylog->setLogName(myname);
    {
        ostringstream s;
        s << prefix << "log";
        conf->initLogStream(mylog, s.str());
    }

    if( mylog->verbose() == 0 )
        mylog->verbose(1);

    loga = make_shared<LogAgregator>(myname + "-loga");
    loga->add(mylog);
    loga->add(ulog());

    logserv = make_shared<LogServer>(loga);
    logserv->init( prefix + "logserver", cnode );

    UniXML::iterator it(cnode);

    int maxCacheSize = conf->getArgPInt("--" + prefix + "max-ui-cache-size", it.getProp("msgUICacheSize"), 5000);
    ui->setCacheMaxSize(maxCacheSize);

    jpoolCapacity = conf->getArgPInt("--" + prefix + "pool-capacity", it.getProp("poolCapacity"), jpoolCapacity);
    jpoolPeakCapacity = conf->getArgPInt("--" + prefix + "pool-peak-capacity", it.getProp("poolPeakCapacity"), jpoolPeakCapacity);

    shm = make_shared<SMInterface>(shmID, ui, getId(), ic);

    maxwsocks = conf->getArgPInt("--" + prefix + "max-conn", it.getProp("wsMaxConnection"), maxwsocks);

    wscmd = make_shared<ev::async>();
    wsactivate.set<UWebSocketGate, &UWebSocketGate::onActivate>(this);
    wscmd->set<UWebSocketGate, &UWebSocketGate::onCommand>(this);
    iocheck.set<UWebSocketGate, &UWebSocketGate::checkMessages>(this);

    maxMessagesProcessing = conf->getArgPInt("--" + prefix + "max-messages-processing", conf->getField("maxMessagesProcessing"), maxMessagesProcessing);

    if( maxMessagesProcessing < 0 )
        maxMessagesProcessing = 100;

    check_sec = (float)conf->getArgPInt("--" + prefix + "msg-check-time", it.getProp("msgCheckTime"), int(check_sec * 1000.0)) / 1000.0;
    int sz = conf->getArgPInt("--uniset-object-size-message-queue", conf->getField("SizeOfMessageQueue"), 10000);

    if( sz > 0 )
        setMaxSizeOfMessageQueue(sz);

    if( findArgParam("--" + prefix + "run-logserver", conf->getArgc(), conf->getArgv()) != -1 )
    {
        logserv_host = conf->getArg2Param("--" + prefix + "logserver-host", it.getProp("logserverHost"), "localhost");
        logserv_port = conf->getArgPInt("--" + prefix + "logserver-port", it.getProp("logserverPort"), getId());
    }

#ifndef DISABLE_REST_API
    wsHeartbeatTime_sec = (float)conf->getArgPInt("--" + prefix + "heartbeat-time", it.getProp("wsHeartbeatTimeTime"), int(wsHeartbeatTime_sec * 1000)) / 1000.0;
    wsSendTime_sec = (float)conf->getArgPInt("--" + prefix + "send-time", it.getProp("wsSendTime"), int(wsSendTime_sec * 1000.0)) / 1000.0;
    wsMaxSend = conf->getArgPInt("--" + prefix + "max-send", it.getProp("wsMaxSend"), wsMaxSend);
    wsMaxCmd = conf->getArgPInt("--" + prefix + "max-cmd", it.getProp("wsMaxCmd"), wsMaxCmd);
    wsPongTimeout_sec = conf->getArgPInt("--" + prefix + "pong-timeout", it.getProp("wsPongTimeout_sec"), wsPongTimeout_sec);
    wsMaxLifetime_sec = (float)conf->getArgPInt("--" + prefix + "max-lifetime", it.getProp("wsMaxLifetime"), 0) / 1000.0;

    myinfoV(1) << myname << "(init): maxSend=" << wsMaxSend
               << " maxCmd=" << wsMaxCmd
               << " maxConnections=" << maxwsocks
               << endl;

    httpHost = conf->getArgParam("--" + prefix + "host", it.getProp2("host", "localhost"));
    httpPort = conf->getArgPInt("--" + prefix + "port", it.getPIntProp("port", 8081));
    httpCORS_allow = conf->getArgParam("--" + prefix + "cors-allow", "*");

    myinfoV(1) << myname << "(init): http server parameters " << httpHost << ":" << httpPort << endl;
    Poco::Net::SocketAddress sa(httpHost, httpPort);

    ostringstream lockfile;
    lockfile << conf->getLockDir() << "ws" << id;

    myinfoV(1) << myname << "(init): lockfile " << lockfile.str() << endl;

    runlock = unisetstd::make_unique<uniset::RunLock>(lockfile.str());

    if( runlock->isLocked() )
    {
        ostringstream err;
        err << myname << "(init): lock failed! UWebSocketGate id=" << id
            << " Already running!" << endl
            << " ..or delete the lockfile " << lockfile.str() << " to run..";
        throw uniset::SystemError(err.str());
    }

    if( !runlock->lock() )
    {
        ostringstream err;
        err << myname << "(init): failed create lockfile " << lockfile.str();
        throw uniset::SystemError(err.str());
    }

    // check create socket
    try
    {
        myinfoV(4) << myname << "(init): check socket create " << httpHost << ":" << httpPort << endl;
        Poco::Net::ServerSocket ss;
        ss.bind(sa);
        ss.close();
    }
    catch( std::exception& ex )
    {
        ostringstream err;
        err << myname << "(init): create socket " << httpHost << ":" << httpPort << " error or already use..";
        throw uniset::SystemError(err.str());
    }

    try
    {
        Poco::Net::HTTPServerParams* httpParams = new Poco::Net::HTTPServerParams;

        int maxQ = conf->getArgPInt("--" + prefix + "max-queued", it.getProp("maxQueued"), 100);
        int maxT = conf->getArgPInt("--" + prefix + "max-threads", it.getProp("maxThreads"), 3);

        httpParams->setMaxQueued(maxQ);
        httpParams->setMaxThreads(maxT);
        httpserv = std::make_shared<Poco::Net::HTTPServer>(new UWebSocketGateRequestHandlerFactory(this), Poco::Net::ServerSocket(sa), httpParams );
    }
    catch( std::exception& ex )
    {
        std::stringstream err;
        err << myname << "(init): " << httpHost << ":" << httpPort << " ERROR: " << ex.what();
        throw uniset::SystemError(err.str());
    }

#endif
}
//--------------------------------------------------------------------------------------------
UWebSocketGate::~UWebSocketGate()
{
    if( evIsActive() )
        evstop();

    if( httpserv )
        httpserv->stop();

    if( runlock )
        runlock->unlock();
}
//--------------------------------------------------------------------------------------------
void UWebSocketGate::terminate()
{
    myinfo << myname << "(terminate): terminate..." << endl;

    try
    {
        httpserv->stop();
        httpserv = nullptr;
    }
    catch( std::exception& ex )
    {
        myinfo << myname << "(terminate): http server: "  << ex.what() << endl;
    }

    try
    {
        if( evIsActive() )
            evstop();
    }
    catch( std::exception& ex )
    {
        myinfo << myname << "(terminate): evstop: "  << ex.what() << endl;
    }
}
//--------------------------------------------------------------------------------------------
void UWebSocketGate::checkMessages( ev::timer& t, int revents )
{
    if( EV_ERROR & revents )
        return;

    for( int i = 0; i < maxMessagesProcessing; i++ )
    {
        auto m = receiveMessage();

        if( !m )
            break;

        processingMessage(m.get());
    }
}
//--------------------------------------------------------------------------------------------
void UWebSocketGate::sensorInfo( const SensorMessage* sm )
{
    myinfoV(5) << myname << "(sensorInfo): sid=" << sm->id << " val=" << sm->value << endl;

    // Оптимизация: используем READ lock и копируем shared_ptr-ы для минимизации времени блокировки
    std::vector<std::shared_ptr<UWebSocket>> local;

    {
        uniset_rwmutex_rlock lock(wsocksMutex);  // READ lock вместо WRITE
        local.reserve(wsocks.size());

        for( auto&& s : wsocks )
            local.push_back(s);
    }

    // Работаем с копией списка без удержания lock
    for( auto&& s : local )
        s->sensorInfo(sm);

    // если нет действующих сокетов, датчик уже не нужный и надо отказаться
    if( local.empty() )
    {
        try
        {
            ui->askSensor(sm->id, UniversalIO::UIODontNotify);
        }
        catch( std::exception& ex )
        {
            mycrit << "(sensorInfo): " << ex.what() << endl;
        }
    }
}
//--------------------------------------------------------------------------------------------
uniset::SimpleInfo* UWebSocketGate::getInfo( const char* userparam )
{
    uniset::SimpleInfo_var i = UniSetObject::getInfo(userparam);

    ostringstream inf;

    inf << i->info << endl;
    //  inf << vmon.pretty_str() << endl;
    inf << endl;
    inf << "LogServer:  " << logserv_host << ":" << logserv_port << endl;

    {
        uniset_rwmutex_wrlock lock(wsocksMutex);
        inf << "websockets[" << wsocks.size() << "]: " << endl;

        for( auto&& s : wsocks )
        {
            inf << "  " << s->getInfo() << endl;
        }
    }

    i->info = inf.str().c_str();
    return i._retn();
}
//--------------------------------------------------------------------------------------------
void UWebSocketGate::UWebSocket::fill_short_json( Poco::JSON::Object::Ptr& json, const std::shared_ptr<sinfo>& si )
{
    json->set("type", "ShortSensorInfo");
    json->set("error", si->err);
    json->set("id", si->id);
    json->set("value", si->value);
}
//--------------------------------------------------------------------------------------------
Poco::JSON::Object::Ptr UWebSocketGate::UWebSocket::to_short_json( const std::shared_ptr<sinfo>& si )
{
    Poco::JSON::Object::Ptr json = new Poco::JSON::Object();
    json->set("type", "ShortSensorInfo");
    json->set("error", si->err);
    json->set("id", si->id);
    json->set("value", si->value);
    return json;
}
//--------------------------------------------------------------------------------------------
void UWebSocketGate::UWebSocket::fill_json( Poco::JSON::Object::Ptr& json, const uniset::SensorMessage* sm, const std::shared_ptr<sinfo>& si )
{
    json->set("type", "SensorInfo");
    json->set("error", std::string(si->err));
    json->set("id", sm->id);
    json->set("value", sm->value);
    json->set("name", si->name);
    json->set("sm_tv_sec", sm->sm_tv.tv_sec);
    json->set("sm_tv_nsec", sm->sm_tv.tv_nsec);
    json->set("iotype", uniset::iotype2str(sm->sensor_type));
    json->set("undefined", sm->undefined );
    json->set("supplier", sm->supplier );
    json->set("tv_sec", sm->tm.tv_sec);
    json->set("tv_nsec", sm->tm.tv_nsec);
    json->set("node", sm->node);
    auto calibr = json->getObject("calibration");

    if( !calibr )
        calibr = uniset::json::make_child(json, "calibration");

    calibr->set("cmin", sm->ci.minCal);
    calibr->set("cmax", sm->ci.maxCal);
    calibr->set("rmin", sm->ci.minRaw);
    calibr->set("rmax", sm->ci.maxRaw);
    calibr->set("precision", sm->ci.precision);
}
//--------------------------------------------------------------------------------------------
void UWebSocketGate::fill_error_json( Poco::JSON::Object::Ptr& json, std::string_view err )
{
    json->set("type", "Error");
    json->set("message", std::string(err));
}
//--------------------------------------------------------------------------------------------
Poco::JSON::Object::Ptr UWebSocketGate::UWebSocket::to_json( const SensorMessage* sm, const std::shared_ptr<sinfo>& si )
{
    Poco::JSON::Object::Ptr json = new Poco::JSON::Object();

    json->set("type", "SensorInfo");
    json->set("error", si->err);
    json->set("id", sm->id);
    json->set("value", sm->value);
    json->set("name", si->name);
    json->set("sm_tv_sec", sm->sm_tv.tv_sec);
    json->set("sm_tv_nsec", sm->sm_tv.tv_nsec);
    json->set("iotype", uniset::iotype2str(sm->sensor_type));
    json->set("undefined", sm->undefined );
    json->set("supplier", sm->supplier );
    json->set("tv_sec", sm->tm.tv_sec);
    json->set("tv_nsec", sm->tm.tv_nsec);
    json->set("node", sm->node);

    Poco::JSON::Object::Ptr calibr = uniset::json::make_child(json, "calibration");
    calibr->set("cmin", sm->ci.minCal);
    calibr->set("cmax", sm->ci.maxCal);
    calibr->set("rmin", sm->ci.minRaw);
    calibr->set("rmax", sm->ci.maxRaw);
    calibr->set("precision", sm->ci.precision);

    return json;
}
//--------------------------------------------------------------------------------------------
Poco::JSON::Object::Ptr UWebSocketGate::error_to_json( std::string_view err )
{
    Poco::JSON::Object::Ptr json = new Poco::JSON::Object();

    json->set("type", "Error");
    json->set("message", std::string(err));

    return json;
}
//--------------------------------------------------------------------------------------------
std::shared_ptr<UWebSocketGate> UWebSocketGate::init_wsgate( int argc, const char* const* argv
        , uniset::ObjectId shmID
        , const std::shared_ptr<SharedMemory>& ic
        , const std::string& prefix )
{
    string name = uniset::getArgParam("--" + prefix + "name", argc, argv, "UWebSocketGate");

    if( name.empty() )
    {
        cerr << "(UWebSocketGate): Unknown name. Use --" << prefix << "name" << endl;
        return nullptr;
    }

    return uniset::make_object<UWebSocketGate>(name, "UWebSocketGate", shmID, ic, prefix);
}
// -----------------------------------------------------------------------------
void UWebSocketGate::help_print()
{
    cout << "--ws-name name                      - Имя. Для поиска настроечной секции в configure.xml" << endl;
    cout << "--uniset-object-size-message-queue num  - Размер uniset-очереди сообщений. По умолчанию: 10000" << endl;
    cout << "--ws-msg-check-time msec            - Период опроса uniset-очереди сообщений, для обработки новых сообщений. По умолчанию: 10 мсек" << endl;
    cout << "--ws-max-messages-processing num    - Количество uniset-сообщений обрабатываемых за один раз. По умолчанию: 100" << endl;

    cout << "websockets: " << endl;
    cout << "--ws-max-conn num             - Максимальное количество одновременных подключений (клиентов). По умолчанию: 50" << endl;
    cout << "--ws-heartbeat-time msec      - Период сердцебиения в соединении. По умолчанию: 3000 мсек" << endl;
    cout << "--ws-send-time msec           - Период посылки сообщений. По умолчанию: 200 мсек" << endl;
    cout << "--ws-max-send num             - Максимальное число сообщений посылаемых за один раз. По умолчанию: 5000" << endl;
    cout << "--ws-max-cmd num              - Максимальное число команд обрабатываемых за один раз. По умолчанию: 200" << endl;
    cout << "--ws-pong-timeout msec        - Таймаут на ответ на сообщение ping. По умолчанию: 5000" << endl;
    cout << "--ws-max-lifetime msec        - Максимальное время жизни соединения. 0 = без ограничений. По умолчанию: 0" << endl;

    cout << "http: " << endl;
    cout << "--ws-host ip                  - IP на котором слушает http сервер. По умолчанию: localhost" << endl;
    cout << "--ws-port num                 - Порт на котором принимать запросы. По умолчанию: 8081" << endl;
    cout << "--ws-max-queued num           - Размер очереди запросов к http серверу. По умолчанию: 100" << endl;
    cout << "--ws-max-threads num          - Разрешённое количество потоков для http-сервера. По умолчанию: 3" << endl;
    cout << "--ws-cors-allow addr          - (CORS): Access-Control-Allow-Origin. Default: *" << endl;

    cout << "logs: " << endl;
    cout << "--ws-log-add-levels [crit,warn,info..]   - Уровень логов" << endl;
    cout << "--ws-log-verbosity N                     - Уровень подробностей [1...5]" << endl;
    cout << "  Пример параметров для запуска с подробными логами: " << endl;
    cout << "  --ws-log-add-levels any --ws-log-verbosity 5" << endl;
    cout << " LogServer: " << endl;
    cout << "--ws-run-logserver      - run logserver. Default: localhost:id" << endl;
    cout << "--ws-logserver-host ip  - listen ip. Default: localhost" << endl;
    cout << "--ws-logserver-port num - listen port. Default: ID" << endl;
}
// -----------------------------------------------------------------------------
void UWebSocketGate::run( bool async )
{
    if( httpserv )
        httpserv->start();

    if( async )
        async_evrun();
    else
        evrun();
}
// -----------------------------------------------------------------------------
void UWebSocketGate::evfinish()
{
    wsactivate.stop();
    iocheck.stop();
    wscmd->stop();
}
// -----------------------------------------------------------------------------
void UWebSocketGate::evprepare()
{
    wsactivate.set(loop);
    wsactivate.start();

    wscmd->set(loop);
    wscmd->start();

    iocheck.set(loop);
    iocheck.start(0, check_sec);
}
// -----------------------------------------------------------------------------
void UWebSocketGate::onActivate( ev::async& watcher, int revents )
{
    if (EV_ERROR & revents)
    {
        mycrit << myname << "(UWebSocketGate::onActivate): invalid event" << endl;
        return;
    }

    uniset_rwmutex_rlock lk(wsocksMutex);

    for( const auto& s : wsocks )
    {
        if( !s->isActive() )
        {
            s->set(loop, wscmd);
            s->doCommand(shm);
        }
    }
}
// -----------------------------------------------------------------------------
void UWebSocketGate::onCommand( ev::async& watcher, int revents )
{
    if (EV_ERROR & revents)
    {
        mycrit << myname << "(UWebSocketGate::onCommand): invalid event" << endl;
        return;
    }

    uniset_rwmutex_rlock lk(wsocksMutex);

    for( const auto& s : wsocks )
        s->doCommand(shm);
}
// -----------------------------------------------------------------------------
#ifndef DISABLE_REST_API
// -----------------------------------------------------------------------------
class UWebSocketGateRequestHandler:
    public Poco::Net::HTTPRequestHandler
{
    public:

        UWebSocketGateRequestHandler( UWebSocketGate* l ): wsgate(l) {}

        virtual void handleRequest( Poco::Net::HTTPServerRequest& request,
                                    Poco::Net::HTTPServerResponse& response ) override
        {
            wsgate->handleRequest(request, response);
        }

    private:
        UWebSocketGate* wsgate;
};
// -----------------------------------------------------------------------------
class UWebSocketGateWebSocketRequestHandler:
    public Poco::Net::HTTPRequestHandler
{
    public:

        UWebSocketGateWebSocketRequestHandler( UWebSocketGate* l ): wsgate(l) {}

        virtual void handleRequest( Poco::Net::HTTPServerRequest& request,
                                    Poco::Net::HTTPServerResponse& response ) override
        {
            wsgate->onWebSocketSession(request, response);
        }

    private:
        UWebSocketGate* wsgate;
};
// -----------------------------------------------------------------------------
Poco::Net::HTTPRequestHandler* UWebSocketGate::UWebSocketGateRequestHandlerFactory::createRequestHandler( const Poco::Net::HTTPServerRequest& req )
{
    if( req.find("Upgrade") != req.end() && Poco::icompare(req["Upgrade"], "websocket") == 0 )
        return new UWebSocketGateWebSocketRequestHandler(wsgate);

    return new UWebSocketGateRequestHandler(wsgate);
}
// -----------------------------------------------------------------------------
void UWebSocketGate::makeResponseAccessHeader( Poco::Net::HTTPServerResponse& resp )
{
    resp.set("Access-Control-Allow-Methods", "GET");
    resp.set("Access-Control-Allow-Request-Method", "*");
    resp.set("Access-Control-Allow-Origin", httpCORS_allow /* req.get("Origin") */);

    //  header('Access-Control-Allow-Credentials: true');
    //  header('Access-Control-Allow-Headers: Origin, X-Requested-With, Content-Type, Accept, Authorization');
}
// -----------------------------------------------------------------------------
void UWebSocketGate::handleRequest( Poco::Net::HTTPServerRequest& req, Poco::Net::HTTPServerResponse& resp )
{
    using Poco::Net::HTTPResponse;

    std::ostream& out = resp.send();

    makeResponseAccessHeader(resp);

    // В этой версии API поддерживается только GET
    if( req.getMethod() != "GET" )
    {
        auto jdata = respError(resp, HTTPResponse::HTTP_BAD_REQUEST, "method must be 'GET'");
        jdata->stringify(out);
        out.flush();
        return;
    }

    resp.setContentType("text/json");

    Poco::URI uri(req.getURI());

    myinfoV(3) << req.getHost() << ": query: " << uri.getQuery() << endl;

    std::vector<std::string> seg;
    uri.getPathSegments(seg);

    // проверка подключения к страничке со списком websocket-ов
    if( !seg.empty() && seg[0] == "wsgate" )
    {
        ostringstream params;
        auto qp = uri.getQueryParameters();

        int i = 0;

        for( const auto& p : qp )
        {
            if( i > 0 )
                params << "&";

            params << p.first;

            if( !p.second.empty() )
                params << "=" << p.second;

            i++;
        }

        httpWebSocketConnectPage(out, req, resp, params.str());
        out.flush();
        return;
    }

    // default page
    httpWebSocketPage(out, req, resp);
    out.flush();
}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr UWebSocketGate::respError( Poco::Net::HTTPServerResponse& resp,
        Poco::Net::HTTPResponse::HTTPStatus estatus,
        const string& message )
{
    makeResponseAccessHeader(resp);
    resp.setStatus(estatus);
    resp.setContentType("text/json");
    Poco::JSON::Object::Ptr jdata = new Poco::JSON::Object();
    jdata->set("error", resp.getReasonForStatus(resp.getStatus()));
    jdata->set("ecode", (int)resp.getStatus());
    jdata->set("message", message);
    return jdata;
}
// -----------------------------------------------------------------------------
void UWebSocketGate::onWebSocketSession( Poco::Net::HTTPServerRequest& req, Poco::Net::HTTPServerResponse& resp)
{
    using Poco::Net::WebSocket;
    using Poco::Net::WebSocketException;
    using Poco::Net::HTTPResponse;
    using Poco::Net::HTTPServerRequest;

    std::vector<std::string> seg;

    makeResponseAccessHeader(resp);

    Poco::URI uri(req.getURI());

    uri.getPathSegments(seg);

    myinfoV(3) << req.getHost() << ": WSOCKET: " << uri.getQuery() << endl;

    // example: ws://host:port/wsgate/?s1,s2,s3,s4
    if( seg.empty() || seg[0] != "wsgate" )
    {
        resp.setStatus(HTTPResponse::HTTP_BAD_REQUEST);
        resp.setContentType("text/html");
        resp.setStatusAndReason(HTTPResponse::HTTP_BAD_REQUEST);
        resp.setContentLength(0);
        std::ostream& err = resp.send();
        err << "Bad request. Must be:  ws://host:port/wsgate/?s1,s2,s3,s4";
        err.flush();
        return;
    }

    auto qp = uri.getQueryParameters();
    auto ws = newWebSocket(&req, &resp, qp);

    if( !ws )
    {
        mywarn << myname << "(onWebSocketSession): failed create socket.." << endl;
        return;
    }

    UWebSocketGuard lk(ws, this);

    myinfoV(3) << myname << "(onWebSocketSession): start session for " << req.clientAddress().toString() << endl;

    // т.к. вся работа происходит в eventloop
    // то здесь просто ждём..
    ws->waitCompletion();

    myinfoV(3) << myname << "(onWebSocketSession): finish session for " << req.clientAddress().toString() << endl;
}
// -----------------------------------------------------------------------------
bool UWebSocketGate::deactivateObject()
{
    terminate();

    if( logserv && logserv->isRunning() )
        logserv->terminate();

    return UniSetObject::deactivateObject();
}
// -----------------------------------------------------------------------------
bool UWebSocketGate::activateObject()
{
    bool ret = UniSetObject::activateObject();

    if( ret )
        run(true);

    return ret;
}
// -----------------------------------------------------------------------------
void UWebSocketGate::sysCommand( const uniset::SystemMessage* _sm )
{
    UniSetObject::sysCommand(_sm);

    switch( _sm->command )
    {
        case SystemMessage::WatchDog:
        case SystemMessage::StartUp:
        {
            try
            {
                if( !logserv_host.empty() && logserv_port != 0 && !logserv->isRunning() )
                {
                    myinfo << myname << "(sysCommand): run log server " << logserv_host << ":" << logserv_port << endl;
                    logserv->async_run(logserv_host, logserv_port);
                }
            }
            catch( std::exception& ex )
            {
                mywarn << myname << "(sysCommand): CAN`t run log server err: " << ex.what() << endl;
            }
            catch( ... )
            {
                mywarn << myname << "(sysCommand): CAN`t run log server err: catch ..." << endl;
            }

            break;
        }

        case SystemMessage::FoldUp:
        case SystemMessage::Finish:
            break;

        case SystemMessage::LogRotate:
        {
            if( logserv && !logserv_host.empty() && logserv_port != 0 )
            {
                try
                {
                    mylogany << myname << "(sysCommand): try restart logserver.." << endl;
                    logserv->check(true);
                }
                catch( std::exception& ex )
                {
                    mywarn << myname << "(sysCommand): CAN`t restart log server err: " << ex.what() << endl;
                }
                catch( ... )
                {
                    mywarn << myname << "(sysCommand): CAN`t restart log server err: catch ..." << endl;
                }
            }
        }
        break;

        default:
            break;
    }
}
// -----------------------------------------------------------------------------
std::shared_ptr<UWebSocketGate::UWebSocket> UWebSocketGate::newWebSocket( Poco::Net::HTTPServerRequest* req,
        Poco::Net::HTTPServerResponse* resp, const Poco::URI::QueryParameters& qp )
{
    using Poco::Net::WebSocket;
    using Poco::Net::WebSocketException;
    using Poco::Net::HTTPResponse;
    using Poco::Net::HTTPServerRequest;

    std::shared_ptr<UWebSocket> ws;

    std::string slist("");

    for( const auto& p : qp )
    {
        if( p.second.empty() && !p.first.empty() )
            slist += ("," + p.first);
    }

    if( qp.size() == 1 && qp[0].first.empty() )
        slist = qp[0].first;

    auto idlist = uniset::explode(slist);

    {
        uniset_rwmutex_wrlock lock(wsocksMutex);

        // Проверка лимита внутри критической секции для предотвращения race condition
        if( wsocks.size() >= maxwsocks )
        {
            resp->setStatus(HTTPResponse::HTTP_SERVICE_UNAVAILABLE);
            resp->setContentType("text/html");
            resp->setStatusAndReason(HTTPResponse::HTTP_SERVICE_UNAVAILABLE);
            resp->setContentLength(0);
            std::ostream& err = resp->send();
            err << "Error: exceeding the maximum number of open connections (" << maxwsocks << ")";
            err.flush();
            return nullptr;
        }

        ws = make_shared<UWebSocket>(req, resp, jpoolCapacity, jpoolPeakCapacity);
        ws->setHearbeatTime(wsHeartbeatTime_sec);
        ws->setSendPeriod(wsSendTime_sec);
        ws->setMaxSendCount(wsMaxSend);
        ws->setMaxCmdCount(wsMaxCmd);
        ws->setPongTimeout(wsPongTimeout_sec);
        ws->setMaxLifetime(wsMaxLifetime_sec);
        ws->mylog = mylog;

        for( const auto& i : idlist.ref() )
        {
            myinfoV(3) << myname << ": ask sid=" << i << endl;
            ws->ask(i);
        }

        wsocks.emplace_back(ws);
    }

    // wsocksMutex надо отпустить, прежде чем посылать сигнал
    // т.к. в обработчике происходит его захват
    wsactivate.send();
    return ws;
}
// -----------------------------------------------------------------------------
void UWebSocketGate::delWebSocket( std::shared_ptr<UWebSocket>& ws )
{
    uniset_rwmutex_wrlock lock(wsocksMutex);

    for( auto it = wsocks.begin(); it != wsocks.end(); it++ )
    {
        if( (*it).get() == ws.get() )
        {
            myinfoV(3) << myname << ": delete websocket " << endl;
            wsocks.erase(it);
            return;
        }
    }
}
// -----------------------------------------------------------------------------

UWebSocketGate::UWebSocket::UWebSocket( Poco::Net::HTTPServerRequest* _req,
                                        Poco::Net::HTTPServerResponse* _resp,
                                        int jpoolCapacity,
                                        int jpoolPeakCapacity ):
    Poco::Net::WebSocket(*_req, *_resp),
    req(_req),
    resp(_resp)
{
    memset(rbuf, 0, sizeof(rbuf));
    setBlocking(false);

    cancelled = false;

    // т.к. создание websocket-а происходит в другом потоке
    // то активация и привязка к loop происходит в функции set()
    // вызываемой из eventloop
    ioping.set<UWebSocketGate::UWebSocket, &UWebSocketGate::UWebSocket::ping>(this);
    iosend.set<UWebSocketGate::UWebSocket, &UWebSocketGate::UWebSocket::send>(this);
    iorecv.set<UWebSocketGate::UWebSocket, &UWebSocketGate::UWebSocket::read>(this);
    iopong.set<UWebSocketGate::UWebSocket, &UWebSocketGate::UWebSocket::pong>(this);
    iolifetime.set<UWebSocketGate::UWebSocket, &UWebSocketGate::UWebSocket::onLifetimeExpired>(this);
    sessionStart = std::chrono::steady_clock::now();

    maxsize = maxsend * Kbuf;

    setReceiveTimeout(uniset::PassiveTimer::millisecToPoco(recvTimeout));
    setMaxPayloadSize(sizeof(rbuf));
    setKeepAlive(true);

    jpoolSM = make_unique<Poco::ObjectPool< Poco::JSON::Object, Poco::JSON::Object::Ptr >>(jpoolCapacity, jpoolPeakCapacity);
    jpoolErr = make_unique<Poco::ObjectPool< Poco::JSON::Object, Poco::JSON::Object::Ptr >>(jpoolCapacity, jpoolPeakCapacity);
    jpoolShortSM = make_unique<Poco::ObjectPool< Poco::JSON::Object, Poco::JSON::Object::Ptr >>(jpoolCapacity, jpoolPeakCapacity);
    wbufpool = make_unique<Poco::ObjectPool< uniset::UTCPCore::Buffer >>(jpoolCapacity, jpoolPeakCapacity);
}
// -----------------------------------------------------------------------------
UWebSocketGate::UWebSocket::~UWebSocket()
{
    if( !cancelled )
        term();

    // удаляем всё что осталось
    while( !wbuf.empty() )
    {
        auto b = wbuf.front();
        wbuf.pop();
        wbufpool->returnObject(b);
    }

    while( !jbuf.empty() )
    {
        auto json = jbuf.front();
        jbuf.pop();
        returnObjectToPool(json);
    }

    while( !qcmd.empty() )
        qcmd.pop();
}
// -----------------------------------------------------------------------------
void UWebSocketGate::UWebSocket::returnObjectToPool( Poco::JSON::Object::Ptr& json )
{
    if( !json )
        return;

    auto stype = json->get("type").toString();

    if( stype == "Error" )
        jpoolErr->returnObject(json);
    else if( stype == "SensorInfo" )
        jpoolSM->returnObject(json);
    else if( stype == "ShortSensorInfo" )
        jpoolShortSM->returnObject(json);
}
// -----------------------------------------------------------------------------
std::string UWebSocketGate::UWebSocket::getInfo() const noexcept
{
    ostringstream inf;

    inf << req->clientAddress().toString()
        << ": jbuf=" << jbuf.size()
        << " wbuf=" << wbuf.size()
        << " ping_sec=" << ping_sec
        << " pongTimeout_sec=" << pongTimeout_sec
        << " maxsend=" << maxsend
        << " send_sec=" << send_sec
        << " maxcmd=" << maxcmd;

    return inf.str();
}
// -----------------------------------------------------------------------------
bool UWebSocketGate::UWebSocket::isActive()
{
    return iosend.is_active();
}
// -----------------------------------------------------------------------------
void UWebSocketGate::UWebSocket::set( ev::dynamic_loop& loop, std::shared_ptr<ev::async> a )
{
    iosend.set(loop);
    ioping.set(loop);
    iopong.set(loop);
    iolifetime.set(loop);

    iosend.start(0, send_sec);
    ioping.start(ping_sec, ping_sec);

    if( maxLifetime_sec > 0 )
        iolifetime.start(maxLifetime_sec);

    iorecv.set(loop);
    iorecv.start(sockfd(), ev::READ);

    cmdsignal = a;
}
// -----------------------------------------------------------------------------
void UWebSocketGate::UWebSocket::send( ev::timer& t, int revents )
{
    if( EV_ERROR & revents )
        return;

    std::queue<Poco::JSON::Object::Ptr> local;

    {
        std::lock_guard<std::mutex> lk(dataMutex);

        if( !jbuf.empty() )
            std::swap(local, jbuf);
    }

    while( !local.empty() && !cancelled )
    {
        // сперва формируем очередной пакет(поток байт) из накопившихся данных для отправки
        ostringstream out;
        out.rdbuf()->pubsetbuf(sbuf, sbufLen);

        out << "{\"data\":[";

        size_t i = 0;

        for( ; !local.empty() && !cancelled; i++ )
        {
            if( i > 0 )
                out << ",";

            auto json = local.front();
            local.pop();

            if( !json )
                continue;

            json->stringify(out);
            returnObjectToPool(json);

            if( out.tellp() >= sbufLim )
            {
                myinfoV(4) << req->clientAddress().toString() << "(write): buffer limit[" << sbufLim << "]: " << i << " objects" << endl;
                break;
            }
        }

        out << "]}";

        auto b = wbufpool->borrowObject();
        b->reset((unsigned char*)sbuf, out.tellp());
        wbuf.emplace(b);

        myinfoV(4) << req->clientAddress().toString() << "(write): batch " << i << " objects" << endl;
    }

    // реальная посылка данных
    for( size_t i = 0; !wbuf.empty() && i < maxsend && !cancelled; i++ )
    {
        write();
    }
}
// -----------------------------------------------------------------------------
const std::string UWebSocketGate::UWebSocket::ping_str = "."; // "{\"data\": [{\"type\": \"Ping\"}]}";

void UWebSocketGate::UWebSocket::ping( ev::timer& t, int revents )
{
    if( EV_ERROR & revents )
        return;

    if( cancelled )
        return;

    if( !wbuf.empty() )
    {
        ioping.stop();
        return;
    }

    auto b = wbufpool->borrowObject();
    b->reset(ping_str);
    wbuf.emplace(b);

    if( ioping.is_active() )
        ioping.stop();
}
// -----------------------------------------------------------------------------
void UWebSocketGate::UWebSocket::pong( ev::timer& t, int revents )
{
    if( EV_ERROR & revents )
        return;

    if( cancelled )
        return;

    if( pongCounter > 1 )
    {
        myinfoV(4) << req->clientAddress().toString() << "(pong): pong timeout " << pongTimeout_sec
                   << " sec. Terminate session.." << endl;
        term();
        return;
    }
}
// -----------------------------------------------------------------------------
void UWebSocketGate::UWebSocket::read( ev::io& io, int revents )
{
    if( EV_ERROR & revents )
        return;

    if( !(revents & EV_READ) )
        return;

    if( cancelled )
        return;

    using Poco::Net::WebSocket;
    using Poco::Net::WebSocketException;
    using Poco::Net::HTTPResponse;
    using Poco::Net::HTTPServerRequest;

    int flags = 0; // WebSocket::FRAME_FLAG_FIN | WebSocket::FRAME_OP_TEXT;

    try
    {
        if( available() <= 0 )
            return;

        int n = receiveFrame(rbuf, sizeof(rbuf), flags);

        if( n >= 0 )
        {
            if( (flags & WebSocket::FRAME_OP_BITMASK) == WebSocket::FRAME_OP_CLOSE )
            {
                term();
                return;
            }

            if( (flags & WebSocket::FRAME_OP_BITMASK) == WebSocket::FRAME_OP_PING )
            {
                myinfoV(4) << req->clientAddress().toString() << "(read): ping request => send pong" << endl;
                sendFrame(rbuf, n, WebSocket::FRAME_FLAG_FIN | WebSocket::FRAME_OP_PONG);
                return;
            }

            if( (flags & WebSocket::FRAME_OP_BITMASK) & WebSocket::FRAME_OP_PONG )
            {
                myinfoV(4) << req->clientAddress().toString() << "(read): pong.." << endl;
                iopong.stop();
                pongCounter = 0;
                return;
            }
        }

        if( n <= 0 )
            return;

        if( n == sizeof(rbuf) )
        {
            ostringstream err;
            err << "Payload too big. Must be < " << sizeof(rbuf) << " bytes";
            sendError(err.str());
            mycritV(4) << req->clientAddress().toString() << "(read): error: " << err.str() << endl;
            return;
        }

        const std::string_view cmd(rbuf, n);
        onCommand(cmd);

        // откладываем ping, т.к. что-то в канале и так было
        ioping.start(ping_sec, ping_sec);
    }
    catch( WebSocketException& exc )
    {
        switch( exc.code() )
        {
            case WebSocket::WS_ERR_HANDSHAKE_UNSUPPORTED_VERSION:
                resp->set("Sec-WebSocket-Version", WebSocket::WEBSOCKET_VERSION);

            case WebSocket::WS_ERR_NO_HANDSHAKE:
            case WebSocket::WS_ERR_HANDSHAKE_NO_VERSION:
            case WebSocket::WS_ERR_HANDSHAKE_NO_KEY:
                resp->setStatusAndReason(HTTPResponse::HTTP_BAD_REQUEST);
                resp->setContentLength(0);
                resp->send();
                break;
        }

        myinfoV(3) << "(websocket): WebSocketException: "
                   << req->clientAddress().toString()
                   << " error: " << exc.displayText()
                   << endl;
    }
    catch( const Poco::Net::NetException& e )
    {
        myinfoV(3) << "(websocket):NetException: "
                   << req->clientAddress().toString()
                   << " error: " << e.displayText()
                   << endl;
    }
    catch( Poco::IOException& ex )
    {
        myinfoV(3) << "(websocket): IOException: "
                   << req->clientAddress().toString()
                   << " error: " << ex.displayText()
                   << endl;
    }
    catch( Poco::TimeoutException& ex )
    {
        // it is ok
    }
    catch( std::exception& ex )
    {
        myinfoV(3) << "(websocket): std::exception: "
                   << req->clientAddress().toString()
                   << " error: " << ex.what()
                   << endl;
    }
}
// -----------------------------------------------------------------------------
void UWebSocketGate::UWebSocket::ask( uniset::ObjectId id )
{
    qcmd.push(std::make_shared<sinfo>("ask", id));
}
// -----------------------------------------------------------------------------
void UWebSocketGate::UWebSocket::del( uniset::ObjectId id )
{
    qcmd.push(std::make_shared<sinfo>("del", id));
}
// -----------------------------------------------------------------------------
void UWebSocketGate::UWebSocket::set( uniset::ObjectId id, long value )
{
    auto s = std::make_shared<sinfo>("set", id);
    s->value = value;
    qcmd.push(s);
}
// -----------------------------------------------------------------------------
void UWebSocketGate::UWebSocket::freeze( uniset::ObjectId id, long value )
{
    auto s = std::make_shared<sinfo>("freeze", id);
    s->value = value;
    qcmd.push(s);
}
// -----------------------------------------------------------------------------
void UWebSocketGate::UWebSocket::unfreeze( uniset::ObjectId id )
{
    auto s = std::make_shared<sinfo>("unfreeze", id);
    qcmd.push(s);
}
// -----------------------------------------------------------------------------
void UWebSocketGate::UWebSocket::get( uniset::ObjectId id )
{
    qcmd.push(std::make_shared<sinfo>("get", id));
}
// -----------------------------------------------------------------------------
void UWebSocketGate::UWebSocket::sensorInfo( const uniset::SensorMessage* sm )
{
    if( cancelled )
        return;

    {
        std::lock_guard<std::mutex> lk(dataMutex);

        auto s = smap.find(sm->id);

        if( s == smap.end() )
            return;

        if( jbuf.size() > maxsize )
        {
            mywarn << req->clientAddress().toString() << " lost messages...(maxsize=" << maxsize << ")" << endl;
            return;
        }

        auto j = jpoolSM->borrowObject();
        fill_json(j, sm, s->second);
        jbuf.emplace(j);
    }

    if( ioping.is_active() )
        ioping.stop();
}
// -----------------------------------------------------------------------------
void UWebSocketGate::UWebSocket::doCommand( const std::shared_ptr<SMInterface>& ui )
{
    if( qcmd.empty() )
        return;

    // копируем пачку команд, чтобы не держать mutex на время долгих операций с ui
    std::vector<std::shared_ptr<sinfo>> local;
    {
        std::lock_guard<std::mutex> lk(dataMutex);

        for( size_t i = 0; i < maxcmd && !qcmd.empty(); i++ )
        {
            local.push_back(qcmd.front());
            qcmd.pop();
        }
    }

    for( const auto& s : local )
    {
        try
        {
            if( s->cmd.empty() )
                continue;

            myinfoV(3) << req->clientAddress().toString() << "(doCommand): "
                       << s->cmd
                       << " sid=" << s->id << " value=" << s->value
                       << endl;

            if( s->cmd == "ask" )
            {
                if( s->name.empty() )
                    s->name = uniset::ORepHelpers::getShortName(uniset_conf()->oind->getMapName(s->id));

                ui->askSensor(s->id, UniversalIO::UIONotify);
                std::lock_guard<std::mutex> lk(dataMutex);
                smap[s->id] = s;
            }
            else if( s->cmd == "del" )
            {
                ui->askSensor(s->id, UniversalIO::UIODontNotify);
                std::lock_guard<std::mutex> lk(dataMutex);
                auto it = smap.find(s->id);

                if( it != smap.end() )
                    smap.erase(it);
            }
            else if( s->cmd == "set" )
            {
                ui->setValue(s->id, s->value);
            }
            else if( s->cmd == "freeze" )
            {
                ui->freezeValue(s->id, true, s->value, ui->ID());
            }
            else if( s->cmd == "unfreeze" )
            {
                ui->freezeValue(s->id, false, 0, ui->ID());
            }
            else if( s->cmd == "get" )
            {
                if( s->name.empty() )
                    s->name = uniset::ORepHelpers::getShortName(uniset_conf()->oind->getMapName(s->id));

                s->value = ui->getValue(s->id);
                s->err = "";
                sendShortResponse(s);
            }

            s->err = "";
            s->cmd = "";
        }
        catch( std::exception& ex )
        {
            mycrit << "(UWebSocket::doCommand): " << ex.what() << endl;

            if( s->name.empty() )
                s->name = uniset::ORepHelpers::getShortName(uniset_conf()->oind->getMapName(s->id));

            s->err = ex.what();
            sendResponse(s);
        }
    }

    if( !qcmd.empty() && cmdsignal )
        cmdsignal->send();
}
// -----------------------------------------------------------------------------
void UWebSocketGate::UWebSocket::sendShortResponse( const std::shared_ptr<sinfo>& si )
{
    if( jbuf.size() > maxsize )
    {
        mywarn << req->clientAddress().toString() << "(sendShortResponse): lost messages (maxsize=" << maxsize << ")" << endl;
        return;
    }

    {
        std::lock_guard<std::mutex> lk(dataMutex);
        auto j = jpoolShortSM->borrowObject();
        fill_short_json(j, si);
        jbuf.emplace(j);
    }

    if( ioping.is_active() )
        ioping.stop();
}
// -----------------------------------------------------------------------------
void UWebSocketGate::UWebSocket::sendResponse( const std::shared_ptr<sinfo>& si )
{
    if( jbuf.size() > maxsize )
    {
        mywarn << req->clientAddress().toString() << "(sendResponse): lost messages (maxsize=" << maxsize << ")"  << endl;
        return;
    }

    uniset::SensorMessage sm(si->id, si->value);
    {
        std::lock_guard<std::mutex> lk(dataMutex);
        auto j = jpoolSM->borrowObject();
        fill_json(j, &sm, si);
        jbuf.emplace(j);
    }

    if( ioping.is_active() )
        ioping.stop();
}
// -----------------------------------------------------------------------------
void UWebSocketGate::UWebSocket::sendError( std::string_view msg )
{
    if( jbuf.size() > maxsize )
    {
        mywarn << req->clientAddress().toString() << "(sendError): lost messages (maxsize=" << maxsize << ")"  << endl;
        return;
    }

    {
        std::lock_guard<std::mutex> lk(dataMutex);
        auto j = jpoolErr->borrowObject();
        fill_error_json(j, msg);
        jbuf.emplace(j);
    }

    if( ioping.is_active() )
        ioping.stop();
}
// -----------------------------------------------------------------------------
void UWebSocketGate::UWebSocket::onCommand( std::string_view cmdtxt )
{
    if( cmdtxt.size() < 5 )
    {
        myinfoV(3) << "(websocket): " << req->clientAddress().toString()
                   << " error: bad command format '" << cmdtxt << "'. Len must be > 4" << endl;

        sendError("Unknown command. Command must be > 4 bytes");
        return;
    }

    auto cpos = cmdtxt.find_first_of(':');

    if( cpos == string_view::npos )
    {
        myinfoV(3) << "(websocket): " << req->clientAddress().toString()
                   << " error: bad command format '" << cmdtxt << "'. Must be: 'cmd:params'" << endl;

        sendError("Bad command format. Command must be 'cmd:params'");
        return;
    }

    string_view cmd = cmdtxt.substr(0, cpos);
    string_view params = cmdtxt.substr(cpos + 1);

    if( cmd == "set" )
    {
        myinfoV(3) << "(websocket): " << req->clientAddress().toString()
                   << "(set): " << params << endl;

        auto idlist = uniset::getSInfoList_sv(params, uniset_conf());

        for( const auto& i : idlist )
            set(i.si.id, i.val);

        // уведомление о новой команде
        cmdsignal->send();
    }
    else if( cmd == "ask" )
    {
        myinfoV(3) << "(websocket): " << req->clientAddress().toString()
                   << "(ask): " << params << endl;

        auto idlist = uniset::split_by_id(params);

        for( const auto& id : idlist.ref() )
            ask(id);

        // уведомление о новой команде
        cmdsignal->send();
    }
    else if( cmd == "del" )
    {
        myinfoV(3) << "(websocket): " << req->clientAddress().toString()
                   << "(del): " << params << endl;

        auto idlist = uniset::split_by_id(params);

        for( const auto& id : idlist.ref() )
            del(id);

        // уведомление о новой команде
        cmdsignal->send();
    }
    else if( cmd == "get" )
    {
        myinfoV(3) << "(websocket): " << req->clientAddress().toString()
                   << "(get): " << params << endl;

        auto idlist = uniset::split_by_id(params);

        for( const auto& id : idlist.ref() )
            get(id);

        // уведомление о новой команде
        cmdsignal->send();
    }
    else if( cmd == "freeze" )
    {
        myinfoV(3) << "(websocket): " << req->clientAddress().toString()
                   << "(freeze): " << params << endl;

        auto idlist = uniset::getSInfoList_sv(params, uniset_conf());

        for( const auto& i : idlist )
            freeze(i.si.id, i.val);

        // уведомление о новой команде
        cmdsignal->send();
    }
    else if( cmd == "unfreeze" )
    {
        myinfoV(3) << "(websocket): " << req->clientAddress().toString()
                   << "(unfreeze): " << params << endl;

        auto idlist = uniset::split_by_id(params);

        for( const auto& id : idlist.ref() )
            unfreeze(id);

        // уведомление о новой команде
        cmdsignal->send();
    }
    else
    {
        mywarnV(3) << "(websocket)(command): " << req->clientAddress().toString()
                   << " UNKNOWN COMMAND '" << cmd << "'  params: " << params << endl;
    }
}
// -----------------------------------------------------------------------------
void UWebSocketGate::UWebSocket::write()
{
    if( wbuf.empty() )
    {
        if( !ioping.is_active() )
            ioping.start(ping_sec, ping_sec);

        return;
    }

    UTCPCore::Buffer* msg = wbuf.front();

    if( !msg )
    {
        wbuf.pop();
        return;
    }

    using Poco::Net::WebSocket;
    using Poco::Net::WebSocketException;
    using Poco::Net::HTTPResponse;
    using Poco::Net::HTTPServerRequest;

    int flags = WebSocket::FRAME_TEXT;

    if( msg->len == ping_str.size() )
    {
        flags = WebSocket::FRAME_FLAG_FIN | WebSocket::FRAME_OP_PING;
        iopong.start(pongTimeout_sec, pongTimeout_sec);
        pongCounter++;
        myinfoV(4) << req->clientAddress().toString() << "(write): ping.." << endl;
    }

    try
    {
        ssize_t ret = sendFrame(msg->dpos(), msg->nbytes(), flags);

        if( ret < 0 )
        {
            myinfoV(3) << "(websocket): " << req->clientAddress().toString()
                       << "  write to socket error(" << errno << "): " << strerror(errno) << endl;

            if( errno == EPIPE || errno == EBADF )
            {
                myinfoV(3) << "(websocket): "
                           << req->clientAddress().toString()
                           << " write error.. terminate session.." << endl;
                term();
            }

            return;
        }

        msg->pos += ret;

        if( msg->nbytes() == 0 )
        {
            wbuf.pop();
            wbufpool->returnObject(msg);
        }

        if( !wbuf.empty() )
        {
            if( ioping.is_active() )
                ioping.stop();
        }
        else
        {
            if( !ioping.is_active() )
                ioping.start(ping_sec, ping_sec);
        }

        return;
    }
    catch( WebSocketException& exc )
    {
        myinfoV(3) << "(sendFrame): ERROR: " << exc.displayText() << endl;

        switch( exc.code() )
        {
            case WebSocket::WS_ERR_HANDSHAKE_UNSUPPORTED_VERSION:
                resp->set("Sec-WebSocket-Version", WebSocket::WEBSOCKET_VERSION);

            case WebSocket::WS_ERR_NO_HANDSHAKE:
            case WebSocket::WS_ERR_HANDSHAKE_NO_VERSION:
            case WebSocket::WS_ERR_HANDSHAKE_NO_KEY:
                resp->setStatusAndReason(HTTPResponse::HTTP_BAD_REQUEST);
                resp->setContentLength(0);
                resp->send();
                break;
        }
    }
    catch( const Poco::Net::NetException& e )
    {
        myinfoV(3) << "(websocket):NetException: "
                   << req->clientAddress().toString()
                   << " error: " << e.displayText()
                   << endl;
    }
    catch( Poco::IOException& ex )
    {
        myinfoV(3) << "(websocket): IOException: "
                   << req->clientAddress().toString()
                   << " error: " << ex.displayText()
                   << endl;
    }
    catch( std::exception& ex )
    {
        myinfoV(3) << "(websocket): std::exception: "
                   << req->clientAddress().toString()
                   << " error: " << ex.what()
                   << endl;
    }

    term();
}
// -----------------------------------------------------------------------------
void UWebSocketGate::UWebSocket::term()
{
    if( cancelled )
        return;

    cancelled = true;
    ioping.stop();
    iosend.stop();
    iorecv.stop();
    iopong.stop();
    iolifetime.stop();
    finish.notify_all();
}
// -----------------------------------------------------------------------------
void UWebSocketGate::UWebSocket::waitCompletion()
{
    std::unique_lock<std::mutex> lk(finishmut);

    while( !cancelled )
        finish.wait(lk);
}
// -----------------------------------------------------------------------------
void UWebSocketGate::UWebSocket::setHearbeatTime( const double& sec )
{
    if( sec > 0 )
        ping_sec = sec;
}
// -----------------------------------------------------------------------------
void UWebSocketGate::UWebSocket::setSendPeriod ( const double& sec )
{
    if( sec > 0 )
        send_sec = sec;
}
// -----------------------------------------------------------------------------
void UWebSocketGate::UWebSocket::setMaxSendCount( size_t val )
{
    if( val > 0 )
    {
        maxsend = val;
        maxsize = maxsend * Kbuf;
    }
}
// -----------------------------------------------------------------------------
void UWebSocketGate::UWebSocket::setMaxCmdCount( size_t val )
{
    if( val > 0 )
        maxcmd = val;
}
// -----------------------------------------------------------------------------
void UWebSocketGate::UWebSocket::setPongTimeout( const double& val )
{
    if( val > 0 )
        pongTimeout_sec = val;
}
// -----------------------------------------------------------------------------
void UWebSocketGate::UWebSocket::setMaxLifetime( const double& sec )
{
    if( sec >= 0 )
        maxLifetime_sec = sec;
}
// -----------------------------------------------------------------------------
void UWebSocketGate::UWebSocket::onLifetimeExpired( ev::timer& t, int revents )
{
    if( EV_ERROR & revents )
        return;

    if( cancelled )
        return;

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - sessionStart).count();

    myinfoV(3) << req->clientAddress().toString()
               << "(onLifetimeExpired): max lifetime " << maxLifetime_sec
               << " sec reached (session " << elapsed << " sec). Terminate session.." << endl;
    term();
}
// -----------------------------------------------------------------------------
void UWebSocketGate::httpWebSocketPage( std::ostream& ostr, Poco::Net::HTTPServerRequest& req, Poco::Net::HTTPServerResponse& resp )
{
    using Poco::Net::HTTPResponse;

    resp.setChunkedTransferEncoding(true);
    resp.setContentType("text/html");
    //  resp.

    ostr << "<html>" << endl;
    ostr << "<head>" << endl;
    ostr << "<title>" << myname << ": test page</title>" << endl;
    ostr << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\">" << endl;
    ostr << "</head>" << endl;
    ostr << "<body>" << endl;
    ostr << "<h1>select sensors:</h1>" << endl;
    ostr << "<ul>" << endl;

    ostr << "  <li><a target='_blank' href=\"http://"
         << req.serverAddress().toString()
         << "/wsgate/?42,30,1042\">42,30,1042</a></li>"
         << endl;

    ostr << "</ul>" << endl;
    ostr << "</body>" << endl;
}
// -----------------------------------------------------------------------------
void UWebSocketGate::httpWebSocketConnectPage( ostream& ostr,
        Poco::Net::HTTPServerRequest& req,
        Poco::Net::HTTPServerResponse& resp,
        const std::string& params )
{
    resp.setChunkedTransferEncoding(true);
    resp.setContentType("text/html");

    // code base on example from
    // https://github.com/pocoproject/poco/blob/developNet/samples/WebSocketServer/src/WebSocketServer.cpp

    ostr << "<html>" << endl;
    ostr << "<head>" << endl;
    ostr << "<title>" << myname << ": sensors event</title>" << endl;
    ostr << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\">" << endl;
    ostr << "<script type=\"text/javascript\">" << endl;
    ostr << "logscrollStopped = false;" << endl;
    ostr << "" << endl;
    ostr << "function clickScroll()" << endl;
    ostr << "{" << endl;
    ostr << "	if( logscrollStopped )" << endl;
    ostr << "		logscrollStopped = false;" << endl;
    ostr << "	else" << endl;
    ostr << "		logscrollStopped = true;" << endl;
    ostr << "}" << endl;
    ostr << "function LogAutoScroll()" << endl;
    ostr << "{" << endl;
    ostr << "   if( logscrollStopped == false )" << endl;
    ostr << "   {" << endl;
    ostr << "	   document.getElementById('end').scrollIntoView();" << endl;
    ostr << "   }" << endl;
    ostr << "}" << endl;
    ostr << "" << endl;
    ostr << "function WebSocketCreate()" << endl;
    ostr << "{" << endl;
    ostr << "  if (\"WebSocket\" in window)" << endl;
    ostr << "  {" << endl;
    ostr << "    var ws = new WebSocket(\"ws://" << req.serverAddress().toString() << "/wsgate/?" << params << "\");" << endl;
    ostr << "setInterval(send_cmd, 1000);" << endl;
    ostr << "    var l = document.getElementById('logname');" << endl;
    ostr << "    l.innerHTML = '*'" << endl;
    ostr << "ws.onopen = function() {" << endl;
    //          ostr << "ws.send(\"set:33=44,344=45\")" << endl;
    ostr << "};" << endl;

    ostr << "    ws.onmessage = function(evt)" << endl;
    ostr << "    {" << endl;
    ostr << "    	var p = document.getElementById('logs');" << endl;
    ostr << "    	if( evt.data != '.' ) {" << endl;
    ostr << "    		p.innerHTML = p.innerHTML + \"</br>\"+evt.data" << endl;
    ostr << "    		LogAutoScroll();" << endl;
    ostr << "    	}" << endl;
    ostr << "    };" << endl;
    ostr << "    ws.onclose = function()" << endl;
    ostr << "      { " << endl;
    ostr << "        alert(\"WebSocket closed.\");" << endl;
    ostr << "      };" << endl;
    ostr << "  }" << endl;
    ostr << "  else" << endl;
    ostr << "  {" << endl;
    ostr << "     alert(\"This browser does not support WebSockets.\");" << endl;
    ostr << "  }" << endl;

    ostr << "function send_cmd() {" << endl;
    //    ostr << "  ws.send( 'set:12,32,34' );" << endl;
    ostr << "}" << endl;

    ostr << "}" << endl;

    ostr << "</script>" << endl;
    ostr << "<style media='all' type='text/css'>" << endl;
    ostr << ".logs {" << endl;
    ostr << "	font-family: 'Liberation Mono', 'DejaVu Sans Mono', 'Courier New', monospace;" << endl;
    ostr << "	padding-top: 30px;" << endl;
    ostr << "}" << endl;
    ostr << "" << endl;
    ostr << ".logtitle {" << endl;
    ostr << "	position: fixed;" << endl;
    ostr << "	top: 0;" << endl;
    ostr << "	left: 0;" << endl;
    ostr << "	padding: 10px;" << endl;
    ostr << "	width: 100%;" << endl;
    ostr << "	height: 25px;" << endl;
    ostr << "	background-color: green;" << endl;
    ostr << "	border-top: 2px solid;" << endl;
    ostr << "	border-bottom: 2px solid;" << endl;
    ostr << "	border-color: white;" << endl;
    ostr << "}" << endl;
    ostr << "</style>" << endl;
    ostr << "</head>" << endl;
    ostr << "<body style='background: #111111; color: #ececec;' onload=\"javascript:WebSocketCreate()\">" << endl;
    ostr << "<h4><div onclick='javascritpt:clickScroll()' id='logname' class='logtitle'></div></h4>" << endl;
    ostr << "<div id='logs' class='logs'></div>" << endl;
    ostr << "<p><div id='end' style='display: hidden;'>&nbsp;</div></p>" << endl;
    ostr << "</body>" << endl;
}
// -----------------------------------------------------------------------------
#endif
// -----------------------------------------------------------------------------
