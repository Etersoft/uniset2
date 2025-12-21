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
#ifndef LogServer_H_
#define LogServer_H_
// -------------------------------------------------------------------------
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <ev++.h>
#include <Poco/JSON/Object.h>
#include "Mutex.h"
#include "UniXML.h"
#include "DebugStream.h"
#include "ThreadCreator.h"
#include "UTCPSocket.h"
#include "CommonEventLoop.h"
#include "LogServerTypes.h"

#ifndef DISABLE_REST_API
#include <Poco/JSON/Object.h>
#endif
// -------------------------------------------------------------------------
namespace uniset
{
    // -------------------------------------------------------------------------
    class LogSession;
    class LogAgregator;
    class NullLogSession;
    // -------------------------------------------------------------------------
    /*! \page pgLogServer Лог сервер
        Лог сервер предназначен для возможности удалённого чтения логов (DebugStream).
    Ему указывается host и port на котором он отвечает, подключаться можно при помощи
    LogReader. Читающих клиентов может быть сколько угодно много, на каждого создаётся своя "сессия"(LogSession).
    При этом через лог сервер имеется возможность управлять включением или отключением определённых уровней логов,
    записью, отключением записи или ротацией файла с логами.  DebugStream за которым ведётся "слежение"
    задаётся в конструкторе для LogServer.

     По умолчанию, при завершении \b ВСЕХ подключений, LogServer автоматически восстанавливает уровни логов,
     которые были на момент \b первого \b подключения.
     Но если была передана команда LogServerTypes::cmdSaveLogLevel (в любом из подключений), то будут сохранены те уровни,
     которые выставлены подключившимся клиентом.
     Для этого LogServer подключается к сигналу на получение команд у каждой сессии и сам обрабатывает команды,
     на сохранение, восстановление и показ текущих "умолчательных" логов.

    \code
       DebugStream mylog;
       LogServer logsrv(mylog);
       ...
       logsrv.run(host,port,create_thread);
       ...
    \endcode

    При этом если необходимо управлять или читать сразу несколько логов можно воспользоваться специальным классом LogAgregator.
    \code
        auto log1 = make_shared<DebugStream>();
        log1->setLogName("log1");

        auto log2 = make_shared<DebugStream>();
        log2->setLogName("log2");

        LogAgregator la;
        la.add(log1);
        la.add(log2);

        LogServer logsrv(la);
        ...
        logsrv.run(host,port,create_thread);
        ...
    \endcode

    \warning Логи отдаются "клиентам" только целиком строкой. Т.е. по сети информация передаваться не будет пока не будет записан "endl".
        Это сделано для "оптимизации передачи" (чтобы не передавать каждый байт)

    \warning Т.к. LogServer в основном только отдаёт "клиентам" логи, то он реализован с использованием CommonEventLoop,
    т.е. у всех LogServer будет ОДИН ОБЩИЙ event loop.
    */
    // -------------------------------------------------------------------------
    class LogServer:
        protected EvWatcher
    {
        public:

            LogServer( std::shared_ptr<DebugStream> log );
            LogServer( std::shared_ptr<LogAgregator> log );
            virtual ~LogServer() noexcept;

            void setCmdTimeout( timeout_t msec ) noexcept;
            void setSessionLog( Debug::type t ) noexcept;
            void setMaxSessionCount( size_t num ) noexcept;

            // port <= 0: select an available port automatically
            bool async_run( const std::string& addr, int port );
            bool run( const std::string& addr, int port );

            void terminate();

            bool isRunning() const noexcept;

            bool check( bool restart_if_fail = true );

            void init( const std::string& prefix, xmlNode* cnode = nullptr, int argc = 0, const char* const argv[] = nullptr );

            static std::string help_print( const std::string& prefix );

            std::string getShortInfo();

#ifndef DISABLE_REST_API
            Poco::JSON::Object::Ptr httpGetShortInfo();
            static Poco::JSON::Object::Ptr httpLogServerInfo( const std::shared_ptr<LogServer>& logserv,
                                                              const std::string& host,
                                                              int port );
#endif

        protected:
            LogServer();

            virtual void evprepare( const ev::loop_ref& loop ) override;
            virtual void evfinish( const ev::loop_ref& loop ) override;
            virtual std::string wname() const noexcept override;

            void ioAccept( ev::io& watcher, int revents );
            void sessionFinished( LogSession* s );
            void saveDefaultLogLevels( const std::string& logname );
            void restoreDefaultLogLevels( const std::string& logname );
            std::string onCommand( LogSession* s, LogServerTypes::Command cmd, const std::string& logname );

        private:

            timeout_t cmdTimeout = { 2000 };
            Debug::type sessLogLevel = { Debug::NONE };
            size_t sessMaxCount = { 10 };

            typedef std::vector< std::shared_ptr<LogSession> > SessionList;
            SessionList slist;
            uniset::uniset_rwmutex mutSList;

            DebugStream mylog;
            ev::io io;

            // делаем loop общим.. одним на всех!
            static CommonEventLoop loop;

            std::shared_ptr<UTCPSocket> sock;
            std::shared_ptr<DebugStream> elog; // eventlog..

            // map с уровнями логов по умолчанию (инициализируются при создании первой сессии),
            // (они необходимы для восстановления настроек после завершения всех (!) сессий)
            // т.к. shared_ptr-ов может быть много, то в качестве ключа используем указатель на "реальный объект"(внутри shared_ptr)
            // но только для этого(!), пользоваться этим указателем ни в коем случае нельзя (и нужно проверять shared_ptr на существование)
            std::unordered_map< DebugStream*, Debug::type > defaultLogLevels;

            std::string myname = { "LogServer" };
            std::string addr = { "" };
            Poco::UInt16 port = { 0 };

            std::atomic_bool isrunning = { false };
    };
    // -------------------------------------------------------------------------
} // end of uniset namespace
// -------------------------------------------------------------------------
#endif // LogServer_H_
// -------------------------------------------------------------------------
