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
#include "LogServer.h"
#include "UniSetTypes.h"
#include "Exceptions.h"
#include "LogSession.h"
#include "LogAgregator.h"
#include "Configuration.h"
// -------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
// -------------------------------------------------------------------------
CommonEventLoop LogServer::loop;
// -------------------------------------------------------------------------
LogServer::~LogServer()
{
	if( isrunning )
		loop.evstop(this);
}
// -------------------------------------------------------------------------
LogServer::LogServer( std::shared_ptr<LogAgregator> log ):
	LogServer(static_pointer_cast<DebugStream>(log))
{

}
// -------------------------------------------------------------------------
LogServer::LogServer( std::shared_ptr<DebugStream> log ):
	timeout(TIMEOUT_INF),
	cmdTimeout(2000),
	sessLogLevel(Debug::NONE),
	sock(0),
	elog(log)
{
}
// -------------------------------------------------------------------------
LogServer::LogServer():
	timeout(TIMEOUT_INF),
	cmdTimeout(2000),
	sessLogLevel(Debug::NONE),
	sock(0),
	elog(nullptr)
{
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
	// который приведёт к вызову sessionFinished()..в котором список будет меняться..
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

	sock.reset();

	if( mylog.is_info() )
		mylog.info() << myname << "(LogServer): finished." << endl;
}
// -------------------------------------------------------------------------
void LogServer::run( const std::string& _addr, ost::tpport_t _port, bool thread )
{
	addr = _addr;
	port = _port;

	{
		ostringstream s;
		s << _addr << ":" << _port;
		myname = s.str();
	}

	loop.evrun(this, thread);
}
// -------------------------------------------------------------------------
void LogServer::terminate()
{
	loop.evstop(this);
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

		throw SystemError( err.str() );
	}

	try
	{
		ost::InetAddress iaddr(addr.c_str());
		sock = make_shared<UTCPSocket>(iaddr, port);
	}
	catch( ost::Socket* socket )
	{
		ost::tpport_t port;
		int errnum = socket->getErrorNumber();
		ost::InetAddress saddr = (ost::InetAddress)socket->getPeer(&port);
		ostringstream err;

		err << myname << "(evprepare): socket error(" << errnum << "): ";

		if( errnum == ost::Socket::errBindingFailed )
			err << "bind failed; port busy" << endl;
		else
			err << "client socket failed" << endl;

		if( mylog.is_crit() )
			mylog.crit() << err.str() << endl;

		throw SystemError( err.str() );
	}

	sock->setCompletion(false);

	io.set<LogServer, &LogServer::ioAccept>(this);
	io.set( eloop );
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

		sock->reject();
		return;
	}

	{
		uniset_rwmutex_wrlock l(mutSList);

		if( scount >= sessMaxCount )
		{
			if( mylog.is_crit() )
				mylog.crit() << myname << "(LogServer::ioAccept): session limit(" << sessMaxCount << ")" << endl;

			sock->reject();
			return;
		}
	}

	try
	{
		auto s = make_shared<LogSession>( watcher.fd, elog, cmdTimeout );
		s->setSessionLogLevel(sessLogLevel);
		s->connectFinalSession( sigc::mem_fun(this, &LogServer::sessionFinished) );
		s->signal_logsession_command().connect( sigc::mem_fun(this, &LogServer::onCommand) );
		{
			uniset_rwmutex_wrlock l(mutSList);
			scount++;

			// на первой сессии запоминаем состояние логов
			if( scount == 1 )
				saveDefaultLogLevels("ALL");

			slist.push_back(s);
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
			scount--;
			break;
		}
	}

	if( slist.empty() )
	{
		scount = 0;
		// восстанавливаем уровни логов по умолчанию
		restoreDefaultLogLevels("ALL");
	}
}
// -------------------------------------------------------------------------
void LogServer::init( const std::string& prefix, xmlNode* cnode )
{
	auto conf = uniset_conf();

	// можем на cnode==0 не проверять, т.е. UniXML::iterator корректно отрабатывает эту ситуацию
	UniXML::iterator it(cnode);

	timeout_t cmdTimeout = conf->getArgPInt("--" + prefix + "-cmd-timeout", it.getProp("cmdTimeout"), 2000);
	setCmdTimeout(cmdTimeout);
}
// -----------------------------------------------------------------------------
std::string LogServer::help_print( const std::string& prefix )
{
	ostringstream h;
	h << "--" << prefix << "-cmd-timeout msec      - Timeout for wait command. Default: 2000 msec." << endl;
	return std::move( h.str() );
}
// -----------------------------------------------------------------------------
string LogServer::getShortInfo()
{
	ostringstream inf;

	inf << "LogServer: " << myname << endl;
	{
		uniset_rwmutex_wrlock l(mutSList);

		for( const auto& s : slist )
			inf << " " << s->getShortInfo() << endl;
	}

	return std::move(inf.str());
}
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

		for( auto && l : lst )
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

		for( auto && l : lst )
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

		return std::move(s.str());
	}

	return "";
}
// -----------------------------------------------------------------------------
