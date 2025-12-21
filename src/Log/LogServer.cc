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
// -------------------------------------------------------------------------
#include <sstream>
#include <iomanip>
#include <Poco/Net/NetException.h>
#include "LogServer.h"
#include "UniSetTypes.h"
#include "Exceptions.h"
#include "LogSession.h"
#include "LogAgregator.h"
#include "Configuration.h"
// -------------------------------------------------------------------------
namespace uniset
{
    // -------------------------------------------------------------------------
    using namespace std;
    // -------------------------------------------------------------------------
    CommonEventLoop LogServer::loop;
    // -------------------------------------------------------------------------
    LogServer::~LogServer() noexcept
    {
        try
        {
            if( isrunning )
                loop.evstop(this);
        }
        catch(...) {}
    }
    // -------------------------------------------------------------------------
    void LogServer::setCmdTimeout( timeout_t msec ) noexcept
    {
        cmdTimeout = msec;
    }
    // -------------------------------------------------------------------------
    void LogServer::setSessionLog( Debug::type t ) noexcept
    {
        sessLogLevel = t;
    }
    // -------------------------------------------------------------------------
    void LogServer::setMaxSessionCount( size_t num ) noexcept
    {
        sessMaxCount = num;
    }
    // -------------------------------------------------------------------------
    LogServer::LogServer( std::shared_ptr<LogAgregator> log ):
        LogServer()
    {
        elog = dynamic_pointer_cast<DebugStream>(log);

        if( !elog )
        {
            ostringstream err;
            err << myname << "(LogServer): dynamic_pointer_cast FAILED! ";

            if( mylog.is_info() )
                mylog.info() << myname << "(evfinish): terminate..." << endl;

            if( mylog.is_crit() )
                mylog.crit() << err.str() << endl;

            cerr << err.str()  << endl;

            throw SystemError(err.str());
        }
    }
    // -------------------------------------------------------------------------
    LogServer::LogServer( std::shared_ptr<DebugStream> log ):
        LogServer()
    {
        elog = log;
    }
    // -------------------------------------------------------------------------
    LogServer::LogServer():
        cmdTimeout(2000),
        sessLogLevel(Debug::NONE),
        sock(nullptr),
        elog(nullptr)
    {
        slist.reserve(sessMaxCount);
    }
    // -------------------------------------------------------------------------
    void LogServer::evfinish( const ev::loop_ref& loop )
    {
        if( !isrunning )
            return;

        if( mylog.is_info() )
            mylog.info() << myname << "(evfinish): terminate..." << endl;

        auto lst(slist);

        // Копируем сперва себе список сессий..
        // т.к при вызове terminate()
        // у Session будет вызван сигнал "final"
        // который приведёт к вызову sessionFinished(), в котором список будет меняться..
        for( const auto& s : lst )
        {
            try
            {
                s->terminate();
            }
            catch( std::exception& ex ) {}
        }

        io.stop();
        isrunning = false;

        sock->close();
        sock.reset();

        if( mylog.is_info() )
            mylog.info() << myname << "(LogServer): finished." << endl;
    }
    // -------------------------------------------------------------------------
    std::string LogServer::wname() const noexcept
    {
        return myname;
    }
    // -------------------------------------------------------------------------
    static Poco::UInt16 normalize_port( int port ) noexcept
    {
        if( port <= 0 )
            return 0;
        if( port > 0xFFFF )
            return 0;
        return static_cast<Poco::UInt16>(port);
    }
    // -------------------------------------------------------------------------
    bool LogServer::run( const std::string& _addr, int _port )
    {
        addr = _addr;
        port = normalize_port(_port);

        {
            ostringstream s;
            s << _addr << ":" << port;
            myname = s.str();
        }

        return loop.evrun(this);
    }
    // -------------------------------------------------------------------------
    bool LogServer::async_run( const std::string& _addr, int _port )
    {
        addr = _addr;
        port = normalize_port(_port);

        {
            ostringstream s;
            s << _addr << ":" << port;
            myname = s.str();
        }
        return loop.async_evrun(this);
    }
    // -------------------------------------------------------------------------
    void LogServer::terminate()
    {
        loop.evstop(this);
    }
    // -------------------------------------------------------------------------
    bool LogServer::isRunning() const noexcept
    {
        return isrunning;
    }
    // -------------------------------------------------------------------------
    bool LogServer::check( bool restart_if_fail )
    {
        // смущает пока только, что эта функция будет вызыватся (обычно) из другого потока
        // и как к этому отнесётся evloop

        try
        {
            // для проверки пробуем открыть соединение..
            UTCPStream s;
            s.create(addr, port, 500);
            s.disconnect();
            return true;
        }
        catch(...) {}

        if( !restart_if_fail )
            return false;

        io.stop();

        if( !sock )
        {
            try
            {
                evprepare(io.loop);
            }
            catch( uniset::SystemError& ex )
            {
                if( mylog.is_crit() )
                    mylog.crit() <<  myname << "(evprepare): " << ex << endl;

                return false;
            }
        }

        if( !io.is_active() )
        {
            io.set<LogServer, &LogServer::ioAccept>(this);
            io.start(sock->getSocket(), ev::READ);
            isrunning = true;
        }

        // Проверяем..
        try
        {
            UTCPStream s;
            s.create(addr, port, 500);
            s.disconnect();
            return true;
        }
        catch( Poco::Net::NetException& ex )
        {
            ostringstream err;
            err << myname << "(check): socket error:" << ex.message();

            if( mylog.is_crit() )
                mylog.crit() << err.str() << endl;
        }

        return false;
    }
    // -------------------------------------------------------------------------
    void LogServer::evprepare( const ev::loop_ref& eloop )
    {
        if( sock )
        {
            ostringstream err;
            err << myname << "(evprepare): socket ALREADY BINDINNG..";

            if( mylog.is_crit() )
                mylog.crit() << err.str() << endl;

            throw uniset::SystemError( err.str() );
        }

        try
        {
            sock = make_shared<UTCPSocket>(addr, port);
        }
        catch( Poco::Net::NetException& ex )
        {
            ostringstream err;

            err << myname << "(evprepare): socket error:" << ex.message();

            if( mylog.is_crit() )
                mylog.crit() << err.str() << endl;

            throw uniset::SystemError( err.str() );
        }
        catch( std::exception& ex )
        {
            ostringstream err;

            err << myname << "(evprepare): " << ex.what();

            if( mylog.is_crit() )
                mylog.crit() << err.str() << endl;

            throw uniset::SystemError( err.str() );
        }

        if( port == 0 )
        {
            try
            {
                port = sock->address().port();
                ostringstream s;
                s << addr << ":" << port;
                myname = s.str();
            }
            catch( ... ) {}
        }

        sock->setBlocking(false);

        io.set( eloop );
        io.set<LogServer, &LogServer::ioAccept>(this);
        io.start(sock->getSocket(), ev::READ);
        isrunning = true;
    }
    // -------------------------------------------------------------------------
    void LogServer::ioAccept( ev::io& watcher, int revents )
    {
        if( EV_ERROR & revents )
        {
            if( mylog.is_crit() )
                mylog.crit() << myname << "(LogServer::ioAccept): invalid event" << endl;

            return;
        }

        if( !isrunning )
        {
            if( mylog.is_crit() )
                mylog.crit() << myname << "(LogServer::ioAccept): terminate work.." << endl;

            sock->close();
            return;
        }

        {
            uniset_rwmutex_rlock l(mutSList);

            if( slist.size() >= sessMaxCount )
            {
                if( mylog.is_crit() )
                    mylog.crit() << myname << "(LogServer::ioAccept): session limit(" << sessMaxCount << ")" << endl;

                sock->close();
                return;
            }
        }

        try
        {
            Poco::Net::StreamSocket ss = sock->acceptConnection();

            auto s = make_shared<LogSession>( ss, elog, cmdTimeout );
            s->setSessionLogLevel(sessLogLevel);
            s->connectFinalSession( sigc::mem_fun(this, &LogServer::sessionFinished) );
            s->signal_logsession_command().connect( sigc::mem_fun(this, &LogServer::onCommand) );
            {
                uniset_rwmutex_wrlock l(mutSList);
                slist.push_back(s);

                // на первой сессии запоминаем состояние логов
                if( slist.size() == 1 )
                    saveDefaultLogLevels("ALL");
            }

            s->run(watcher.loop);
        }
        catch( const std::exception& ex )
        {
            mylog.warn() << "(LogServer::ioAccept): catch exception: " << ex.what() << endl;
        }
    }
    // -------------------------------------------------------------------------
    void LogServer::sessionFinished( LogSession* s )
    {
        uniset_rwmutex_wrlock l(mutSList);

        for( SessionList::iterator i = slist.begin(); i != slist.end(); ++i )
        {
            if( i->get() == s )
            {
                slist.erase(i);
                break;
            }
        }

        if( slist.empty() )
        {
            // восстанавливаем уровни логов по умолчанию
            restoreDefaultLogLevels("ALL");
        }
    }
    // -------------------------------------------------------------------------
    void LogServer::init( const std::string& prefix, xmlNode* cnode, int argc, const char* const argv[] )
    {
        int ac = argc;
        auto av = argv;
        if( argv == nullptr ) {
            auto conf = uniset_conf();
            if( conf )
            {
                ac = conf->getArgc();
                av = conf->getArgv();
            }
        }

        // можем на cnode==0 не проверять, т.е. UniXML::iterator корректно отрабатывает эту ситуацию
        UniXML::iterator it(cnode);

        timeout_t cmdTimeout = uniset::getArgPInt("--" + prefix + "-cmd-timeout", ac, av, it.getProp("cmdTimeout"), 2000);
        setCmdTimeout(cmdTimeout);
    }
    // -----------------------------------------------------------------------------
    std::string LogServer::help_print( const std::string& prefix )
    {
        std::ostringstream h;
        h << "--" << prefix << "-cmd-timeout msec      - Timeout for wait command. Default: 2000 msec." << endl;
        return h.str();
    }
    // -----------------------------------------------------------------------------
    string LogServer::getShortInfo()
    {
        std::ostringstream inf;

        inf << "LogServer: " << myname
            << " ["
            << " sessMaxCount=" << sessMaxCount
            << " ]"
            << endl;

        {
            uniset_rwmutex_rlock l(mutSList);

            for( const auto& s : slist )
                inf << " " << s->getShortInfo() << endl;
        }

        return inf.str();
    }
    // -----------------------------------------------------------------------------
#ifndef DISABLE_REST_API
    Poco::JSON::Object::Ptr LogServer::httpGetShortInfo()
    {
        Poco::JSON::Object::Ptr jdata = new Poco::JSON::Object();
        jdata->set("name", myname);
        jdata->set("host", addr);
        jdata->set("port", port);
        jdata->set("sessMaxCount", sessMaxCount);

        {
            uniset_rwmutex_rlock l(mutSList);

            Poco::JSON::Array::Ptr jsess = new Poco::JSON::Array();
            jdata->set("sessions", jsess);

            for( const auto& s : slist )
                jsess->add(s->httpGetShortInfo());
        }

        return jdata;
    }

