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
#include <queue>
#include <cc++/socket.h>
#include <ev++.h>
#include "Mutex.h"
#include "DebugStream.h"
#include "LogAgregator.h"
#include "USocket.h"
#include "UTCPCore.h"
// -------------------------------------------------------------------------
/*! Реализация "сессии" для клиентов LogServer. */
class LogSession
{
	public:

		LogSession(int sock, std::shared_ptr<DebugStream>& log, timeout_t cmdTimeout = 2000 );
		~LogSession();

		typedef sigc::slot<void, LogSession*> FinalSlot;
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
			mylog.level(t);
		}
		inline void addSessionLogLevel( Debug::type t )
		{
			mylog.addLevel(t);
		}
		inline void delSessionLogLevel( Debug::type t )
		{
			mylog.delLevel(t);
		}

		// запуск обработки входящих запросов
		void run( ev::loop_ref& loop );
		void terminate();

		bool isAcive();

	protected:
		LogSession( ost::TCPSocket& server );

		void event( ev::async& watcher, int revents );
		void callback( ev::io& watcher, int revents );
		void readEvent( ev::io& watcher );
		void writeEvent( ev::io& watcher );
		size_t readData( unsigned char* buf, int len );
		void cmdProcessing( const std::string& cmdLogName, const LogServerTypes::lsMessage& msg );
		void onCmdTimeout( ev::timer& watcher, int revents );
		void final();

		void logOnEvent( const std::string& s );

		timeout_t cmdTimeout = { 2000 };

	private:
		std::queue<UTCPCore::Buffer*> logbuf;
		std::mutex logbuf_mutex;

		std::string peername = { "" };
		std::string caddr = { "" };
		std::shared_ptr<DebugStream> log;
		std::shared_ptr<LogAgregator> alog;
		sigc::connection conn;

		std::shared_ptr<USocket> sock;

		ev::io  io;
		ev::timer  cmdTimer;
		ev::async  asyncEvent;
		std::mutex io_mutex;

		FinalSlot slFin;
		std::atomic_bool cancelled = { false };

		DebugStream mylog;
};
// -------------------------------------------------------------------------
#endif // LogSession_H_
// -------------------------------------------------------------------------
