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
	if( isRunning() )
	{
		cancelled = true;
		ost::Thread::join();
		disconnect();
	}
}
// -------------------------------------------------------------------------
LogSession::LogSession( ost::TCPSocket& server ):
	TCPSession(server)
{

}
// -------------------------------------------------------------------------
LogSession::LogSession( ost::TCPSocket& server, std::shared_ptr<DebugStream>& _log, timeout_t _sessTimeout, timeout_t _cmdTimeout, timeout_t _outTimeout, timeout_t _delay ):
	TCPSession(server),
	sessTimeout(_sessTimeout),
	cmdTimeout(_cmdTimeout),
	outTimeout(_outTimeout),
	delayTime(_delay),
	peername(""),
	caddr(""),
	log(_log),
	cancelled(false)
{
	log_notify = ATOMIC_VAR_INIT(0);

	//slog.addLevel(Debug::ANY);
	if( log )
		conn = log->signal_stream_event().connect( sigc::mem_fun(this, &LogSession::logOnEvent) );
	else
		slog.crit() << "LOG NULL!!" << endl;

	auto ag = dynamic_pointer_cast<LogAgregator>(log);

	if( ag )
		alog = ag;
}
// -------------------------------------------------------------------------
void LogSession::logOnEvent( const std::string& s )
{
	{
		std::unique_lock<std::mutex> lk(log_mutex);
		lbuf.push_back(s);
		log_notify = true;
	}

	log_event.notify_one();
}
// -------------------------------------------------------------------------
void LogSession::run()
{
	if( cancelled )
		return;

	{
		ost::tpport_t p;
		ost::InetAddress iaddr = getIPV4Peer(&p);

		// resolve..
		caddr = string( iaddr.getHostname() );

		ostringstream s;
		s << iaddr << ":" << p;
		peername = s.str();
	}

	if( slog.debugging(Debug::INFO) )
		slog[Debug::INFO] << peername << "(run): run thread of sessions.." << endl;

	if( !log )
		slog.crit() << peername << "(run): LOG NULL!!" << endl;


	setKeepAlive(true);

	// Команды могут посылаться только в начале сессии..
	if( isPending(Socket::pendingInput, cmdTimeout) )
	{
		LogServerTypes::lsMessage msg;

		// проверяем канал..(если данных нет, значит "клиент отвалился"...
		if( peek( (void*)(&msg), sizeof(msg)) > 0 )
		{
			ssize_t ret = readData( &msg, sizeof(msg) );

			if( ret != sizeof(msg) || msg.magic != LogServerTypes::MAGICNUM )
				slog.warn() << peername << "(run): BAD MESSAGE..." << endl;
			else
			{
				slog.info() << peername << "(run): receive command: '" << msg.cmd << "'" << endl;
				string cmdLogName(msg.logname);

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

				// если команда "вывести список"
				// выводим и завершаем работу
				if( msg.cmd == LogServerTypes::cmdList )
				{
					ostringstream s;
					s << "List of managed logs(filter='" << cmdLogName << "'):" << endl;
					s << "=====================" << endl;
					LogAgregator::printLogList(s,loglist);
					s << "=====================" << endl << endl;

					if( isPending(Socket::pendingOutput, cmdTimeout) )
					{
						*tcp() << s.str();
						tcp()->sync();
					}

					// вывели список и завершили работу..
					cancelled = true;
					disconnect();
					return;
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
							slog.warn() << peername << "(run): Unknown command '" << msg.cmd << "'" << endl;
							break;
					}
				}

				// Выводим итоговый получившийся список (с учётом выполненных команд)
				ostringstream s;
				if( msg.cmd == LogServerTypes::cmdFilterMode )
				{
					s << "List of managed logs(filter='" << cmdLogName << "'):" << endl;
					s << "=====================" << endl;
					LogAgregator::printLogList(s,loglist);
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
						LogAgregator::printLogList(s,lst);
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
			}
		}
	}

	cancelled = false;

	while( !cancelled && isConnected() ) // !ptSessionTimeout.checkTime()
	{
		// проверка только ради проверки "целостности" соединения
		if( isPending(Socket::pendingInput, 10) )
		{
			char buf[10];

			// проверяем канал..(если данных нет, значит "клиент отвалился"...
			if( peek(buf, sizeof(buf)) <= 0 )
				break;
		}

		if( isPending(Socket::pendingOutput, outTimeout) )
		{
			//slog.info() << peername << "(run): send.." << endl;
			//      ptSessionTimeout.reset();

			// чтобы не застревать на посылке в сеть..
			// делаем через промежуточный буффер (stringstream)
			sbuf.str("");
			bool send = false;
			{
				std::unique_lock<std::mutex> lk(log_mutex);

				// uniset_rwmutex_wrlock l(mLBuf);
				if( !lbuf.empty() )
				{
					slog.info() << peername << "(run): send messages.." << endl;

					while( !lbuf.empty() )
					{
						sbuf << lbuf.front();
						lbuf.pop_front();
					}

					send = true;
				}
			}

			if( send )
			{
				*tcp() << sbuf.str();
				tcp()->sync();
			}

			{
				std::unique_lock<std::mutex> lk(log_mutex);
				log_event.wait_for(lk, std::chrono::milliseconds(outTimeout), [&]()
				{
					return (log_notify == true || cancelled);
				} );
			}

			if( cancelled )
				break;
		}
	}

	if( slog.debugging(Debug::INFO) )
		slog[Debug::INFO] << peername << "(run): stop thread of sessions..disconnect.." << endl;

	disconnect();

	if( slog.debugging(Debug::INFO) )
		slog[Debug::INFO] << peername << "(run): thread stopping..." << endl;
}
// -------------------------------------------------------------------------
void LogSession::final()
{
	tcp()->sync();

	try
	{
		auto s = shared_from_this();

		if( s )
			slFin(s);
	}
	catch( const std::bad_weak_ptr )
	{

	}
}
// -------------------------------------------------------------------------
void LogSession::connectFinalSession( FinalSlot sl )
{
	slFin = sl;
}
// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
NullLogSession::NullLogSession( const std::string& _msg ):
	msg(_msg),
	cancelled(false)
{
}
// ---------------------------------------------------------------------
NullLogSession::~NullLogSession()
{
	cancelled = true;

	if( isRunning() )
		exit(); // terminate();
}
// ---------------------------------------------------------------------
void NullLogSession::add( ost::TCPSocket& sock )
{
	uniset_rwmutex_wrlock l(smutex);
	auto s = make_shared<ost::TCPStream>();
	s->connect(sock);
	slist.push_back(s);
}
// ---------------------------------------------------------------------
void NullLogSession::setMessage( const std::string& _msg )
{
	uniset_rwmutex_wrlock l(smutex);
	msg = _msg;
}
// ---------------------------------------------------------------------
void NullLogSession::run()
{
	while( !cancelled )
	{
		{
			// lock slist
			uniset_rwmutex_wrlock l(smutex);

			for( auto i = slist.begin(); !cancelled && i != slist.end(); ++i )
			{
				auto s(*i);

				if( s->isPending(ost::Socket::pendingInput, 10) )
				{
					char buf[10];

					// проверяем канал..(если данных нет, значит "клиент отвалился"...
					if( s->peek(buf, sizeof(buf)) <= 0 )
					{
						i = slist.erase(i);
						continue;
					}
				}

				if( s->isPending(ost::Socket::pendingOutput) )
				{
					(*s.get()) << msg << endl;
					s->sync();
					s->disconnect(); // послали сообщение и закрываем соединение..
				}
			}

			slist.clear();
		} // unlock slist

		if( cancelled )
			break;

		msleep(5000); // делаем паузу, чтобы освободить на время "список"..
	}

	{
		uniset_rwmutex_wrlock l(smutex);

		for( auto i = slist.begin(); i != slist.end(); ++i )
		{
			auto s(*i);

			if( s )
				s->disconnect();
		}
	}
}
// ---------------------------------------------------------------------
void NullLogSession::final()
{
#if 0
	{
		uniset_rwmutex_wrlock l(smutex);

		for( auto i = slist.begin(); i != slist.end(); ++i )
		{
			auto s(*i);

			if( s )
				s->disconnect();
		}
	}
#endif
}
// ---------------------------------------------------------------------
