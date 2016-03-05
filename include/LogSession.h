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
#ifndef LogSession_H_
#define LogSession_H_
// -------------------------------------------------------------------------
#include <string>
#include <memory>
#include <deque>
#include <cc++/socket.h>
#include <condition_variable>
#include <mutex>
#include "Mutex.h"
#include "DebugStream.h"
#include "LogAgregator.h"
#include "PassiveTimer.h"
// -------------------------------------------------------------------------
/*! Реализация "сессии" для клиентов LogServer. */
class LogSession:
	public std::enable_shared_from_this<LogSession>,
	public ost::TCPSession
{
	public:

		LogSession( ost::TCPSocket& server, std::shared_ptr<DebugStream>& log, timeout_t sessTimeout = 10000, timeout_t cmdTimeout = 2000, timeout_t outTimeout = 2000, timeout_t delay = 2000 );
		virtual ~LogSession();

		typedef sigc::slot<void, std::shared_ptr<LogSession>> FinalSlot;
		void connectFinalSession( FinalSlot sl );

		inline void cancel()
		{
			cancelled = true;
		}
		inline std::string getClientAddress()
		{
			return caddr;
		}

		inline void setSessionLogLevel( Debug::type t )
		{
			slog.level(t);
		}
		inline void addSessionLogLevel( Debug::type t )
		{
			slog.addLevel(t);
		}
		inline void delSessionLogLevel( Debug::type t )
		{
			slog.delLevel(t);
		}

	protected:
		LogSession( ost::TCPSocket& server );

		virtual void run();
		virtual void final();
		void logOnEvent( const std::string& s );
		void readStream();

		// msec
		timeout_t sessTimeout = { 10000 };
		timeout_t cmdTimeout = { 2000 };
		timeout_t outTimeout = { 2000 };
		timeout_t delayTime = { 2000 };

	private:
		typedef std::deque<std::string> LogBuffer;
		LogBuffer lbuf;
		std::string peername = { "" };
		std::string caddr = { "" };
		std::shared_ptr<DebugStream> log;
		std::shared_ptr<LogAgregator> alog;
		sigc::connection conn;

		std::shared_ptr<LogSession> myptr;

		//        PassiveTimer ptSessionTimeout;
		FinalSlot slFin;
		std::atomic_bool cancelled = { false };

		DebugStream slog;
		std::ostringstream sbuf;

		std::mutex              log_mutex;
		std::condition_variable log_event;
		std::atomic_bool log_notify = ATOMIC_VAR_INIT(0);
};
// -------------------------------------------------------------------------
/*! Сессия просто заверщающаяся с указанным сообщением */
class NullLogSession:
	public ost::Thread
{
	public:

		NullLogSession( const std::string& _msg );
		~NullLogSession();

		void add( ost::TCPSocket& server );
		void setMessage( const std::string& _msg );

		inline void cancel()
		{
			cancelled = true;
		}

	protected:

		virtual void run();
		virtual void final();

	private:
		std::string msg;

		typedef std::list< std::shared_ptr<ost::TCPStream> > TCPStreamList;
		TCPStreamList slist;
		UniSetTypes::uniset_rwmutex smutex;
		std::atomic_bool cancelled;
};
// -------------------------------------------------------------------------
#endif // LogSession_H_
// -------------------------------------------------------------------------
