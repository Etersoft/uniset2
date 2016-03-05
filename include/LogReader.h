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
#ifndef LogReader_H_
#define LogReader_H_
// -------------------------------------------------------------------------
#include <string>
#include <memory>
#include <queue>
#include <vector>
#include <cc++/socket.h>
#include "UTCPStream.h"
#include "DebugStream.h"
#include "LogServerTypes.h"
// -------------------------------------------------------------------------
class LogReader
{
	public:

		LogReader();
		~LogReader();

		struct Command
		{
			Command( LogServerTypes::Command c, unsigned int d, const std::string& f = "" ): cmd(c), data(d), logfilter(f) {}

			LogServerTypes::Command cmd = { LogServerTypes::cmdNOP };
			unsigned int data = {0};
			std::string logfilter = { "" };
		};

		void sendCommand( const std::string& addr, ost::tpport_t port,
						  std::vector<Command>& vcmd, bool cmd_only = true,
						  bool verbose = false );

		void readlogs( const std::string& addr, ost::tpport_t port, LogServerTypes::Command c = LogServerTypes::cmdNOP, const std::string logfilter = "", bool verbose = false );

		bool isConnection();

		inline void setReadCount( unsigned int n )
		{
			readcount = n;
		}

		inline void setCommandOnlyMode( bool s )
		{
			cmdonly = s;
		}

		inline void setinTimeout( timeout_t msec )
		{
			inTimeout = msec;
		}
		inline void setoutTimeout( timeout_t msec )
		{
			outTimeout = msec;
		}
		inline void setReconnectDelay( timeout_t msec )
		{
			reconDelay = msec;
		}

		DebugStream::StreamEvent_Signal signal_stream_event();

		void setLogLevel( Debug::type t );

		inline std::shared_ptr<DebugStream> log(){ return outlog; }

	protected:

		void connect( const std::string& addr, ost::tpport_t port, timeout_t tout = TIMEOUT_INF );
		void connect( ost::InetAddress addr, ost::tpport_t port, timeout_t tout = TIMEOUT_INF );
		void disconnect();
		void logOnEvent( const std::string& s );
		void sendCommand(LogServerTypes::lsMessage& msg, bool verbose = false );

		timeout_t inTimeout = { 10000 };
		timeout_t outTimeout = { 6000 };
		timeout_t reconDelay = { 5000 };

	private:
		std::shared_ptr<UTCPStream> tcp;
		std::string iaddr = { "" };
		ost::tpport_t port = { 0 };
		bool cmdonly { false };
		unsigned int readcount = { 0 }; // количество циклов чтения

		DebugStream rlog;
		std::shared_ptr<DebugStream> outlog; // рабочий лог в который выводиться полученная информация..

		DebugStream::StreamEvent_Signal m_logsig;
};
// -------------------------------------------------------------------------
#endif // LogReader_H_
// -------------------------------------------------------------------------
