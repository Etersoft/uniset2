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
#include <list>
#include <string>
#include <memory>
#include <cc++/socket.h>
#include <ev++.h>
#include "Mutex.h"
#include "UniXML.h"
#include "DebugStream.h"
#include "ThreadCreator.h"
#include "UTCPSocket.h"
#include "CommonEventLoop.h"
// -------------------------------------------------------------------------
class LogSession;
class LogAgregator;
class NullLogSession;
// -------------------------------------------------------------------------
/*! \page pgLogServer Лог сервер
    Лог сервер предназначен для возможности удалённого чтения логов (DebugStream).
Ему указывается host и port для прослушивания запросов, которые можно делать при помощи
LogReader. Читающих клиентов может быть скольугодно много, на каждого создаётся своя "сессия"(LogSession).
При этом через лог сервер имеется возможность управлять включением или отключением определённых уровней логов,
записью, отключением записи или ротацией файла с логами.  DebugStream за которым ведётся "слежение"
задаётся в конструкторе для LogServer.
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

\warning Логи отдаются "клиентам" только целоиком строкой. Т.е. по сети информация передаваться не будет пока не будет записан "endl".
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
		virtual ~LogServer();

		inline void setCmdTimeout( timeout_t msec )
		{
			cmdTimeout = msec;
		}

		inline void setSessionLog( Debug::type t )
		{
			sessLogLevel = t;
		}
		inline void setMaxSessionCount( int num )
		{
			sessMaxCount = num;
		}

		void run( const std::string& addr, ost::tpport_t port, bool thread = true );
		void terminate();

		inline bool isRunning()
		{
			return isrunning;
		}

		void init( const std::string& prefix, xmlNode* cnode = 0 );

		static std::string help_print( const std::string& prefix );

	protected:
		LogServer();

		virtual void evprepare( const ev::loop_ref& loop ) override;
		virtual void evfinish( const ev::loop_ref& loop ) override;
		virtual std::string wname()
		{
			return myname;
		}

		void ioAccept( ev::io& watcher, int revents );
		void sessionFinished( LogSession* s );

	private:
		typedef std::list< std::shared_ptr<LogSession> > SessionList;
		SessionList slist;
		UniSetTypes::uniset_rwmutex mutSList;

		timeout_t timeout;
		timeout_t cmdTimeout;
		Debug::type sessLogLevel;
		size_t sessMaxCount = { 10 };

		DebugStream mylog;
		ev::io io;

		// делаем loop общим.. одним на всех!
		static CommonEventLoop loop;

		std::shared_ptr<UTCPSocket> sock;
		std::shared_ptr<DebugStream> elog; // eventlog..

		std::string myname = { "LogServer" };
		std::string addr;
		ost::tpport_t port;

		std::atomic_bool isrunning = { false };
};
// -------------------------------------------------------------------------
#endif // LogServer_H_
// -------------------------------------------------------------------------
