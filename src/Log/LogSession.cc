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
#include <Poco/Net/NetException.h>
#include <Poco/Exception.h>
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

	if( checkConnectionTimer.is_active() )
		checkConnectionTimer.stop();

	if( asyncEvent.is_active() )
		asyncEvent.stop();
}
// -------------------------------------------------------------------------
LogSession::LogSession( const Poco::Net::StreamSocket& s, std::shared_ptr<DebugStream>& _log, timeout_t _cmdTimeout, timeout_t _checkConnectionTime ):
	cmdTimeout(_cmdTimeout),
	checkConnectionTime(_checkConnectionTime / 1000.),
	peername(""),
	caddr(""),
	log(_log)
{
	auto ag = dynamic_pointer_cast<LogAgregator>(log);

	if( ag )
		alog = ag;

	try
	{
		sock = make_shared<UTCPStream>(s);
		sock->setBlocking(false);
		Poco::Net::SocketAddress  iaddr = sock->peerAddress();

		if( iaddr.host().toString().empty() )
		{
			ostringstream err;
			err << "(ModbusTCPSession): unknonwn ip(0.0.0.0) client disconnected?!";

			if( mylog.is_crit() )
				mylog.crit() << err.str() << endl;

			sock->close();
			throw SystemError(err.str());
		}

		caddr = iaddr.host().toString();

		ostringstream s;
		s << caddr << ":" << iaddr.port();
		peername = s.str();
	}
	catch( const Poco::Net::NetException& ex )
	{
		ostringstream err;
		err << ex.what();
		mylog.crit() << "(LogSession): err: " << err.str() << endl;
		throw SystemError(err.str());
	}

	sock->setBlocking(false);

	io.set<LogSession, &LogSession::callback>(this);
	cmdTimer.set<LogSession, &LogSession::onCmdTimeout>(this);
	asyncEvent.set<LogSession, &LogSession::event>(this);
	checkConnectionTimer.set<LogSession, &LogSession::onCheckConnectionTimer>(this);

	if( log )
		conn = log->signal_stream_event().connect( sigc::mem_fun(this, &LogSession::logOnEvent) );
	else
		mylog.crit() << "LOG NULL!!" << endl;
}
// -------------------------------------------------------------------------
void LogSession::logOnEvent( const std::string& s )
{
	if( cancelled || s.empty() )
		return;

	{
		// чтобы поменьше удерживать mutex
		std::unique_lock<std::mutex> lk(logbuf_mutex);

		// собираем статистику..
		// --------------------------
		if( s.size() < minSizeMsg || minSizeMsg == 0 )
			minSizeMsg = s.size();

		if( s.size() > maxSizeMsg )
			maxSizeMsg = s.size();

		if( logbuf.size() > maxCount )
			maxCount = logbuf.size();

		// --------------------------

		// проверяем на переполнение..
		if( logbuf.size() >= maxRecordsNum )
		{
			numLostMsg++;

			if( !lostMsg )
			{
				ostringstream err;
				err <<  "The buffer is full. Message is lost...(size of buffer " << maxRecordsNum << ")" << endl;
				logbuf.emplace(new UTCPCore::Buffer(std::move(err.str())));
				lostMsg = true;
			}

			return;
		}

		lostMsg = false;
		logbuf.emplace(new UTCPCore::Buffer(s));
	}

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
	checkConnectionTimer.set(loop);

	io.set(loop);
	io.start(sock->getSocket(), ev::READ);
	cmdTimer.start( cmdTimeout / 1000. );
	checkConnectionTimer.start( checkConnectionTime );
}
// -------------------------------------------------------------------------
void LogSession::terminate()
{
	if( mylog.is_info() )
		mylog.info() << peername << "(terminate)..." << endl;

	cancelled = true;

	{
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

	sock->close();
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

	io.set(ev::WRITE);
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

	UTCPCore::Buffer* buffer = 0;

	{
		std::unique_lock<std::mutex> lk(logbuf_mutex);

		if( logbuf.empty() )
		{
			io.set(EV_NONE);
			checkConnectionTimer.start( checkConnectionTime ); // restart timer
			return;
		}

		buffer = logbuf.front();
	}

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
		{
			std::unique_lock<std::mutex> lk(logbuf_mutex);
			logbuf.pop();
		}

		delete buffer;
	}


	{
		std::unique_lock<std::mutex> lk(logbuf_mutex);

		if( logbuf.empty() )
		{
			io.set(EV_NONE);
			checkConnectionTimer.start( checkConnectionTime ); // restart timer
			return;
		}
	}

	if( checkConnectionTimer.is_active() )
		checkConnectionTimer.stop();

	io.set(ev::WRITE);
	//io.set(ev::READ | ev::WRITE);
}
// -------------------------------------------------------------------------
size_t LogSession::readData( unsigned char* buf, int len )
{
	try
	{
		ssize_t res = sock->receiveBytes(buf, len);

		if( res > 0 )
			return res;

		if( res < 0 )
		{
			if( errno != EAGAIN && mylog.is_warn() )
				mylog.warn() << peername << "(readData): read from socket error(" << errno << "): " << strerror(errno) << endl;

			return 0;
		}
	}
	catch( Poco::TimeoutException& ex )
	{
		return 0;
	}
	catch( Poco::Net::ConnectionResetException& ex )
	{

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

	if( ret == 0  || cancelled )
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

	cmdTimer.stop();
	asyncEvent.start();
	checkConnectionTimer.start( checkConnectionTime ); // restart timer
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
			loglist.emplace_back(log, log->getLogName());
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

			case LogServerTypes::cmdOffLogFile:
				l.log->offLogFile();
				break;

			case LogServerTypes::cmdOnLogFile:
				l.log->onLogFile();
				break;

			case LogServerTypes::cmdFilterMode:
				l.log->signal_stream_event().connect( sigc::mem_fun(this, &LogSession::logOnEvent) );
				break;

			case LogServerTypes::cmdList:
			case LogServerTypes::cmdSaveLogLevel:
			case LogServerTypes::cmdRestoreLogLevel:
			case LogServerTypes::cmdViewDefaultLogLevel:
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

		{
			std::unique_lock<std::mutex> lk(logbuf_mutex);
			logbuf.emplace(new UTCPCore::Buffer(s.str()));
		}

		io.set(ev::WRITE);
	}

	try
	{
		std::string ret( std::move(m_command_sig.emit(this, msg.cmd, cmdLogName)) );

		if( !ret.empty() )
		{
			{
				std::unique_lock<std::mutex> lk(logbuf_mutex);
				logbuf.emplace(new UTCPCore::Buffer( std::move(ret) ));
			}

			io.set(ev::WRITE);
		}
	}
	catch( std::exception& ex )
	{
		if( mylog.is_warn() )
			mylog.warn() << peername << "(cmdProcessing): " << ex.what() << endl;
	}
}
// -------------------------------------------------------------------------
void LogSession::onCmdTimeout( ev::timer& watcher, int revents )
{
	if( EV_ERROR & revents )
	{
		if( mylog.is_crit() )
			mylog.crit() << peername << "(onCmdTimeout): EVENT ERROR.." << endl;

		return;
	}

	io.set(ev::WRITE);
	asyncEvent.start();
}
// -------------------------------------------------------------------------
void LogSession::onCheckConnectionTimer( ev::timer& watcher, int revents )
{
	if( EV_ERROR & revents )
	{
		if( mylog.is_crit() )
			mylog.crit() << peername << "(onCheckConnectionTimer): EVENT ERROR.." << endl;

		return;
	}

	std::unique_lock<std::mutex> lk(logbuf_mutex);

	if( !logbuf.empty() )
	{
		checkConnectionTimer.start( checkConnectionTime ); // restart timer
		return;
	}

	// если клиент уже отвалился.. то при попытке write.. сессия будет закрыта.

	// длинное сообщение ("keep alive message") забивает логи, что потом неудобно смотреть, поэтому пишем "пустоту"
	logbuf.emplace(new UTCPCore::Buffer(""));
	io.set(ev::WRITE);
	checkConnectionTimer.start( checkConnectionTime ); // restart timer
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
LogSession::LogSessionCommand_Signal LogSession::signal_logsession_command()
{
	return m_command_sig;
}
// ---------------------------------------------------------------------
void LogSession::cancel()
{
	cancelled = true;
}
// ---------------------------------------------------------------------
void LogSession::setMaxBufSize( size_t num )
{
	std::unique_lock<std::mutex> lk(logbuf_mutex);
	maxRecordsNum = num;
}
// ---------------------------------------------------------------------
size_t LogSession::getMaxBufSize() const
{
	return maxRecordsNum;
}
// ---------------------------------------------------------------------
bool LogSession::isAcive() const
{
	return io.is_active();
}
// ---------------------------------------------------------------------
string LogSession::getShortInfo()
{
	size_t sz = 0;
	{
		std::unique_lock<std::mutex> lk(logbuf_mutex);
		sz = logbuf.size();
	}

	ostringstream inf;

	inf << "client: " << caddr << endl
		<< " buffer[" << maxRecordsNum << "]: size=" << sz
		<< " maxCount=" << maxCount
		<< " minSizeMsg=" << minSizeMsg
		<< " maxSizeMsg=" << maxSizeMsg
		<< " numLostMsg=" << numLostMsg
		<< endl;

	return std::move(inf.str());
}
// ---------------------------------------------------------------------
