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
#include <sstream>
#include <iomanip>
#include <unistd.h>
#include <fstream>

#include "unisetstd.h"
#include <Poco/Net/NetException.h>
#include <Poco/Net/WebSocket.h>
#include <Poco/StreamCopier.h>
#include <Poco/DeflatingStream.h>
#include <Poco/File.h>
#include <Poco/Path.h>
#include "ujson.h"
#include "LogDB.h"
#include "Configuration.h"
#include "Exceptions.h"
#include "LogDBSugar.h"
// --------------------------------------------------------------------------
#ifndef UNISET_DATADIR
#define UNISET_DATADIR "/usr/share/uniset2/"
#endif
// --------------------------------------------------------------------------
using namespace uniset;
using namespace std;
// --------------------------------------------------------------------------
LogDB::LogDB( const string& name, int argc, const char* const* argv, const string& prefix ):
    myname(name)
{
    dblog = make_shared<DebugStream>();

    std::string specconfig;

    int i = uniset::findArgParam("--" + prefix + "single-confile", argc, argv);

    bool singleMode = (i != -1);

    if( singleMode )
        specconfig = uniset::getArgParam("--" + prefix + "single-confile", argc, argv, "");

    std::shared_ptr<UniXML> xml;

    if( specconfig.empty() )
    {
        cout << myname << "(init): init from uniset configure..." << endl;
        uniset_init(argc, argv, "configure.xml");
        auto conf = uniset_conf();

        xml = conf->getConfXML();
    }
    else
    {
        cout << myname << "(init): init from single-confile " << specconfig << endl;
        xml = make_shared<UniXML>();

        try
        {
            xml->open(specconfig);
        }
        catch( std::exception& ex )
        {
            throw ex;
        }
    }

    xmlNode* cnode = xml->findNode(xml->getFirstNode(), "LogDB", name);

    if( !cnode )
    {
        ostringstream err;
        err << name << "(init): Not found confnode <LogDB name='" << name << "'...>";
        dbcrit << err.str() << endl;
        throw uniset::SystemError(err.str());
    }

    UniXML::iterator it(cnode);

    if( specconfig.empty() )
        uniset_conf()->initLogStream(dblog, prefix + "log" );
    else
    {
        // инициализируем сами, т.к. conf нету..
        const std::string loglevels = uniset::getArg2Param("--" + prefix + "log-add-levels", argc, argv, it.getProp("log"), "");

        if( !loglevels.empty() )
            dblog->level(Debug::value(loglevels));
    }

    qbufSize = uniset::getArgPInt("--" + prefix + "db-buffer-size", argc, argv, it.getProp("dbBufferSize"), qbufSize);
    maxdbRecords = uniset::getArgPInt("--" + prefix + "db-max-records", argc, argv, it.getProp("dbMaxRecords"), qbufSize);

    string tformat = uniset::getArg2Param("--" + prefix + "db-timestamp-format", argc, argv, it.getProp("dbTimestampFormat"), "localtime");

    if( tformat == "localtime" || tformat == "utc" )
        tmsFormat = tformat;

    double checkConnection_sec = atof( uniset::getArg2Param("--" + prefix + "ls-check-connection-sec", argc, argv, it.getProp("lsCheckConnectionSec"), "5").c_str());

    int bufSize = uniset::getArgPInt("--" + prefix + "ls-read-buffer-size", argc, argv, it.getProp("lsReadBufferSize"), 10001);

    std::string s_overflow = uniset::getArg2Param("--" + prefix + "db-overflow-factor", argc, argv, it.getProp("dbOverflowFactor"), "1.3");
    float ovf = atof(s_overflow.c_str());

    numOverflow = lroundf( (float)maxdbRecords * ovf );

    if( numOverflow == 0 )
        numOverflow = maxdbRecords;

    dbinfo << myname << "(init): maxdbRecords=" << maxdbRecords << " numOverflow=" << numOverflow << endl;

    flushBufferTimer.set<LogDB, &LogDB::onCheckBuffer>(this);
    wsactivate.set<LogDB, &LogDB::onActivate>(this);
    sigTERM.set<LogDB, &LogDB::onTerminate>(this);
    sigQUIT.set<LogDB, &LogDB::onTerminate>(this);
    sigINT.set<LogDB, &LogDB::onTerminate>(this);

    bool dbDisabled = ( uniset::findArgParam("--" + prefix + "db-disable", argc, argv) != -1 );

    if( dbDisabled )
        dbinfo << myname << "(init): save to database DISABLED.." << endl;

    UniXML::iterator sit(cnode);

    if( !sit.goChildren() )
    {
        ostringstream err;
        err << name << "(init): Not found confnode list of log servers for <LogDB name='" << name << "'...>";
        dbcrit << err.str() << endl;
        throw uniset::SystemError(err.str());
    }

    for( ; sit.getCurrent(); sit++ )
    {
        auto l = make_shared<Log>();

        l->name = sit.getProp("name");
        l->ip = sit.getProp("ip");
        l->port = sit.getIntProp("port");
        l->cmd = sit.getProp("cmd");
        l->description = sit.getProp("description");

        l->setReadBufSize(bufSize);

        l->setCheckConnectionTime(checkConnection_sec);

        if( l->name.empty()  )
        {
            ostringstream err;
            err << name << "(init): Unknown name for logserver.";
            dbcrit << err.str() << endl;
            throw uniset::SystemError(err.str());
        }

        if( l->ip.empty()  )
        {
            ostringstream err;
            err << name << "(init): Unknown 'ip' for '" << l->name << "'..";
            dbcrit << err.str() << endl;
            throw uniset::SystemError(err.str());
        }

        if( l->port == 0 )
        {
            ostringstream err;
            err << name << "(init): Unknown 'port' for '" << l->name << "'..";
            dbcrit << err.str() << endl;
            throw uniset::SystemError(err.str());
        }

        //      l->tcp = make_shared<UTCPStream>();
        l->dblog = dblog;

        if( !dbDisabled )
            l->signal_on_read().connect(sigc::mem_fun(this, &LogDB::addLog));

        auto lfile = sit.getProp("logfile");

        if( !lfile.empty() )
        {
            l->logfile = make_shared<DebugStream>();
            l->logfile->logFile(lfile, false);
            l->logfile->level(Debug::ANY);
            l->logfile->showDateTime(false);
            l->logfile->showLogType(false);
            l->logfile->disableOnScreen();
            l->signal_on_read().connect(sigc::mem_fun(this, &LogDB::log2File));
        }

        //      l->set(loop);

        logservers.push_back(l);
    }

    if( logservers.empty() )
    {
        ostringstream err;
        err << name << "(init): empty list of log servers for <LogDB name='" << name << "'...>";
        dbcrit << err.str() << endl;
        throw uniset::SystemError(err.str());
    }


    if( !dbDisabled )
    {
        dbfile = uniset::getArgParam("--" + prefix + "dbfile", argc, argv, it.getProp("dbfile"));

        if( dbfile.empty() )
        {
            ostringstream err;
            err << name << "(init): dbfile (sqlite) not defined. Use: <LogDB name='" << name << "' dbfile='..' ...>";
            dbcrit << err.str() << endl;
            throw uniset::SystemError(err.str());
        }

        db = unisetstd::make_unique<SQLiteInterface>();

        if( !db->connect(dbfile, false, SQLITE_OPEN_FULLMUTEX) )
        {
            ostringstream err;
            err << myname
                << "(init): DB connection error: "
                << db->error();
            dbcrit << err.str() << endl;
            throw uniset::SystemError(err.str());
        }
    }

#ifndef DISABLE_REST_API
    wsHeartbeatTime_sec = (float)uniset::getArgPInt("--" + prefix + "ws-heartbeat-time", argc, argv, it.getProp("wsPingTime"), wsHeartbeatTime_sec) / 1000.0;
    wsSendTime_sec = (float)uniset::getArgPInt("--" + prefix + "ws-send-time", argc, argv, it.getProp("wsSendTime"), wsSendTime_sec) / 1000.0;
    wsMaxSend = uniset::getArgPInt("--" + prefix + "ws-max-send", argc, argv, it.getProp("wsMaxSend"), wsMaxSend);

    httpHost = uniset::getArgParam("--" + prefix + "httpserver-host", argc, argv, "localhost");
    httpPort = uniset::getArgInt("--" + prefix + "httpserver-port", argc, argv, "8080");
    httpCORS_allow = uniset::getArgParam("--" + prefix + "httpserver-cors-allow", argc, argv, httpCORS_allow);
    httpReplyAddr = uniset::getArgParam("--" + prefix + "httpserver-reply-addr", argc, argv, "");
    auto httpDefaultCharset =  uniset::getArgParam("--" + prefix + "httpserver-charset", argc, argv, "UTF-8");
    httpHtmlContentType = "text/html; charset=" + httpDefaultCharset;
    httpJsonContentType = "text/json; charset=" + httpDefaultCharset;

    httpEnabledLogControl = (uniset::findArgParam("--" + prefix + "httpserver-logcontrol-enable", argc, argv) != -1 );

    if( httpEnabledLogControl )
        dblog1 << myname << "(init): enabled log control via http api" << endl;

    if( !dbDisabled )
    {
        httpEnabledDownload = (uniset::findArgParam("--" + prefix + "httpserver-download-enable", argc, argv) != -1);

        if( httpEnabledDownload )
            dbinfo << myname << "(init): enabled download API.." << endl;
    }

    dblog1 << myname << "(init): http server parameters " << httpHost << ":" << httpPort << endl;
    Poco::Net::SocketAddress sa(httpHost, httpPort);

    maxwsocks = uniset::getArgPInt("--" + prefix + "ws-max", argc, argv, it.getProp("wsMax"), maxwsocks);

    // загружаем html-файл
    auto htmlfile = uniset::getArg2Param("--" + prefix + "ws-html-template", argc, argv, it.getProp("wsHtmlTemplate"), "logdb-websocket.html");

    if( htmlfile.empty() )
    {
        ostringstream err;
        err << myname
            << "(init): websocket html template file not defined...";

        dbcrit << err.str() << endl;
        throw uniset::SystemError(err.str());
    }

    if( htmlfile[0] != '.' && htmlfile[0] != '/' )
    {
        std::vector<std::string> searchPath = {"./"};

        if( !singleMode )
            searchPath.push_back(uniset_conf()->getDataDir());

        searchPath.push_back(UNISET_DATADIR);

        for( const auto& p : searchPath )
        {
            if( file_exist( p + htmlfile) )
            {
                htmlfile = p + htmlfile;
                break;
            }
        }
    }

    dbinfo << "(init): websocket html template " << htmlfile << endl;

    if( !file_exist(htmlfile) )
    {
        ostringstream err;
        err << myname
            << "(init): Not found " << htmlfile;

        dbcrit << err.str() << endl;
        throw uniset::SystemError(err.str());
    }

    wsPageTemplate = htmlfile;

    try
    {
        Poco::Net::HTTPServerParams* httpParams = new Poco::Net::HTTPServerParams;

        int maxQ = uniset::getArgPInt("--" + prefix + "httpserver-max-queued", argc, argv, it.getProp("httpMaxQueued"), 100);
        int maxT = uniset::getArgPInt("--" + prefix + "httpserver-max-threads", argc, argv, it.getProp("httpMaxThreads"), 3);

        httpParams->setMaxQueued(maxQ);
        httpParams->setMaxThreads(maxT);
        httpserv = std::make_shared<Poco::Net::HTTPServer>(new LogDBRequestHandlerFactory(this), Poco::Net::ServerSocket(sa), httpParams );
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
LogDB::~LogDB()
{
    if( evIsActive() )
        evstop();

#ifndef DISABLE_REST_API

    if( httpserv )
        httpserv->stop();

#endif

    if( db )
        db->close();
}
//--------------------------------------------------------------------------------------------
void LogDB::onTerminate( ev::sig& evsig, int revents )
{
    if( EV_ERROR & revents )
    {
        dbcrit << myname << "(onTerminate): invalid event" << endl;
        return;
    }

    dbinfo << myname << "(onTerminate): terminate..." << endl;

    try
    {
        flushBuffer();
    }
    catch( std::exception& ex )
    {
        dbinfo << myname << "(onTerminate): "  << ex.what() << endl;

    }

    evsig.stop();

    //evsig.loop.break_loop();
#ifndef DISABLE_REST_API

    try
    {
        httpserv->stop();
    }
    catch( std::exception& ex )
    {
        dbinfo << myname << "(onTerminate): "  << ex.what() << endl;
    }

#endif

    try
    {
        evstop();
    }
    catch( std::exception& ex )
    {
        dbinfo << myname << "(onTerminate): "  << ex.what() << endl;

    }
}
//--------------------------------------------------------------------------------------------
void LogDB::flushBuffer()
{
    if( !db || qbuf.empty() || !db->isConnection() )
        return;

    // без BEGIN и COMMIT вставка большого количества данных будет тормозить!
    db->query("BEGIN;");

    while( !qbuf.empty() )
    {
        if( !db->insert(qbuf.front()) )
        {
            dbcrit << myname << "(flushBuffer): error: " << db->error()
                   << " lost query: " << qbuf.front() << endl;
        }

        qbuf.pop();
    }

    db->query("COMMIT;");

    if( !db->lastQueryOK() )
    {
        dbcrit << myname << "(flushBuffer): error: " << db->error() << endl;
    }

    // вызываем каждый раз, для отслеживания переполнения..
    rotateDB();
}
//--------------------------------------------------------------------------------------------
void LogDB::rotateDB()
{
    if( !db )
        return;

    // ротация отключена
    if( maxdbRecords == 0 )
        return;

    size_t num = getCountOfRecords();

    if( num <= numOverflow )
        return;

    dblog2 << myname << "(rotateDB): num=" << num << " > " << numOverflow << endl;

    size_t firstOldID = getFirstOfOldRecord(numOverflow);

    DBResult ret = db->query("DELETE FROM logs WHERE id <= " + std::to_string(firstOldID) + ";");

    if( !db->lastQueryOK() )
    {
        dbwarn << myname << "(rotateDB): delete error: " << db->error() << endl;
    }

    ret = db->query("VACUUM;");

    if( !db->lastQueryOK() )
    {
        dbwarn << myname << "(rotateDB): vacuum error: " << db->error() << endl;
    }

    //  dblog3 <<  myname << "(rotateDB): after rotate: " << getCountOfRecords() << " records" << endl;
}
//--------------------------------------------------------------------------------------------
void LogDB::addLog( LogDB::Log* log, const string& txt )
{
    auto tm = uniset::now_to_timespec();

    ostringstream q;

    q << "INSERT INTO logs(tms,usec,name,text) VALUES("
      << "datetime(" << tm.tv_sec << ",'unixepoch'),'"   //  timestamp
      << tm.tv_nsec << "','"  //  usec
      << log->name << "','"
      << qEscapeString(txt) << "');";

    qbuf.emplace(q.str());
}
//--------------------------------------------------------------------------------------------
void LogDB::log2File( LogDB::Log* log, const string& txt )
{
    if( !log->logfile || !log->logfile->isOnLogFile() )
        return;

    log->logfile->any() << txt << endl;
}
//--------------------------------------------------------------------------------------------
size_t LogDB::getCountOfRecords( const std::string& logname )
{
    ostringstream q;

    q << "SELECT count(*) FROM logs";

    if( !logname.empty() )
        q << " WHERE name='" << logname << "'";

    DBResult ret = db->query(q.str());

    if( !ret )
        return 0;

    return (size_t) ret.begin().as_int(0);
}
//--------------------------------------------------------------------------------------------
size_t LogDB::getFirstOfOldRecord( size_t maxnum )
{
    ostringstream q;

    q << "SELECT id FROM logs order by id DESC limit " << maxnum << ",1";
    DBResult ret = db->query(q.str());

    if( !ret )
        return 0;

    return (size_t) ret.begin().as_int(0);
}
//--------------------------------------------------------------------------------------------
std::shared_ptr<LogDB> LogDB::init_logdb( int argc, const char* const* argv, const std::string& prefix )
{
    string name = uniset::getArgParam("--" + prefix + "name", argc, argv, "LogDB");

    if( name.empty() )
    {
        cerr << "(LogDB): Unknown name. Use --" << prefix << "name" << endl;
        return nullptr;
    }

    return make_shared<LogDB>(name, argc, argv, prefix);
}
// -----------------------------------------------------------------------------
void LogDB::help_print()
{
    cout << "Default: prefix='logdb'" << endl;
    cout << "--prefix-single-confile conf.xml     - Отдельный конфигурационный файл (не требующий структуры uniset)" << endl;
    cout << "--prefix-name name                   - Имя. Для поиска настроечной секции в configure.xml" << endl;
    cout << "database: " << endl;
    cout << "--prefix-db-buffer-size sz                  - Размер буфера (до скидывания в БД)." << endl;
    cout << "--prefix-db-max-records sz                  - Максимальное количество записей в БД. При превышении, старые удаляются. 0 - не удалять" << endl;
    cout << "--prefix-db-overflow-factor float           - Коэффициент переполнения, после которого запускается удаление старых записей. По умолчанию: 1.3" << endl;
    cout << "--prefix-db-disable                         - Отключить запись в БД" << endl;
    cout << "--prefix-db-timestamp-format localtime|utc  - Формат времени в ответе на запросы. По умолчанию: localtime" << endl;

    cout << "websockets: " << endl;
    cout << "--prefix-ws-max num                  - Максимальное количество websocket-ов" << endl;
    cout << "--prefix-ws-heartbeat-time msec      - Период сердцебиения в соединении. По умолчанию: 3000 мсек" << endl;
    cout << "--prefix-ws-send-time msec           - Период посылки сообщений. По умолчанию: 500 мсек" << endl;
    cout << "--prefix-ws-max num                  - Максимальное число сообщений посылаемых за один раз. По умолчанию: 200" << endl;
    cout << "--prefix-ws-html-template file       - Шаблон websocket страницы. По умолчанию: " << std::string(UNISET_DATADIR) << "logdb-websocket.html" << endl;

    cout << "logservers: " << endl;
    cout << "--prefix-ls-check-connection-sec sec    - Период проверки соединения с логсервером" << endl;
    cout << "--prefix-ls-read-buffer-size num        - Размер буфера для чтения сообщений от логсервера. По умолчанию: 10001" << endl;

    cout << "http: " << endl;
    cout << "--prefix-httpserver-host ip                 - IP на котором слушает http сервер. По умолчанию: localhost" << endl;
    cout << "--prefix-httpserver-port num                - Порт на котором принимать запросы. По умолчанию: 8080" << endl;
    cout << "--prefix-httpserver-max-queued num          - Размер очереди запросов к http серверу. По умолчанию: 100" << endl;
    cout << "--prefix-httpserver-max-threads num         - Разрешённое количество потоков для http-сервера. По умолчанию: 3" << endl;
    cout << "--prefix-httpserver-cors-allow addr         - (CORS): Access-Control-Allow-Origin. Default: *" << endl;
    cout << "--prefix-httpserver-charset charset         - ContentType charset. Default: 'UTF-8'" << endl;
    cout << "--prefix-httpserver-reply-addr host[:port]  - Адрес отдаваемый клиенту для подключения. По умолчанию адрес узла где запущен logdb" << endl;
    cout << "--prefix-httpserver-logcontrol-enable       - Включить API для управления логами через HTTP" << endl;
    cout << "--prefix-httpserver-download-enable         - Включить возможность скачать файл БД через HTTP" << endl;
}
// -----------------------------------------------------------------------------
void LogDB::run( bool async )
{
#ifndef DISABLE_REST_API

    if( httpserv )
        httpserv->start();

#endif

    if( async )
        async_evrun();
    else
        evrun();
}
// -----------------------------------------------------------------------------
void LogDB::evfinish()
{
    flushBufferTimer.stop();
    wsactivate.stop();
}
// -----------------------------------------------------------------------------
void LogDB::evprepare()
{
    if( db )
    {
        flushBufferTimer.set(loop);
        flushBufferTimer.start(0, tmFlushBuffer_sec);
    }

    wsactivate.set(loop);
    wsactivate.start();

    for( const auto& s : logservers )
        s->set(loop);

    sigTERM.set(loop);
    sigTERM.start(SIGTERM);
    sigQUIT.set(loop);
    sigQUIT.start(SIGQUIT);
    sigINT.set(loop);
    sigINT.start(SIGINT);
}
// -----------------------------------------------------------------------------
void LogDB::onCheckBuffer(ev::timer& t, int revents)
{
    if( EV_ERROR & revents )
    {
        dbcrit << myname << "(LogDB::onCheckBuffer): invalid event" << endl;
        return;
    }

    if( qbuf.size() >= qbufSize )
        flushBuffer();
}
// -----------------------------------------------------------------------------
void LogDB::onActivate( ev::async& watcher, int revents )
{
    if (EV_ERROR & revents)
    {
        dbcrit << myname << "(LogDB::onActivate): invalid event" << endl;
        return;
    }

#ifndef DISABLE_REST_API
    uniset_rwmutex_rlock lk(wsocksMutex);

    for( const auto& s : wsocks )
    {
        if( !s->isActive() )
            s->set(loop);
    }

#endif
}
// -----------------------------------------------------------------------------
bool LogDB::Log::isConnected() const
{
    return tcp && tcp->isConnected();
}
// -----------------------------------------------------------------------------
void LogDB::Log::set( ev::dynamic_loop& loop )
{
    io.set(loop);
    iocheck.set(loop);
    iocheck.set<LogDB::Log, &LogDB::Log::check>(this);
    iocheck.start(0, checkConnection_sec);
    iocmd.set(loop);
    iocmd.start();
}
// -----------------------------------------------------------------------------
void LogDB::Log::check( ev::timer& t, int revents )
{
    if( EV_ERROR & revents)
        return;

    if( isConnected() )
        return;

    if( connect() )
        ioprepare();
}
// -----------------------------------------------------------------------------
bool LogDB::Log::connect() noexcept
{
    if( tcp && tcp->isConnected() )
        return true;

    //  dbinfo << name << "(connect): connect " << ip << ":" << port << "..." << endl;

    if( peername.empty() )
        peername = ip + ":" + std::to_string(port);

    try
    {
        tcp = make_shared<UTCPStream>();
        tcp->create(ip, port);
        //      tcp->setReceiveTimeout( UniSetTimer::millisecToPoco(inTimeout) );
        //      tcp->setSendTimeout( UniSetTimer::millisecToPoco(outTimeout) );
        tcp->setKeepAlive(true);
        tcp->setBlocking(false);
        dbinfo << name << "(connect): connect OK to " << ip << ":" << port << endl;
        return true;
    }
    catch( const Poco::TimeoutException& e )
    {
        dbwarn << name << "(connect): connection " << peername << " timeout.." << endl;
    }
    catch( const Poco::Net::NetException& e )
    {
        dbwarn << name << "(connect): connection " << peername << " error: " << e.what() << endl;
    }
    catch( const std::exception& e )
    {
        dbwarn << name << "(connect): connection " << peername << " error: " << e.what() << endl;
    }
    catch( ... )
    {
        std::exception_ptr p = std::current_exception();
        dbwarn << name << "(connect): connection " << peername << " error: "
               << (p ? p.__cxa_exception_type()->name() : "null") << endl;
    }

    tcp->disconnect();
    tcp = nullptr;

    return false;
}
// -----------------------------------------------------------------------------
void LogDB::Log::ioprepare()
{
    if( !tcp || !tcp->isConnected() )
        return;

    io.set<LogDB::Log, &LogDB::Log::event>(this);
    io.start(tcp->getSocket(), ev::READ);
    text.reserve(reservsize);
    iocmd.set<LogDB::Log, &LogDB::Log::oncommand>(this);

    // первый раз при подключении надо послать команды
    if( !usercmd.empty() )
        setCommand(usercmd);
    else
        setCommand(cmd);
}
// -----------------------------------------------------------------------------
void LogDB::Log::event( ev::io& watcher, int revents )
{
    if( EV_ERROR & revents )
    {
        dbcrit << name << "(event): invalid event" << endl;
        return;
    }

    if( revents & EV_READ )
        read(watcher);

    if( revents & EV_WRITE )
        write(watcher);
}
// -----------------------------------------------------------------------------
LogDB::Log::ReadSignal LogDB::Log::signal_on_read()
{
    return sigRead;
}
// -----------------------------------------------------------------------------
void LogDB::Log::setCheckConnectionTime( double sec )
{
    if( sec > 0 )
        checkConnection_sec = sec;
}
// -----------------------------------------------------------------------------
void LogDB::Log::setReadBufSize( size_t sz )
{
    buf.resize(sz);
}
// -----------------------------------------------------------------------------
void LogDB::Log::setCommand( const std::string& c )
{
    //! \todo Пока закрываем глаза на не оптимальность того, что парсим строку каждый раз
    auto cmdlist = LogServerTypes::getCommands(c);

    if( cmdlist.empty() )
        return;

    lastcmd = c;

    uniset::uniset_rwmutex_wrlock lock(cmdmut);

    for( const auto& msg : cmdlist )
    {
        dblog3 << name << ": command: " << msg << endl;
        cmdbuf.push_back(new UTCPCore::Buffer((unsigned char*) &msg, sizeof(msg)));
    }

    iocmd.send();
}
// -----------------------------------------------------------------------------
void LogDB::Log::oncommand( ev::async& watcher, int revents )
{
    if( EV_ERROR & revents )
    {
        dbcrit << name << "(oncommand): invalid event" << endl;
        return;
    }

    uniset::uniset_rwmutex_wrlock lock(cmdmut);

    for( const auto& msg : cmdbuf )
    {
        wbuf.push(msg);
    }

    cmdbuf.clear();
    io.set(EV_WRITE);
}
// -----------------------------------------------------------------------------
void LogDB::Log::read( ev::io& watcher )
{
    if( !tcp )
        return;

    int n = tcp->available();

    n = std::min(n, (int)buf.size());

    if( n > 0 )
    {
        tcp->receiveBytes(buf.data(), n);

        // нарезаем на строки
        for( int i = 0; i < n; i++ )
        {
            if( buf[i] != '\n' )
                text += buf[i];
            else
            {
                sigRead.emit(this, text);
                text = "";

                if( text.capacity() < reservsize )
                    text.reserve(reservsize);
            }
        }
    }
    else if( n == 0 )
    {
        dbinfo << name << ": " << ip << ":" << port << " connection is closed.." << endl;

        if( !text.empty() )
        {
            sigRead.emit(this, text);
            text = "";

            if( text.capacity() < reservsize )
                text.reserve(reservsize);
        }

        close();
    }
}
// -----------------------------------------------------------------------------
void LogDB::Log::write( ev::io& io )
{
    UTCPCore::Buffer* buffer = 0;

    if( wbuf.empty() )
    {
        io.set(EV_READ);
        return;
    }

    buffer = wbuf.front();

    if( !buffer )
    {
        io.set(EV_READ);
        return;
    }

    ssize_t ret = ::write(io.fd, buffer->dpos(), buffer->nbytes());

    if( ret < 0 )
    {
        int errnum = errno;
        dbwarn << peername << "(write): write to socket error(" << errnum << "): " << strerror(errnum) << endl;

        if( errnum == EPIPE || errnum == EBADF )
        {
            dbwarn << peername << "(write): write error.. terminate session.." << endl;
            io.set(EV_NONE);
            close();
        }

        return;
    }

    buffer->pos += ret;

    if( buffer->nbytes() == 0 )
    {
        wbuf.pop();
        delete buffer;
    }

    if( wbuf.empty() )
        io.set(EV_READ);
    else
        io.set(EV_WRITE);
}
// -----------------------------------------------------------------------------
void LogDB::Log::close()
{
    tcp->disconnect();
    //tcp = nullptr;
    io.stop();
    iocheck.stop();
    iocmd.stop();
}
// -----------------------------------------------------------------------------
std::string LogDB::qEscapeString( const string& txt )
{
    ostringstream ret;

    for( const auto& c : txt )
    {
        ret << c;

        if( c == '\'' || c == '"' )
            ret << c;
    }

    return ret.str();
}
// -----------------------------------------------------------------------------
#ifndef DISABLE_REST_API
// -----------------------------------------------------------------------------
class LogDBRequestHandler:
    public Poco::Net::HTTPRequestHandler
{
    public:

        LogDBRequestHandler( LogDB* l ): logdb(l) {}

        virtual void handleRequest( Poco::Net::HTTPServerRequest& request,
                                    Poco::Net::HTTPServerResponse& response ) override
        {
            logdb->handleRequest(request, response);
        }

    private:
        LogDB* logdb;
};
// -----------------------------------------------------------------------------
class LogDBWebSocketRequestHandler:
    public Poco::Net::HTTPRequestHandler
{
    public:

        LogDBWebSocketRequestHandler( LogDB* l ): logdb(l) {}

        virtual void handleRequest( Poco::Net::HTTPServerRequest& request,
                                    Poco::Net::HTTPServerResponse& response ) override
        {
            logdb->onWebSocketSession(request, response);
        }

    private:
        LogDB* logdb;
};
// -----------------------------------------------------------------------------
Poco::Net::HTTPRequestHandler* LogDB::LogDBRequestHandlerFactory::createRequestHandler( const Poco::Net::HTTPServerRequest& req )
{
    if( req.find("Upgrade") != req.end() && Poco::icompare(req["Upgrade"], "websocket") == 0 )
        return new LogDBWebSocketRequestHandler(logdb);

    return new LogDBRequestHandler(logdb);
}
// -----------------------------------------------------------------------------
void LogDB::handleRequest( Poco::Net::HTTPServerRequest& req, Poco::Net::HTTPServerResponse& resp )
{
    using Poco::Net::HTTPResponse;

    resp.set("Access-Control-Allow-Methods", "GET");
    resp.set("Access-Control-Allow-Request-Method", "*");
    resp.set("Access-Control-Allow-Origin", httpCORS_allow /* req.get("Origin") */);
    resp.setContentType(httpJsonContentType);

    try
    {
        // В этой версии API поддерживается только GET
        if( req.getMethod() != "GET" )
        {
            std::ostream& out = resp.send();
            auto jdata = respError(resp, HTTPResponse::HTTP_BAD_REQUEST, "method must be 'GET'");
            jdata->stringify(out);
            out.flush();
            return;
        }

        Poco::URI uri(req.getURI());

        dblog3 << req.getHost() << ": query: " << uri.getQuery() << endl;

        std::vector<std::string> seg;
        uri.getPathSegments(seg);
        auto qp = uri.getQueryParameters();

        // проверка подключения к страничке со списком websocket-ов
        // http://[xxxx:port]/ws/
        if( seg.size() > 0 && seg[0] == "ws" )
        {
            if( seg.size() > 2 )
            {
                if( seg[1] == "connect" )
                {
                    resp.setContentType(httpHtmlContentType);
                    httpWebSocketConnectPage(req, resp, seg[2], qp);
                    return;
                }

                std::ostream& out = resp.send();
                auto jdata = respError(resp, HTTPResponse::HTTP_BAD_REQUEST, "Unknown path '" + seg[1]  + "'");
                jdata->stringify(out);
                out.flush();
                return;
            }

            // default page
            resp.setContentType(httpHtmlContentType);
            std::ostream& out = resp.send();
            httpWebSocketPage(out, req, resp, qp);
            out.flush();
            return;
        }

        // example: http://host:port/api/version/logcontrol/..
        if( seg.size() >= 4
                && seg[0] == "api"
                && seg[1] == uniset::UHttp::UHTTP_API_VERSION
                && seg[2] == "logcontrol" )
        {
            resp.setStatus(HTTPResponse::HTTP_OK);
            std::ostream& out = resp.send();

            auto jdata = httpLogControl(out, req, resp, seg[3], qp);
            jdata->stringify(out);
            out.flush();
            return;
        }

        // example: http://host:port/api/version/logdb/..
        if( seg.size() < 4
                || seg[0] != "api"
                || seg[1] != uniset::UHttp::UHTTP_API_VERSION
                || seg[2].empty()
                || seg[2] != "logdb")
        {
            std::ostream& out = resp.send();
            ostringstream err;
            err << "Bad request structure. Must be /api/" << uniset::UHttp::UHTTP_API_VERSION << "/logdb/xxx";
            auto jdata = respError(resp, HTTPResponse::HTTP_BAD_REQUEST, err.str());
            jdata->stringify(out);
            out.flush();
            return;
        }

        if( !db )
        {
            std::ostream& out = resp.send();
            ostringstream err;
            err << "Working with the database is disabled";
            auto jdata = respError(resp, HTTPResponse::HTTP_SERVICE_UNAVAILABLE, err.str());
            jdata->stringify(out);
            out.flush();
            return;
        }

        resp.setStatus(HTTPResponse::HTTP_OK);
        string cmd = seg[3];

        if( cmd == "download" )
        {
            httpDownload(req, resp, qp);
            return;
        }

        std::ostream& out = resp.send();

        if( cmd == "help" )
        {
            using uniset::json::help::item;
            uniset::json::help::object myhelp("help");
            myhelp.emplace(item("help", "this help"));
            myhelp.emplace(item("list", "list of logs"));
            myhelp.emplace(item("count?logname", "count of logs for logname"));
            myhelp.emplace(item("set?logname=level1,level2&logname2=level3,warn", "setup logs for lognames"));

            item l("logs", "read logs");
            l.param("from='YYYY-MM-DD'", "From date");
            l.param("to='YYYY-MM-DD'", "To date");
            l.param("last=XX[m|h|d|M]", "Last records (m - minute, h - hour, d - day, M - month)");
            l.param("offset=N", "offset");
            l.param("limit=M", "limit records for response");
            myhelp.add(l);

            myhelp.emplace(item("apidocs", "https://github.com/Etersoft/uniset2"));
            myhelp.get()->stringify(out);
        }
        else
        {
            auto json = httpGetRequest(resp, cmd, qp);

            if( json )
                json->stringify(out);
        }

        out.flush();
    }
    catch( std::exception& ex )
    {
        std::ostream& out = resp.send();
        auto jdata = respError(resp, HTTPResponse::HTTP_INTERNAL_SERVER_ERROR, ex.what());
        jdata->stringify(out);
        out.flush();
    }
}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr LogDB::respError( Poco::Net::HTTPServerResponse& resp,
        Poco::Net::HTTPResponse::HTTPStatus estatus,
        const string& message )
{
    resp.setStatus(estatus);
    resp.setContentType(httpJsonContentType);
    Poco::JSON::Object::Ptr jdata = new Poco::JSON::Object();
    jdata->set("error", resp.getReasonForStatus(resp.getStatus()));
    jdata->set("ecode", (int)resp.getStatus());
    jdata->set("message", message);
    return jdata;
}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr LogDB::httpGetRequest( Poco::Net::HTTPServerResponse& resp,
        const string& cmd,
        const Poco::URI::QueryParameters& p )
{
    if( cmd == "list" )
        return httpGetList(resp, p);

    if( cmd == "logs" )
        return httpGetLogs(resp, p);

    if( cmd == "count" )
        return httpGetCount(resp, p);

    return respError(resp, Poco::Net::HTTPResponse::HTTP_BAD_REQUEST, "Unknown command '"  + cmd + "'");
}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr LogDB::httpGetList( Poco::Net::HTTPServerResponse& resp, const Poco::URI::QueryParameters& p )
{
    if( !db )
    {
        ostringstream err;
        err << "DB unavailable..";
        throw uniset::SystemError(err.str());

    }

    Poco::JSON::Object::Ptr jdata = new Poco::JSON::Object();

    Poco::JSON::Array::Ptr jlist = uniset::json::make_child_array(jdata, "logs");

#if 0
    // Получение из БД
    // хорошо тем, что возвращаем список реально доступных логов (т.е. тех что есть в БД)
    // плохо тем, что если в конфигурации добавили какие-то логи, но в БД
    // ещё ничего не попало, мы их не увидим

    ostringstream q;

    q << "SELECT COUNT(*), name FROM logs GROUP BY name";
    DBResult ret = db->query(q.str());

    if( !ret )
        return jdata;

    for( auto it = ret.begin(); it != ret.end(); ++it )
    {
        Poco::JSON::Object::Ptr j = new Poco::JSON::Object();
        j->set("name", it.as_string("name"));
        jlist->add(j);
    }

#else
    // Получение из конфигурации
    // хорошо тем, что если логов ещё не было
    // то всё-равно видно, какие доступны потенциально
    // плохо тем, что если конфигурацию поменяли (убрали какой-то лог)
    // а в БД записи по нему остались, то мы не получим к ним доступ

    /*! \todo пока список logservers формируется только в начале (в конструкторе)
     * можно не защищаться mutex-ом, т.к. мы его не меняем
     * если вдруг в REST API будет возможность добавлять логи.. нужно защищаться
     * либо переделывать обработку
     */
    for( const auto& s : logservers )
    {
        Poco::JSON::Object::Ptr j = new Poco::JSON::Object();
        j->set("name", s->name);
        j->set("description", s->description);
        jlist->add(j);
    }

#endif

    return jdata;
}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr LogDB::httpGetLogs( Poco::Net::HTTPServerResponse& resp, const Poco::URI::QueryParameters& params )
{
    Poco::JSON::Object::Ptr jdata = new Poco::JSON::Object();

    if( params.empty() || params[0].first.empty() )
    {
        dblog3 << "(httpGetLogs): empty logname" << endl;
        return respError(resp, Poco::Net::HTTPResponse::HTTP_BAD_REQUEST, "Unknown logname");
    }

    const auto& logname = params[0].first;

    size_t offset = 0;
    size_t limit = 0;

    vector<std::string> q_where;

    for( const auto& p : params )
    {
        if( p.first == "offset" )
            offset = uni_atoi(p.second);
        else if( p.first == "limit" )
            limit = uni_atoi(p.second);
        else if( p.first == "from" )
            q_where.push_back("tms>='" + qDate(p.second) + " 00:00:00'");
        else if( p.first == "to" )
            q_where.push_back("tms<='" + qDate(p.second) + " 23:59:59'");
        else if( p.first == "last" )
            q_where.push_back(qLast(p.second));
    }

    Poco::JSON::Array::Ptr jlist = uniset::json::make_child_array(jdata, "logs");

    ostringstream q;

    q << "SELECT tms,"
      << " strftime('%d-%m-%Y',datetime(tms,'" << tmsFormat << "')) as date,"
      << " strftime('%H:%M:%S',datetime(tms,'" << tmsFormat << "')) as time,"
      << " usec, text FROM logs WHERE name='" << logname << "'";

    if( !q_where.empty() )
    {
        for( const auto& w : q_where )
            q << " AND " << w;
    }

    if( limit > 0 )
        q << " ORDER BY tms ASC LIMIT " << offset << "," << limit;

    DBResult ret = db->query(q.str());

    if( !ret )
        return jdata;

    for( auto it = ret.begin(); it != ret.end(); ++it )
    {
        Poco::JSON::Object::Ptr j = new Poco::JSON::Object();
        j->set("tms", it.as_string("tms"));
        j->set("date", it.as_string("date"));
        j->set("time", it.as_string("time"));
        j->set("usec", it.as_string("usec"));
        j->set("text", it.as_string("text"));
        jlist->add(j);
    }

    return jdata;
}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr LogDB::httpGetCount( Poco::Net::HTTPServerResponse& resp, const Poco::URI::QueryParameters& params )
{
    if( params.empty() || params[0].first.empty() )
    {
        dblog3 << "(httpGetCount): empty logname" << endl;
        return respError(resp, Poco::Net::HTTPResponse::HTTP_BAD_REQUEST, "Unknown logname");
    }

    Poco::JSON::Object::Ptr jdata = new Poco::JSON::Object();

    size_t count = getCountOfRecords(params[0].first);
    jdata->set("name", params[0].first);
    jdata->set("count", count);
    return jdata;
}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr LogDB::httpDownload( Poco::Net::HTTPServerRequest& req,
        Poco::Net::HTTPServerResponse& resp,
        const Poco::URI::QueryParameters& p )
{
    if( !httpEnabledDownload )
    {
        dbwarn << "(httpDownload): db download disabled" << endl;
        return respError(resp, Poco::Net::HTTPResponse::HTTP_SERVICE_UNAVAILABLE, "API download disabled");
    }

    dblog3 << "(httpDownload): download database '" << dbfile << "'" << endl;

    Poco::File pfile(dbfile);

    if( !pfile.exists() || !pfile.isFile() )
    {
        dbwarn << "(httpDownload): Can't open file '" << dbfile << "'" << endl;
        return respError(resp, Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR, "Can't open dbfile");
    }

    std::ifstream sfile(dbfile, std::ios::binary);

    if( !sfile.is_open() )
    {
        dbwarn << "(httpDownload): Can't open ifstream for file '" << dbfile << "'" << endl;
        return respError(resp, Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR, "Can't open dbfile");
    }

    // Проверяем поддержку gzip клиентом
    bool useGzip = supportsGzip(req);

    auto path = Poco::Path(pfile.path());
    std::string fname = path.getFileName();

    if( useGzip )
        fname += ".gz";

    std::string utf8Name;
    Poco::URI::encode(fname, utf8Name, utf8Code);

    const std::string contentDisposition =
        "attachment; filename=\"" + fname + "\"; " +
        "filename*=UTF-8''" + utf8Name;

    resp.set("Content-Disposition", contentDisposition);

    resp.setContentType("application/octet-stream");

    if( useGzip )
    {
        resp.setContentType("application/gzip");
        resp.set("Content-Encoding", "gzip");
        resp.setChunkedTransferEncoding(true);
    }
    else
        resp.setContentLength(pfile.getSize());

    try
    {
        if( useGzip )
        {
            Poco::DeflatingOutputStream gzipStream(resp.send(), Poco::DeflatingStreamBuf::STREAM_GZIP);
            Poco::StreamCopier::copyStream(sfile, gzipStream);
            gzipStream.close();
        }
        else
        {
            // Отправляем без сжатия
            Poco::StreamCopier::copyStream(sfile, resp.send());
        }

        sfile.close();
    }
    catch( const Poco::Exception& exc )
    {
        sfile.close();
        dbwarn << "Error sending file: " << exc.displayText() << std::endl;
        return respError(resp, Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR, "Can't send dbfile");
    }

    return nullptr;
}
// -----------------------------------------------------------------------------
bool LogDB::supportsGzip( Poco::Net::HTTPServerRequest& request )
{
    if (request.has("Accept-Encoding"))
    {
        std::string acceptEncoding = request.get("Accept-Encoding");
        return acceptEncoding.find("gzip") != std::string::npos;
    }

    return false;
}
// -----------------------------------------------------------------------------
string LogDB::qLast( const string& p )
{
    if( p.empty() )
        return "";

    char unit =  p[p.size() - 1];
    std::string sval = p.substr(0, p.size() - 1);

    if( unit == 'h' || unit == 'H' )
    {
        size_t h = uni_atoi(sval);
        ostringstream q;
        q << "tms >= strftime('%s',datetime('now')) - " << h << "*60*60";
        return q.str();
    }
    else if( unit == 'd' || unit == 'D' )
    {
        size_t d = uni_atoi(sval);
        ostringstream q;
        q << "tms >= strftime('%s',datetime('now')) - " << d << "*24*60*60";
        return q.str();
    }
    else if( unit == 'M' )
    {
        size_t m = uni_atoi(sval);
        ostringstream q;
        q << "tms >= strftime('%s',datetime('now')) - " << m << "*30*24*60*60";
        return q.str();
    }
    else // по умолчанию минут
    {
        size_t m = (unit == 'm') ? uni_atoi(sval) : uni_atoi(p);
        ostringstream q;
        q << "tms >= strftime('%s',datetime('now')) - " << m << "*60";
        return q.str();
    }

    return "";
}
// -----------------------------------------------------------------------------
string LogDB::qDate( const string& p, const char sep )
{
    if( p.size() < 8 || p.size() > 10 )
        return ""; // bad format

    // преобразование в дату 'YYYY-MM-DD' из строки 'YYYYMMDD' или 'YYYY/MM/DD'
    if( p.size() == 10 ) // <-- значит у нас длинная строка
    {
        std::string ret(p);
        // независимо от того, правильная она или нет
        // расставляем разделитель
        ret[4] = sep;
        ret[8] = sep;
        return ret;
    }

    return p.substr(0, 4) + "-" + p.substr(4, 2) + "-" + p.substr(6, 2);
}
// -----------------------------------------------------------------------------
void LogDB::onWebSocketSession(Poco::Net::HTTPServerRequest& req, Poco::Net::HTTPServerResponse& resp)
{
    using Poco::Net::WebSocket;
    using Poco::Net::WebSocketException;
    using Poco::Net::HTTPResponse;
    using Poco::Net::HTTPServerRequest;

    std::vector<std::string> seg;

    Poco::URI uri(req.getURI());

    uri.getPathSegments(seg);
    auto qp = uri.getQueryParameters();

    // example: http://host:port/ws/connect/logname
    if( seg.size() < 3
            || seg[0] != "ws"
            || seg[1] != "connect"
            || seg[2].empty())
    {

        resp.setStatus(HTTPResponse::HTTP_BAD_REQUEST);
        resp.setContentType(httpHtmlContentType);
        resp.setStatusAndReason(HTTPResponse::HTTP_BAD_REQUEST);
        resp.setContentLength(0);
        std::ostream& err = resp.send();
        err << "Bad request. Must be ws://host:port/ws/connect/logname";
        err.flush();
        return;
    }

    {
        uniset_rwmutex_rlock lk(wsocksMutex);

        if( wsocks.size() >= maxwsocks )
        {
            resp.setStatus(HTTPResponse::HTTP_SERVICE_UNAVAILABLE);
            resp.setContentType(httpHtmlContentType);
            resp.setStatusAndReason(HTTPResponse::HTTP_SERVICE_UNAVAILABLE);
            resp.setContentLength(0);
            std::ostream& err = resp.send();
            err << "Error: exceeding the maximum number of open connections (" << maxwsocks << ")";
            err.flush();
            return;
        }
    }

    auto ws = newWebSocket(&req, &resp, seg[2], qp);

    if( !ws )
        return;

    LogWebSocketGuard lk(ws, this);

    dblog3 << myname << "(onWebSocketSession): start session for " << req.clientAddress().toString() << endl;

    // т.к. вся работа происходит в eventloop
    // то здесь просто ждём.
    ws->waitCompletion();

    dblog3 << myname << "(onWebSocketSession): finish session for " << req.clientAddress().toString() << endl;
}
// -----------------------------------------------------------------------------
std::shared_ptr<LogDB::LogWebSocket> LogDB::newWebSocket( Poco::Net::HTTPServerRequest* req,
        Poco::Net::HTTPServerResponse* resp,
        const std::string& logname,
        const Poco::URI::QueryParameters& params )
{
    using Poco::Net::WebSocket;
    using Poco::Net::WebSocketException;
    using Poco::Net::HTTPResponse;
    using Poco::Net::HTTPServerRequest;

    std::shared_ptr<Log> log;

    for( const auto& s : logservers )
    {
        if( s->name == logname )
        {
            log = s;
            break;
        }
    }

    if( !log )
    {
        resp->setStatus(HTTPResponse::HTTP_BAD_REQUEST);
        resp->setContentType(httpHtmlContentType);
        resp->setStatusAndReason(HTTPResponse::HTTP_NOT_FOUND);
        std::ostream& err = resp->send();
        err << "Not found '" << logname << "'";
        err.flush();
        return nullptr;
    }

    for( const auto& p : params )
    {
        if( p.first == "set" )
        {
            if( p.second.empty() )
                break;

            if( log->usercmd.empty() )
                log->usercmd = "-s " + p.second;
            else
            {
                dbwarn << log->peername << ": Ignored set log level '" << p.second << "' because already set in other connection" << endl;
            }

            break;
        }
    }

    std::shared_ptr<LogWebSocket> ws;

    {
        uniset_rwmutex_wrlock lock(wsocksMutex);
        ws = make_shared<LogWebSocket>(req, resp, log);
        ws->setHearbeatTime(wsHeartbeatTime_sec);
        ws->setSendPeriod(wsSendTime_sec);
        ws->setMaxSendCount(wsMaxSend);
        ws->dblog = dblog;
        wsocks.emplace_back(ws);
    }
    // wsocksMutex надо отпустить, прежде чем посылать сигнал
    // т.е. в обработчике происходит его захват
    wsactivate.send();
    return ws;
}
// -----------------------------------------------------------------------------
void LogDB::delWebSocket( std::shared_ptr<LogWebSocket>& ws )
{
    uniset_rwmutex_wrlock lock(wsocksMutex);

    for( auto it = wsocks.begin(); it != wsocks.end(); it++ )
    {
        if( (*it).get() == ws.get() )
        {
            dblog3 << myname << ": delete websocket " << endl;
            wsocks.erase(it);
            return;
        }
    }
}
// -----------------------------------------------------------------------------
LogDB::LogWebSocket::LogWebSocket(Poco::Net::HTTPServerRequest* _req,
                                  Poco::Net::HTTPServerResponse* _resp,
                                  std::shared_ptr<Log>& _log ):
    Poco::Net::WebSocket(*_req, *_resp),
    req(_req),
    resp(_resp),
    log(_log)
{
    setBlocking(false);

    cancelled = false;

    con = _log->signal_on_read().connect( sigc::mem_fun(*this, &LogWebSocket::add));

    // т.к. создание websocket-а происходит в другом потоке
    // то активация и привязка к loop происходит в функции set()
    // вызываемой из eventloop
    ioping.set<LogDB::LogWebSocket, &LogDB::LogWebSocket::ping>(this);
    iosend.set<LogDB::LogWebSocket, &LogDB::LogWebSocket::send>(this);

    maxsize = maxsend * 10; // пока так
}
// -----------------------------------------------------------------------------
LogDB::LogWebSocket::~LogWebSocket()
{
    if( !cancelled )
        term();

    // удаляем всё что осталось
    while(!wbuf.empty())
    {
        delete wbuf.front();
        wbuf.pop();
    }

    if( log )
        log->usercmd = "";
}
// -----------------------------------------------------------------------------
bool LogDB::LogWebSocket::isActive()
{
    return iosend.is_active();
}
// -----------------------------------------------------------------------------
void LogDB::LogWebSocket::set( ev::dynamic_loop& loop )
{
    iosend.set(loop);
    ioping.set(loop);

    iosend.start(0, send_sec);
    ioping.start(ping_sec, ping_sec);

    if( !log->usercmd.empty() )
        log->setCommand(log->usercmd);
}
// -----------------------------------------------------------------------------
void LogDB::LogWebSocket::send( ev::timer& t, int revents )
{
    if( EV_ERROR & revents )
        return;

    for( size_t i = 0; !wbuf.empty() && i < maxsend && !cancelled; i++ )
        write();
}
// -----------------------------------------------------------------------------
void LogDB::LogWebSocket::ping( ev::timer& t, int revents )
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

    wbuf.emplace(new UTCPCore::Buffer("."));

    if( ioping.is_active() )
        ioping.stop();
}
// -----------------------------------------------------------------------------
void LogDB::LogWebSocket::add( LogDB::Log* log, const string& txt )
{
    if( cancelled || txt.empty())
        return;

    if( wbuf.size() > maxsize )
    {
        dbwarn << req->clientAddress().toString() << " lost messages..." << endl;
        return;
    }

    wbuf.emplace(new UTCPCore::Buffer(txt));

    if( ioping.is_active() )
        ioping.stop();
}
// -----------------------------------------------------------------------------
void LogDB::LogWebSocket::write()
{
    UTCPCore::Buffer* msg = 0;

    if( wbuf.empty() )
    {
        if( !ioping.is_active() )
            ioping.start(ping_sec, ping_sec);

        return;
    }

    msg = wbuf.front();

    if( !msg )
        return;

    using Poco::Net::WebSocket;
    using Poco::Net::WebSocketException;
    using Poco::Net::HTTPResponse;
    using Poco::Net::HTTPServerRequest;

    int flags = WebSocket::FRAME_TEXT;

    if( msg->len == 1 ) // это пинг состоящий из "."
        flags = WebSocket::FRAME_FLAG_FIN | WebSocket::FRAME_OP_PING;

    try
    {
        ssize_t ret = sendFrame(msg->dpos(), msg->nbytes(), flags);

        if( ret < 0 )
        {
            int errnum = errno;

            dblog3 << "(websocket): " << req->clientAddress().toString()
                   << "  write to socket error(" << errnum << "): " << strerror(errnum) << endl;

            if( errnum == EPIPE || errnum == EBADF )
            {
                dblog3 << "(websocket): "
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
            delete msg;
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
        dblog3 << "(websocket):NetException: "
               << req->clientAddress().toString()
               << " error: " << e.displayText()
               << endl;
    }
    catch( Poco::IOException& ex )
    {
        dblog3 << "(websocket): IOException: "
               << req->clientAddress().toString()
               << " error: " << ex.displayText()
               << endl;
    }

    term();
}
// -----------------------------------------------------------------------------
void LogDB::LogWebSocket::term()
{
    if( cancelled )
        return;

    if( log && !log->usercmd.empty() )
        log->usercmd = "";

    cancelled = true;
    con.disconnect();
    ioping.stop();
    iosend.stop();
    finish.notify_all();
}
// -----------------------------------------------------------------------------
void LogDB::LogWebSocket::waitCompletion()
{
    std::unique_lock<std::mutex> lk(finishmut);

    while( !cancelled )
        finish.wait(lk);
}
// -----------------------------------------------------------------------------
void LogDB::LogWebSocket::setHearbeatTime( const double& sec )
{
    if( sec > 0 )
        ping_sec = sec;
}
// -----------------------------------------------------------------------------
void LogDB::LogWebSocket::setSendPeriod ( const double& sec )
{
    if( sec > 0 )
        send_sec = sec;
}
// -----------------------------------------------------------------------------
void LogDB::LogWebSocket::setMaxSendCount( size_t val )
{
    if( val > 0 )
        maxsend = val;
}
// -----------------------------------------------------------------------------
void LogDB::httpWebSocketPage( std::ostream& ostr, Poco::Net::HTTPServerRequest& req,
                               Poco::Net::HTTPServerResponse& resp,
                               const Poco::URI::QueryParameters& p )
{
    using Poco::Net::HTTPResponse;

    //    resp.setChunkedTransferEncoding(true);
    resp.setContentType(httpHtmlContentType);

    ostr << "<html>" << endl;
    ostr << "<head>" << endl;
    ostr << "<title>" << myname << ": log servers list</title>" << endl;
    ostr << "<meta http-equiv=\"Content-Type\" content=\"" << httpHtmlContentType << "\">" << endl;
    ostr << "</head>" << endl;
    ostr << "<body>" << endl;
    ostr << "<h1>servers:</h1>" << endl;
    ostr << "<ul>" << endl;

    for( const auto& l : logservers )
    {
        ostr << "  <li><a target='_blank' href=\"http://"
             << ( httpReplyAddr.empty() ? req.serverAddress().toString() : httpReplyAddr )
             << "/ws/connect/" << l->name << "\">"
             << l->name << "</a>  &#8211; "
             << "<i>" << l->description << "</i></li>"
             << endl;
    }

    ostr << "</ul>" << endl;
    ostr << "</body>" << endl;
}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr LogDB::httpLogControl( std::ostream& out, Poco::Net::HTTPServerRequest& req,
        Poco::Net::HTTPServerResponse& resp,
        const std::string& logname,
        const Poco::URI::QueryParameters& params )
{
    if( !httpEnabledLogControl )
        return respError(resp, Poco::Net::HTTPResponse::HTTP_SERVICE_UNAVAILABLE,
                         "logcontrol API disabled for this server");

    if( params.size() < 1 )
    {
        dblog3 << "(httpLogControl): Unknown params for logname '" << logname << "'" << endl;
        return respError(resp, Poco::Net::HTTPResponse::HTTP_BAD_REQUEST, "Unknown params for '" + logname + "'");
    }

    const auto& api = params[0].first;
    const auto& cmd = params[0].second;

    Poco::JSON::Object::Ptr jdata = new Poco::JSON::Object();
    jdata->set("name", logname);
    bool found = false;

    for( auto& ls : logservers )
    {
        if( ls->name != logname )
            continue;

        dblog3 << "(httpLogControl): logname '" << logname << "' API '" << api << "' command: " << cmd << endl;
        jdata->set("cmd", cmd);

        if( api == "set" )
        {
            if( params[0].second.empty() )
            {
                dblog3 << "(httpLogControl): Unknown params for logname '" << logname << "' API 'set'" << endl;
                return respError(resp, Poco::Net::HTTPResponse::HTTP_BAD_REQUEST, "Unknown params for '" + logname + "'");
            }

            ls->setCommand("-s " + cmd);
        }
        else if( api == "reset" )
        {
            dblog3 << "(httpLogControl): logname '" << logname << "' API 'reset' command: " << ls->cmd << endl;
            ls->setCommand(ls->cmd);
        }
        else if( api == "get" )
            jdata->set("cmd", ls->lastcmd);

        found = true;
        break;
    }

    if( !found )
    {
        dblog3 << "(httpLogControl): not found logname '" << logname << "'" << endl;
        return respError(resp, Poco::Net::HTTPResponse::HTTP_NOT_FOUND, "Not found '" + logname + "'");
    }

    return jdata;
}
// -----------------------------------------------------------------------------
static void replaceAll(std::string& str, const std::string& from, const std::string& to)
{
    if( from.empty() )
        return;

    size_t start_pos = 0;

    while((start_pos = str.find(from, start_pos)) != std::string::npos)
    {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}
// -----------------------------------------------------------------------------
void LogDB::httpWebSocketConnectPage( Poco::Net::HTTPServerRequest& req,
                                      Poco::Net::HTTPServerResponse& resp,
                                      const std::string& logname,
                                      const Poco::URI::QueryParameters& params )
{
    resp.setChunkedTransferEncoding(true);
    resp.setContentType("text/html; charset=UTF-8");

    std::string qparams = "";

    for( const auto& p : params )
    {
        if( p.first == "set" )
        {
            if( !httpEnabledLogControl )
            {
                dbwarn << "(httpWebSocketConnectPage): Ignored set log level command. 'Set' - disabled for this server" << endl;
            }
            else
                qparams = p.second;

            break;
        }
    }

    if( !qparams.empty() )
        qparams = "?set=" + qparams;

    std::string serverAddr = httpReplyAddr.empty() ? req.serverAddress().toString() : httpReplyAddr;
    std::ifstream file(wsPageTemplate);

    if( !file.is_open() )
    {
        dbcrit << "(httpWebSocketConnectPage): Can't open file: '" << wsPageTemplate << "'" << endl;
        resp.setStatusAndReason(Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
        resp.setContentLength(0);
        resp.send();
        return;
    }

    std::ostream& out = resp.send();
    std::string line;

    while( std::getline(file, line) )
    {
        replaceAll(line, "{{LOGNAME}}", logname);
        replaceAll(line, "{{ADDR}}", serverAddr);
        replaceAll(line, "{{QPARAMS}}", qparams);
        out << line;
    }

    out.flush();
}
// -----------------------------------------------------------------------------
#endif
// -----------------------------------------------------------------------------
