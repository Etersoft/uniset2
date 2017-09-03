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
// --------------------------------------------------------------------------
/*! \file
 *  \author Pavel Vainerman
*/
// --------------------------------------------------------------------------
#include <sstream>
#include <iomanip>

#include "unisetstd.h"
#include <Poco/Net/NetException.h>
#include "LogDB.h"
#include "Configuration.h"
#include "Debug.h"
#include "UniXML.h"
#include "LogDBSugar.h"
// --------------------------------------------------------------------------
using namespace uniset;
using namespace std;
// --------------------------------------------------------------------------
LogDB::LogDB( const string& name , const string& prefix ):
	myname(name)
{
	dblog = make_shared<DebugStream>();

	auto conf = uniset_conf();
	auto xml = conf->getConfXML();

	conf->initLogStream(dblog, prefix + "-log" );

	xmlNode* cnode = conf->findNode(xml->getFirstNode(), "LogDB", name);

	if( !cnode )
	{
		ostringstream err;
		err << name << "(init): Not found confnode <LogDB name='" << name << "'...>";
		dbcrit << err.str() << endl;
		throw uniset::SystemError(err.str());
	}

	UniXML::iterator it(cnode);

	qbufSize = conf->getArgPInt("--" + prefix + "-buffer-size", it.getProp("bufferSize"), qbufSize);

	tmConnection_sec = tmConnection_msec / 1000.;

	connectionTimer.set<LogDB, &LogDB::onTimer>(this);
	checkBufferTimer.set<LogDB, &LogDB::onCheckBuffer>(this);

	UniXML::iterator sit(cnode);
	if( !sit.goChildren() )
	{
		ostringstream err;
		err << name << "(init): Not found confnode list of log servers for <LogDB name='" << name << "'...>";
		dbcrit << err.str() << endl;
		throw uniset::SystemError(err.str());
	}

	for( ;sit.getCurrent(); sit++ )
	{
		auto l = make_shared<Log>();

		l->name = sit.getProp("name");
		l->ip = sit.getProp("ip");
		l->port = sit.getIntProp("port");
		l->cmd = sit.getProp("cmd");

		if( l->name.empty()  )
		{
			ostringstream err;
			err << name << "(init): Unknown name for logserver..";
			dbcrit << err.str() << endl;
			throw uniset::SystemError(err.str());
		}

		if( l->ip.empty()  )
		{
			ostringstream err;
			err << name << "(init): Unknown 'ip' for '" << l->name << "'..";
			dbcrit << err.str() << endl;
			throw uniset::SystemError(err.str());
		}

		if( l->port == 0 )
		{
			ostringstream err;
			err << name << "(init): Unknown 'port' for '" << l->name << "'..";
			dbcrit << err.str() << endl;
			throw uniset::SystemError(err.str());
		}

//		if( l->cmd.empty() )
//		{
//			ostringstream err;
//			err << name << "(init): Unknown 'cmd' for '" << l->name << "'..";
//			dbcrit << err.str() << endl;
//			throw uniset::SystemError(err.str());
//		}

//		l->tcp = make_shared<UTCPStream>();
		l->dblog = dblog;
		l->signal_on_read().connect(sigc::mem_fun(this, &LogDB::addLog));

		logservers.push_back(l);
	}

	if( logservers.empty() )
	{
		ostringstream err;
		err << name << "(init): empty list of log servers for <LogDB name='" << name << "'...>";
		dbcrit << err.str() << endl;
		throw uniset::SystemError(err.str());
	}


	std::string dbfile = conf->getArgParam("--" + prefix + "-dbfile", it.getProp("dbfile"));
	if( dbfile.empty() )
	{
		ostringstream err;
		err << name << "(init): dbfile (sqlite) not defined. Use: <LogDB name='" << name << "' dbfile='..' ...>";
		dbcrit << err.str() << endl;
		throw uniset::SystemError(err.str());
	}

	db = unisetstd::make_unique<SQLiteInterface>();
	if( !db->connect(dbfile, false) )
	{
		ostringstream err;
		err << myname
			   << "(init): DB connection error: "
			   << db->error();
		dbcrit << err.str() << endl;
		throw uniset::SystemError(err.str());
	}
}
//--------------------------------------------------------------------------------------------
LogDB::~LogDB()
{
	if( db )
		db->close();
}
//--------------------------------------------------------------------------------------------
void LogDB::flushBuffer()
{
	// Сперва пробуем очистить всё что накопилось в очереди до этого...
	while( !qbuf.empty() )
	{
		if( !db->insert(qbuf.front()) )
		{
			dbcrit << myname << "(flushBuffer): error: " << db->error() <<
				   " lost query: " << qbuf.front() << endl;
		}

		qbuf.pop();
	}
}
//--------------------------------------------------------------------------------------------
void LogDB::addLog( LogDB::Log* log, const string& txt )
{
	auto tm = uniset::now_to_timespec();

	ostringstream q;

	q << "INSERT INTO log(tms,usec,name,text) VALUES('"
	  << tm.tv_sec << "','"   //  timestamp
	  << tm.tv_nsec << "','"  //  usec
	  << log->name << "','"
	  << txt << "');";

	qbuf.emplace(q.str());
}
//--------------------------------------------------------------------------------------------
std::shared_ptr<LogDB> LogDB::init_logdb( int argc, const char* const* argv, const std::string& prefix )
{
	auto conf = uniset_conf();

	string name = conf->getArgParam("--" + prefix + "-name", "");

	if( name.empty() )
	{
		cerr << "(LogDB): Unknown name. Use --" << prefix << "-name" << endl;
		return nullptr;
	}

	return make_shared<LogDB>(name, prefix);
}
// -----------------------------------------------------------------------------
void LogDB::help_print()
{
	cout << "Default: prefix='logdb'" << endl;
	cout << "--prefix-name name        - Имя. Для поиска настроечной секции в configure.xml" << endl;
	cout << "--prefix-buffer-size sz   - Размер буфера (до скидывания в БД)." << endl;
}
// -----------------------------------------------------------------------------
void LogDB::run( bool async )
{
	if( async )
		async_evrun();
	else
		evrun();
}
// -----------------------------------------------------------------------------
void LogDB::evfinish()
{
	connectionTimer.stop();
	checkBufferTimer.stop();
}
// -----------------------------------------------------------------------------
void LogDB::evprepare()
{
	connectionTimer.set(loop);
	checkBufferTimer.set(loop);

	if( tmConnection_msec != UniSetTimer::WaitUpTime )
		connectionTimer.start(0, tmConnection_sec);

	checkBufferTimer.start(0, tmCheckBuffer_sec);
}
// -----------------------------------------------------------------------------
void LogDB::onTimer( ev::timer& t, int revents )
{
	if (EV_ERROR & revents)
	{
		dbcrit << myname << "(LogDB::onTimer): invalid event" << endl;
		return;
	}

	// проверяем соединения..
	for( const auto& s: logservers )
	{
		if( !s->isConnected() )
		{
			if( s->connect() )
				s->ioprepare(loop);
		}
	}
}
// -----------------------------------------------------------------------------
void LogDB::onCheckBuffer(ev::timer& t, int revents)
{
	if (EV_ERROR & revents)
	{
		dbcrit << myname << "(LogDB::onTimer): invalid event" << endl;
		return;
	}

	if( qbuf.size() >= qbufSize )
		flushBuffer();
}
// -----------------------------------------------------------------------------
bool LogDB::Log::isConnected() const
{
	return tcp && tcp->isConnected();
}
// -----------------------------------------------------------------------------
bool LogDB::Log::connect() noexcept
{
	if( tcp && tcp->isConnected() )
		return true;

//	dbinfo << name << "(connect): connect " << ip << ":" << port << "..." << endl;

	try
	{
		tcp = make_shared<UTCPStream>();
		tcp->create(ip, port);
//		tcp->setReceiveTimeout( UniSetTimer::millisecToPoco(inTimeout) );
//		tcp->setSendTimeout( UniSetTimer::millisecToPoco(outTimeout) );
		tcp->setKeepAlive(true);
		tcp->setBlocking(false);
		dbinfo << name << "(connect): connect OK to " << ip << ":" << port << endl;
		return true;
	}
	catch( const Poco::TimeoutException& e )
	{
		dbwarn << name << "(connect): connection " << ip << ":" << port << " timeout.." << endl;
	}
	catch( const Poco::Net::NetException& e )
	{
		dbwarn << name << "(connect): connection " << ip << ":" << port << " error: " << e.what() << endl;
	}
	catch( const std::exception& e )
	{
		dbwarn << name << "(connect): connection " << ip << ":" << port << " error: " << e.what() << endl;
	}
	catch( ... )
	{
		std::exception_ptr p = std::current_exception();
		dbwarn << name << "(connect): connection " << ip << ":" << port << " error: "
			 << (p ? p.__cxa_exception_type()->name() : "null") << endl;
	}

	tcp->disconnect();
	tcp = nullptr;

	return false;
}
// -----------------------------------------------------------------------------
void LogDB::Log::ioprepare( ev::dynamic_loop& loop )
{
	if( !tcp || !tcp->isConnected() )
		return;

	io.set(loop);
	io.set<LogDB::Log, &LogDB::Log::event>(this);
	io.start(tcp->getSocket(), ev::READ);
	text.reserve(reservsize);
}
// -----------------------------------------------------------------------------
void LogDB::Log::event( ev::io& watcher, int revents )
{
	if( EV_ERROR & revents )
	{
		dbcrit << name << "(event): invalid event" << endl;
		return;
	}

	if( revents & EV_READ )
		read();

	if( revents & EV_WRITE )
	{
		dbinfo << name << "(event): ..write event.." << endl;
	}
}
// -----------------------------------------------------------------------------
LogDB::Log::ReadSignal LogDB::Log::signal_on_read()
{
	return sigRead;
}
// -----------------------------------------------------------------------------
void LogDB::Log::read()
{
	int n = tcp->available();

	n = std::min(n,bufsize);

	if( n > 0 )
	{
		tcp->receiveBytes(buf, n);

		// нарезаем на строки
		for( size_t i=0; i<n; i++ )
		{
			if( buf[i] != '\n' )
				text += buf[i];
			else
			{
				sigRead.emit(this,text);
				text = "";
				if( text.capacity() < reservsize )
					text.reserve(reservsize);
			}
		}
	}
	else if( n == 0 )
	{
		dbinfo << name << ": " << ip << ":" << port << " connection is closed.." << endl;
		tcp->disconnect();
		if( !text.empty() )
		{
			sigRead.emit(this,text);
			text = "";
			if( text.capacity() < reservsize )
				text.reserve(reservsize);
		}
	}
}
// -----------------------------------------------------------------------------
void LogDB::Log::write()
{

}
// -----------------------------------------------------------------------------
