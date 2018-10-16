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
namespace uniset
{
	// -------------------------------------------------------------------------
	using namespace std;
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
				err << "(LogSession): unknonwn ip(0.0.0.0) client disconnected?!";

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
			mylog.crit() << "(LogSession): LOG NULL!!" << endl;
	}
	// -------------------------------------------------------------------------
	void LogSession::logOnEvent( const std::string& s ) noexcept
	{
		if( cancelled || s.empty() )
			return;

		try
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

				if( numLostMsg > maxRecordsNum )
				{
					// видимо клиент отвалился или совсем не успевает читать
					// разрываем сессию..
					if( mylog.is_info() )
						mylog.info() << peername << "(LogSession::onEvent): too many lost messages. Close session.." << endl;

					cancelled = true;
				}

				if( !lostMsg )
				{
					ostringstream err;
					err <<  "(LogSession): The buffer is full. Message is lost...(size of buffer " << maxRecordsNum << ")" << endl;
					logbuf.emplace(new UTCPCore::Buffer(std::move(err.str())));
					lostMsg = true;
				}

				return;
			}

			lostMsg = false;
			logbuf.emplace(new UTCPCore::Buffer(s));
		}
		catch(...) {}

		if( asyncEvent.is_active() )
			asyncEvent.send();
	}
	// -------------------------------------------------------------------------
	void LogSession::run( const ev::loop_ref& loop ) noexcept
	{
		setSessionLogLevel(Debug::ANY);

		if( mylog.is_info() )
			mylog.info() << peername << "(LogSession::run): run session.." << endl;

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
			mylog.info() << peername << "(LogSession::terminate)..." << endl;

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

		sock->disconnect();
		sock->close();
		final();
	}
	// -------------------------------------------------------------------------
	void LogSession::event( ev::async& watcher, int revents ) noexcept
	{
		if( EV_ERROR & revents )
		{
			if( mylog.is_crit() )
				mylog.crit() << peername << "(LogSession::event): EVENT ERROR.." << endl;

			return;
		}

		io.set(ev::WRITE);
	}
	// ---------------------------------------------------------------------
	void LogSession::callback( ev::io& watcher, int revents ) noexcept
	{
		if( EV_ERROR & revents )
		{
			if( mylog.is_crit() )
				mylog.crit() << peername << "(LogSession::callback): EVENT ERROR.." << endl;

			return;
		}

		if (revents & EV_READ)
		{
			try
			{
				readEvent(watcher);
			}
			catch(...) {}
		}

		if (revents & EV_WRITE)
		{
			try
			{
				writeEvent(watcher);
			}
			catch(...) {}
		}

		if( cancelled.load() )
		{
			if( mylog.is_info() )
				mylog.info() << peername << "(LogSession): stop session... disconnect.." << endl;

			io.stop();
			cmdTimer.stop();

			try
			{
				std::unique_lock<std::mutex> lk(logbuf_mutex);
				asyncEvent.stop();
				conn.disconnect();
			}
			catch(...) {}

			final();
		}
	}
	// -------------------------------------------------------------------------
	void LogSession::writeEvent( ev::io& watcher )
	{
		if( cancelled.load() )
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

		ssize_t ret = ::write(watcher.fd, buffer->dpos(), buffer->nbytes());

		if( ret < 0 )
		{
			// копируем, а потом проверяем
			// хоть POSIX говорит о том, что errno thread-local
			// но почему-то словил, что errno (по крайней мере для EPIPE "broken pipe")
			// в лог выводилось, а в if( ... ) уже не ловилось
			// возможно связано с тем, что ввод/вывод "прерываемая" операция при многопоточности
			int errnum = errno;

			// можно было бы конечно убрать вывод лога в else, после проверки в if
			if( mylog.is_warn() )
				mylog.warn() << peername << "(LogSession::writeEvent): write to socket error(" << errnum << "): " << strerror(errnum) << endl;

			if( errnum == EPIPE || errnum == EBADF )
			{
				if( mylog.is_warn() )
					mylog.warn() << peername << "(LogSession::writeEvent): write error.. terminate session.." << endl;

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
					mylog.warn() << peername << "(LogSession::readData): read from socket error(" << errno << "): " << strerror(errno) << endl;

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
		catch( std::exception& ex )
		{

		}

		mylog.info() << peername << "(LogSession::readData): client disconnected.." << endl;
		cancelled = true;
		return 0;
	}
	// --------------------------------------------------------------------------------
	void LogSession::readEvent( ev::io& watcher ) noexcept
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
			{
				ostringstream err;
				err << peername << "(LogSession::readEvent): MESSAGE ERROR: ";

				if( msg.magic != LogServerTypes::MAGICNUM )
					err << "BAD MAGICNUM";

				if( ret != sizeof(msg) )
					err << "BAD soze of message (" << ret << ")";

				mylog.warn() << err.str() << endl;
			}

			return;
		}

		if( mylog.is_info() )
			mylog.info() << peername << "(LogSession::readEvent): receive command: '" << msg.cmd << "'" << endl;

		string cmdLogName(msg.logname);

		try
		{
			cmdProcessing(cmdLogName, msg);
		}
		catch( std::exception& ex )
		{
			if( mylog.is_warn() )
				mylog.warn() << peername << "(LogSession::readEvent): " << ex.what() << endl;
		}
		catch(...) {}

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
			std::string ret( m_command_sig.emit(this, msg.cmd, cmdLogName) );

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
				mylog.warn() << peername << "(LogSession::cmdProcessing): " << ex.what() << endl;
		}
	}
	// -------------------------------------------------------------------------
	void LogSession::onCmdTimeout( ev::timer& t, int revents ) noexcept
	{
		if( EV_ERROR & revents )
		{
			if( mylog.is_crit() )
				mylog.crit() << peername << "(LogSession::onCmdTimeout): EVENT ERROR.." << endl;

			return;
		}

		t.stop();
		io.set(ev::WRITE);
		asyncEvent.start();
	}
	// -------------------------------------------------------------------------
	void LogSession::onCheckConnectionTimer( ev::timer& t, int revents ) noexcept
	{
		if( EV_ERROR & revents )
		{
			if( mylog.is_crit() )
				mylog.crit() << peername << "(LogSession::onCheckConnectionTimer): EVENT ERROR.." << endl;

			return;
		}

		std::unique_lock<std::mutex> lk(logbuf_mutex);

		if( !logbuf.empty() )
			return;

		// если клиент уже отвалился.. то при попытке write.. сессия будет закрыта.

		// длинное сообщение ("keep alive message") забивает логи, что потом неудобно смотреть
		// поэтому используем "пробел и возврат на один символ"
		try
		{
			//
			logbuf.emplace(new UTCPCore::Buffer(" \b"));
		}
		catch(...) {}

		io.set(ev::WRITE);
	}
	// -------------------------------------------------------------------------
	void LogSession::final() noexcept
	{
		try
		{
			slFin(this);
		}
		catch(...) {}
	}
	// -------------------------------------------------------------------------
	void LogSession::connectFinalSession( FinalSlot sl ) noexcept
	{
		slFin = sl;
	}
	// ---------------------------------------------------------------------
	LogSession::LogSessionCommand_Signal LogSession::signal_logsession_command()
	{
		return m_command_sig;
	}
	// ---------------------------------------------------------------------
	void LogSession::cancel() noexcept
	{
		cancelled = true;
	}
	// ---------------------------------------------------------------------
	string LogSession::getClientAddress() const noexcept
	{
		return caddr;
	}
	// ---------------------------------------------------------------------
	void LogSession::setSessionLogLevel(Debug::type t) noexcept
	{
		mylog.level(t);
	}
	// ---------------------------------------------------------------------
	void LogSession::addSessionLogLevel(Debug::type t) noexcept
	{
		mylog.addLevel(t);
	}
	// ---------------------------------------------------------------------
	void LogSession::delSessionLogLevel(Debug::type t) noexcept
	{
		mylog.delLevel(t);
	}
	// ---------------------------------------------------------------------
	void LogSession::setMaxBufSize( size_t num )
	{
		std::unique_lock<std::mutex> lk(logbuf_mutex);
		maxRecordsNum = num;
	}
	// ---------------------------------------------------------------------
	size_t LogSession::getMaxBufSize() const noexcept
	{
		return maxRecordsNum;
	}
	// ---------------------------------------------------------------------
	bool LogSession::isAcive() const noexcept
	{
		return io.is_active();
	}
	// ---------------------------------------------------------------------
	string LogSession::getShortInfo() noexcept
	{
		size_t sz = 0;
		{
			std::unique_lock<std::mutex> lk(logbuf_mutex);
			sz = logbuf.size();
		}

		ostringstream inf;

		inf << "client: " << caddr << " :"
			<< " buffer[" << maxRecordsNum << "]: size=" << sz
			<< " maxCount=" << maxCount
			<< " minSizeMsg=" << minSizeMsg
			<< " maxSizeMsg=" << maxSizeMsg
			<< " numLostMsg=" << numLostMsg
			<< endl;

		return inf.str();
	}
	// ---------------------------------------------------------------------
#ifndef DISABLE_REST_API
	Poco::JSON::Object::Ptr LogSession::httpGetShortInfo()
	{
		Poco::JSON::Object::Ptr jret = new Poco::JSON::Object();

		size_t sz = 0;
		{
			std::unique_lock<std::mutex> lk(logbuf_mutex);
			sz = logbuf.size();
		}

		Poco::JSON::Object::Ptr jdata = new Poco::JSON::Object();
		jret->set(caddr, jdata);

		jdata->set("client", caddr);
		jdata->set("maxbufsize", maxRecordsNum);
		jdata->set("bufsize", sz);
		jdata->set("maxCount", maxCount);
		jdata->set("minSizeMsg", minSizeMsg);
		jdata->set("maxSizeMsg", maxSizeMsg);
		jdata->set("numLostMsg", numLostMsg);

		return jret;
	}
#endif // #ifndef DISABLE_REST_API
	// ---------------------------------------------------------------------
	string LogSession::name() const noexcept
	{
		return caddr;
	}
	// ---------------------------------------------------------------------
} // end of namespace uniset