    Poco::JSON::Object::Ptr LogServer::httpLogServerInfo( const std::shared_ptr<LogServer>& logserv,
                                                          const std::string& host,
                                                          int port )
    {
        Poco::JSON::Object::Ptr jls = new Poco::JSON::Object();

        if( logserv )
        {
            jls->set("state", logserv->isRunning() ? "RUNNING" : "STOPPED");
            auto info = logserv->httpGetShortInfo();

            if( info )
            {
                if( info->has("host") )
                    jls->set("host", info->get("host"));
                if( info->has("port") )
                    jls->set("port", info->get("port"));
                jls->set("info", info);
            }
            else
            {
                jls->set("host", host);
                jls->set("port", port);
            }
        }
        else
        {
            jls->set("state", "NOT_CONFIGURED");
            jls->set("host", host);
            jls->set("port", port);
        }

        return jls;
    }
#endif // #ifndef DISABLE_REST_API
    // -----------------------------------------------------------------------------
    void LogServer::saveDefaultLogLevels( const std::string& logname )
    {
        if( mylog.is_info() )
            mylog.info() << myname << "(saveDefaultLogLevels): SAVE DEFAULT LOG LEVELS.." << endl;

        auto alog = dynamic_pointer_cast<LogAgregator>(elog);

        if( alog )
        {
            std::list<LogAgregator::iLog> lst;

            if( logname.empty() || logname == "ALL" )
                lst = alog->getLogList();
            else
                lst = alog->getLogList(logname);

            for( auto&& l : lst )
                defaultLogLevels[l.log.get()] = l.log->level();
        }
        else if( elog )
            defaultLogLevels[elog.get()] = elog->level();
    }
    // -----------------------------------------------------------------------------
    void LogServer::restoreDefaultLogLevels( const std::string& logname )
    {
        if( mylog.is_info() )
            mylog.info() << myname << "(restoreDefaultLogLevels): RESTORE DEFAULT LOG LEVELS.." << endl;

        auto alog = dynamic_pointer_cast<LogAgregator>(elog);

        if( alog )
        {
            std::list<LogAgregator::iLog> lst;

            if( logname.empty() || logname == "ALL" )
                lst = alog->getLogList();
            else
                lst = alog->getLogList(logname);

            for( auto&& l : lst )
            {
                auto d = defaultLogLevels.find(l.log.get());

                if( d != defaultLogLevels.end() )
                    l.log->level(d->second);
            }
        }
        else if( elog )
        {
            auto d = defaultLogLevels.find(elog.get());

            if( d != defaultLogLevels.end() )
                elog->level(d->second);
        }
    }
    // -----------------------------------------------------------------------------
    std::string LogServer::onCommand( LogSession* s, LogServerTypes::Command cmd, const std::string& logname )
    {
        if( cmd == LogServerTypes::cmdSaveLogLevel )
        {
            saveDefaultLogLevels(logname);
        }
        else if( cmd == LogServerTypes::cmdRestoreLogLevel )
        {
            restoreDefaultLogLevels(logname);
        }
        else if( cmd == LogServerTypes::cmdViewDefaultLogLevel )
        {
            ostringstream s;
            s << "List of saved default log levels (filter='" << logname << "')[" << defaultLogLevels.size() << "]: " << endl;
            s << "=================================" << endl;
            auto alog = dynamic_pointer_cast<LogAgregator>(elog);

            if( alog ) // если у нас "агрегатор", то работаем с его списком потоков
            {
                std::list<LogAgregator::iLog> lst;

                if( logname.empty() || logname == "ALL" )
                    lst = alog->getLogList();
                else
                    lst = alog->getLogList(logname);

                std::string::size_type max_width = 1;

                // ищем максимальное название для выравнивания по правому краю
                for( const auto& l : lst )
                    max_width = std::max(max_width, l.name.length() );

                for( const auto& l : lst )
                {
                    Debug::type deflevel = Debug::NONE;
                    auto i = defaultLogLevels.find(l.log.get());

                    if( i != defaultLogLevels.end() )
                        deflevel = i->second;

                    s << std::left << setw(max_width) << l.name << std::left << " [ " << Debug::str(deflevel) << " ]" << endl;
                }
            }
            else if( elog )
            {
                Debug::type deflevel = Debug::NONE;
                auto i = defaultLogLevels.find(elog.get());

                if( i != defaultLogLevels.end() )
                    deflevel = i->second;

                s << elog->getLogName() << " [" << Debug::str(deflevel) << " ]" << endl;
            }

            s << "=================================" << endl << endl;

            return s.str();
        }

        return "";
    }
    // -----------------------------------------------------------------------------
} // end of namespace uniset
