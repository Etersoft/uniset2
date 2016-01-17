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
#include <iostream>
#include <iomanip>
#include <memory>
#include <string>
#include <sstream>
#include <regex>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <cc++/socket.h>
#include "Exceptions.h"
#include "LogSession.h"
#include "UniSetTypes.h"
#include "LogServerTypes.h"
#include "LogAgregator.h"
// -------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
// -------------------------------------------------------------------------
LogSession::~LogSession()
{
	cancelled = true;

	conn.disconnect();

	if( io.is_active() )
		io.stop();

	if( cmdTimer.is_active() )
		cmdTimer.stop();

	if( asyncEvent.is_active() )
		asyncEvent.stop();
}
// -------------------------------------------------------------------------
LogSession::LogSession( int sfd, std::shared_ptr<DebugStream>& _log, timeout_t _cmdTimeout ):
	cmdTimeout(_cmdTimeout),
	peername(""),
	caddr(""),
	log(_log)
{
	if( log )
		conn = log->signal_stream_event().connect( sigc::mem_fun(this, &LogSession::logOnEvent) );
	else
		mylog.crit() << "LOG NULL!!" << endl;

	auto ag = dynamic_pointer_cast<LogAgregator>(log);

	if( ag )
		alog = ag;

	try
	{
		sock = make_shared<USocket>(sfd);
		ost::tpport_t p;
		ost::InetAddress iaddr = sock->getIPV4Peer(&p);

		// resolve..
		caddr = string( iaddr.getHostname() );

		ostringstream s;
		s << iaddr << ":" << p;
		peername = s.str();
	}
	catch( const ost::SockException& ex )
	{
		ostringstream err;
		err << ex.what();
		mylog.crit() << "(LogSession): err: " << err.str() << endl;
		throw SystemError(err.str());
	}

	sock->setCompletion(false);

	io.set<LogSession, &LogSession::callback>(this);
	cmdTimer.set<LogSession, &LogSession::onCmdTimeout>(this);
	asyncEvent.set<LogSession, &LogSession::event>(this);
}
// -------------------------------------------------------------------------
void LogSession::logOnEvent( const std::string& s )
{
	if( cancelled )
		return;

	std::unique_lock<std::mutex> lk(logbuf_mutex);
	logbuf.emplace(new UTCPCore::Buffer(s));

	if( asyncEvent.is_active() )
		asyncEvent.send();
}
// -------------------------------------------------------------------------
void LogSession::run( const ev::loop_ref& loop )
{
	setSessionLogLevel(Debug::ANY);

	if( mylog.is_info() )
		mylog.info() << peername << "(run): run session.." << endl;

	asyncEvent.set(loop);
	cmdTimer.set(loop);

	io.set(loop);
	io.start(sock->getSocket(), ev::READ);
	cmdTimer.start( cmdTimeout / 1000. );
}
// -------------------------------------------------------------------------
void LogSession::terminate()
{
	if( mylog.is_info() )
		mylog.info() << peername << "(terminate)..." << endl;

	cancelled = true;

	{
		std::unique_lock<std::mutex> lk2(io_mutex);
		io.stop();
		cmdTimer.stop();
		asyncEvent.stop();
		conn.disconnect();
	}

	{
		std::unique_lock<std::mutex> lk(logbuf_mutex);

		while( !logbuf.empty() )
			logbuf.pop();
	}

	sock.reset(); // close..
	final();
}
// -------------------------------------------------------------------------
void LogSession::event( ev::async& watcher, int revents )
{
	if( EV_ERROR & revents )
	{
		if( mylog.is_crit() )
			mylog.crit() << peername << "(event): EVENT ERROR.." << endl;

		return;
	}

	//writeEvent(io);
	std::unique_lock<std::mutex> lk(io_mutex);
	io.set(ev::READ | ev::WRITE);
}
// ---------------------------------------------------------------------
void LogSession::callback( ev::io& watcher, int revents )
{
	if( EV_ERROR & revents )
	{
		if( mylog.is_crit() )
			mylog.crit() << peername << "(callback): EVENT ERROR.." << endl;

		return;
	}

	if (revents & EV_READ)
		readEvent(watcher);

	if (revents & EV_WRITE)
		writeEvent(watcher);

	if( cancelled )
	{
		if( mylog.is_info() )
			mylog.info() << peername << ": stop session... disconnect.." << endl;

		std::unique_lock<std::mutex> lk(io_mutex);
		io.stop();
		cmdTimer.stop();
		{
			std::unique_lock<std::mutex> lk(logbuf_mutex);
			asyncEvent.stop();
			conn.disconnect();
		}
		final();
	}
}
// -------------------------------------------------------------------------
void LogSession::writeEvent( ev::io& watcher )
{
	if( cancelled )
		return;

	std::unique_lock<std::mutex> lk(logbuf_mutex);

	if( logbuf.empty() )
		return;

	auto buffer = logbuf.front();

	if( !buffer )
		return;

	ssize_t ret = write(watcher.fd, buffer->dpos(), buffer->nbytes());

	if( ret < 0 )
	{
		if( mylog.is_warn() )
			mylog.warn() << peername << "(writeEvent): write to socket error(" << errno << "): " << strerror(errno) << endl;

		if( errno == EPIPE )
		{
			if( mylog.is_warn() )
				mylog.warn() << peername << "(writeEvent): write error.. terminate session.." << endl;

			cancelled = true;
		}

		return;
	}

	buffer->pos += ret;

	if( buffer->nbytes() == 0 )
	{
		logbuf.pop();
		delete buffer;
	}

	std::unique_lock<std::mutex> lk1(io_mutex);

	if( logbuf.empty() )
		io.set(ev::READ);
	else
		io.set(ev::READ | ev::WRITE);
}
// -------------------------------------------------------------------------
size_t LogSession::readData( unsigned char* buf, int len )
{
	ssize_t res = read( sock->getSocket(), buf, len );

	if( res > 0 )
		return res;

	if( res < 0 )
	{
		if( errno != EAGAIN && mylog.is_warn() )
			mylog.warn() << peername << "(readData): read from socket error(" << errno << "): " << strerror(errno) << endl;

		return 0;
	}

	mylog.info() << peername << "(readData): client disconnected.." << endl;
	cancelled = true;
	return 0;
}
// --------------------------------------------------------------------------------
void LogSession::readEvent( ev::io& watcher )
{
	if( cancelled )
		return;

	LogServerTypes::lsMessage msg;

	size_t ret = readData( (unsigned char*)(&msg), sizeof(msg) );

	if( cancelled )
		return;

	if( ret != sizeof(msg) || msg.magic != LogServerTypes::MAGICNUM )
	{
		if( mylog.is_warn() )
			mylog.warn() << peername << "(readEvent): BAD MESSAGE..." << endl;

		return;
	}

	if( mylog.is_info() )
		mylog.info() << peername << "(run): receive command: '" << msg.cmd << "'" << endl;

	string cmdLogName(msg.logname);

	cmdProcessing(cmdLogName, msg);

#if 0
	// Выводим итоговый получившийся список (с учётом выполненных команд)
	ostringstream s;

	if( msg.cmd == LogServerTypes::cmdFilterMode )
	{
		s << "List of managed logs(filter='" << cmdLogName << "'):" << endl;
		s << "=====================" << endl;
		LogAgregator::printLogList(s, loglist);
		s << "=====================" << endl << endl;
	}
	else
	{
		s << "List of managed logs:" << endl;
		s << "=====================" << endl;

		// выводим полный список
		if( alog )
		{
			auto lst = alog->getLogList();
			LogAgregator::printLogList(s, lst);
		}
		else
			s << log->getLogName() << " [" << Debug::str(log->level()) << " ]" << endl;

		s << "=====================" << endl << endl;
	}

	if( isPending(Socket::pendingOutput, cmdTimeout) )
	{
		*tcp() << s.str();
		tcp()->sync();
	}

#endif

	std::unique_lock<std::mutex> lk(io_mutex);
	cmdTimer.stop();
	io.set(ev::WRITE);
	asyncEvent.start();
}
// --------------------------------------------------------------------------------
void LogSession::cmdProcessing( const string& cmdLogName, const LogServerTypes::lsMessage& msg )
{
	std::list<LogAgregator::iLog> loglist;

	if( alog ) // если у нас "агрегатор", то работаем с его списком потоков
	{
		if( cmdLogName.empty() || cmdLogName == "ALL" )
			loglist = alog->getLogList();
		else
			loglist = alog->getLogList(cmdLogName);
	}
	else
	{
		if( cmdLogName.empty() || cmdLogName == "ALL" || log->getLogFile() == cmdLogName )
		{
			LogAgregator::iLog llog(log, log->getLogName());
			loglist.push_back(llog);
		}
	}

	if( msg.cmd == LogServerTypes::cmdFilterMode )
	{
		// отлючаем старый обработчик
		if( conn )
			conn.disconnect();
	}

	// обрабатываем команды только если нашли подходящие логи
	for( auto && l : loglist )
	{
		// Обработка команд..
		// \warning Работа с логом ведётся без mutex-а, хотя он разделяется отдельными потоками
		switch( msg.cmd )
		{
			case LogServerTypes::cmdSetLevel:
				l.log->level( (Debug::type)msg.data );
				break;

			case LogServerTypes::cmdAddLevel:
				l.log->addLevel( (Debug::type)msg.data );
				break;

			case LogServerTypes::cmdDelLevel:
				l.log->delLevel( (Debug::type)msg.data );
				break;

			case LogServerTypes::cmdRotate:
				l.log->onLogFile(true);
				break;

			case LogServerTypes::cmdList: // обработали выше (в начале)
				break;

			case LogServerTypes::cmdOffLogFile:
				l.log->offLogFile();
				break;

			case LogServerTypes::cmdOnLogFile:
				l.log->onLogFile();
				break;

			case LogServerTypes::cmdFilterMode:
				l.log->signal_stream_event().connect( sigc::mem_fun(this, &LogSession::logOnEvent) );
				break;

			default:
				mylog.warn() << peername << "(run): Unknown command '" << msg.cmd << "'" << endl;
				break;
		}
	} // end if for

	// если команда "вывести список"
	// выводим и завершаем работу
	if( msg.cmd == LogServerTypes::cmdList )
	{
		ostringstream s;
		s << "List of managed logs(filter='" << cmdLogName << "'):" << endl;
		s << "=====================" << endl;
		LogAgregator::printLogList(s, loglist);
		s << "=====================" << endl << endl;

		std::unique_lock<std::mutex> lk(logbuf_mutex);
		logbuf.push(new UTCPCore::Buffer(s.str()));
		std::unique_lock<std::mutex> lk1(io_mutex);
		io.set(ev::WRITE);
	}
}
// -------------------------------------------------------------------------
void LogSession::onCmdTimeout(ev::timer& watcher, int revents)
{
	io.set(ev::WRITE);
	asyncEvent.start();
}
// -------------------------------------------------------------------------
void LogSession::final()
{
	slFin(this);
}
// -------------------------------------------------------------------------
void LogSession::connectFinalSession( FinalSlot sl )
{
	slFin = sl;
}
// ---------------------------------------------------------------------
bool LogSession::isAcive()
{
	std::unique_lock<std::mutex> lk(io_mutex);
	return io.is_active();
}
// ---------------------------------------------------------------------
