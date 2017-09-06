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
#include <Poco/Net/WebSocket.h>
#include "ujson.h"
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

	conf->initLogStream(dblog, prefix + "log" );

	xmlNode* cnode = conf->findNode(xml->getFirstNode(), "LogDB", name);

	if( !cnode )
	{
		ostringstream err;
		err << name << "(init): Not found confnode <LogDB name='" << name << "'...>";
		dbcrit << err.str() << endl;
		throw uniset::SystemError(err.str());
	}

	UniXML::iterator it(cnode);

	qbufSize = conf->getArgPInt("--" + prefix + "buffer-size", it.getProp("bufferSize"), qbufSize);

	tmConnection_sec = tmConnection_msec / 1000.;

	connectionTimer.set<LogDB, &LogDB::onTimer>(this);
	checkBufferTimer.set<LogDB, &LogDB::onCheckBuffer>(this);
	pingWebSockets.set<LogDB, &LogDB::onPingWebSockets>(this);

	UniXML::iterator sit(cnode);

	if( !sit.goChildren() )
	{
		ostringstream err;
		err << name << "(init): Not found confnode list of log servers for <LogDB name='" << name << "'...>";
		dbcrit << err.str() << endl;
		throw uniset::SystemError(err.str());
	}

	for( ; sit.getCurrent(); sit++ )
	{
		auto l = make_shared<Log>();

		l->name = sit.getProp("name");
		l->ip = sit.getProp("ip");
		l->port = sit.getIntProp("port");
		l->cmd = sit.getProp("cmd");
		l->description = sit.getProp("description");

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


	std::string dbfile = conf->getArgParam("--" + prefix + "dbfile", it.getProp("dbfile"));

	if( dbfile.empty() )
	{
		ostringstream err;
		err << name << "(init): dbfile (sqlite) not defined. Use: <LogDB name='" << name << "' dbfile='..' ...>";
		dbcrit << err.str() << endl;
		throw uniset::SystemError(err.str());
	}

	db = unisetstd::make_unique<SQLiteInterface>();

	if( !db->connect(dbfile, false, SQLITE_OPEN_FULLMUTEX) )
	{
		ostringstream err;
		err << myname
			<< "(init): DB connection error: "
			<< db->error();
		dbcrit << err.str() << endl;
		throw uniset::SystemError(err.str());
	}


#ifndef DISABLE_REST_API
	httpHost = conf->getArgParam("--" + prefix + "httpserver-host", "localhost");
	httpPort = conf->getArgInt("--" + prefix + "httpserver-port", "8080");
	dblog1 << myname << "(init): http server parameters " << httpHost << ":" << httpPort << endl;
	Poco::Net::SocketAddress sa(httpHost, httpPort);

	try
	{
		/*! \FIXME: доделать конфигурирование параметров */
		Poco::Net::HTTPServerParams* httpParams = new Poco::Net::HTTPServerParams;
		httpParams->setMaxQueued(100);
		httpParams->setMaxThreads(3);
		httpserv = std::make_shared<Poco::Net::HTTPServer>(this, Poco::Net::ServerSocket(sa), httpParams );
	}
	catch( std::exception& ex )
	{
		std::stringstream err;
		err << myname << "(init): " << httpHost << ":" << httpPort << " ERROR: " << ex.what();
		throw uniset::SystemError(err.str());
	}

#endif
}
//--------------------------------------------------------------------------------------------
LogDB::~LogDB()
{
	if( evIsActive() )
		evstop();

	if( httpserv )
		httpserv->stop();

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

	q << "INSERT INTO logs(tms,usec,name,text) VALUES('"
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

	string name = conf->getArgParam("--" + prefix + "name", "");

	if( name.empty() )
	{
		cerr << "(LogDB): Unknown name. Use --" << prefix << "name" << endl;
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
	if( httpserv )
		httpserv->start();

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
	pingWebSockets.stop();
}
// -----------------------------------------------------------------------------
void LogDB::evprepare()
{
	connectionTimer.set(loop);
	checkBufferTimer.set(loop);
	pingWebSockets.set(loop);

	if( tmConnection_msec != UniSetTimer::WaitUpTime )
		connectionTimer.start(0, tmConnection_sec);

	checkBufferTimer.start(0, tmCheckBuffer_sec);
	pingWebSockets.start(0, tmPingWebSockets_sec);
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
	for( const auto& s : logservers )
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

	if( peername.empty() )
		peername = ip + ":" + std::to_string(port);

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
		dbwarn << name << "(connect): connection " << peername << " timeout.." << endl;
	}
	catch( const Poco::Net::NetException& e )
	{
		dbwarn << name << "(connect): connection " << peername << " error: " << e.what() << endl;
	}
	catch( const std::exception& e )
	{
		dbwarn << name << "(connect): connection " << peername << " error: " << e.what() << endl;
	}
	catch( ... )
	{
		std::exception_ptr p = std::current_exception();
		dbwarn << name << "(connect): connection " << peername << " error: "
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

	// первый раз при подключении надо послать команды

	//! \todo Пока закрываем глаза на не оптимальность, того, что парсим строку каждый раз
	auto cmdlist = LogServerTypes::getCommands(cmd);

	if( !cmdlist.empty() )
	{
		for( const auto& msg : cmdlist )
			wbuf.emplace(new UTCPCore::Buffer((unsigned char*)&msg, sizeof(msg)));

		io.set(ev::WRITE);
	}
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
		read(watcher);

	if( revents & EV_WRITE )
		write(watcher);
}
// -----------------------------------------------------------------------------
LogDB::Log::ReadSignal LogDB::Log::signal_on_read()
{
	return sigRead;
}
// -----------------------------------------------------------------------------
void LogDB::Log::read( ev::io& watcher )
{
	if( !tcp )
		return;

	int n = tcp->available();

	n = std::min(n, bufsize);

	if( n > 0 )
	{
		tcp->receiveBytes(buf, n);

		// нарезаем на строки
		for( int i = 0; i < n; i++ )
		{
			if( buf[i] != '\n' )
				text += buf[i];
			else
			{
				sigRead.emit(this, text);
				text = "";

				if( text.capacity() < reservsize )
					text.reserve(reservsize);
			}
		}
	}
	else if( n == 0 )
	{
		dbinfo << name << ": " << ip << ":" << port << " connection is closed.." << endl;

		if( !text.empty() )
		{
			sigRead.emit(this, text);
			text = "";

			if( text.capacity() < reservsize )
				text.reserve(reservsize);
		}

		close();
	}
}
// -----------------------------------------------------------------------------
void LogDB::Log::write( ev::io& io )
{
	UTCPCore::Buffer* buffer = 0;

	if( wbuf.empty() )
	{
		io.set(EV_READ);
		return;
	}

	buffer = wbuf.front();

	if( !buffer )
		return;

	ssize_t ret = ::write(io.fd, buffer->dpos(), buffer->nbytes());

	if( ret < 0 )
	{
		dbwarn << peername << "(write): write to socket error(" << errno << "): " << strerror(errno) << endl;

		if( errno == EPIPE || errno == EBADF )
		{
			dbwarn << peername << "(write): write error.. terminate session.." << endl;
			io.set(EV_NONE);
			close();
		}

		return;
	}

	buffer->pos += ret;

	if( buffer->nbytes() == 0 )
	{
		wbuf.pop();
		delete buffer;
	}

	if( wbuf.empty() )
		io.set(EV_READ);
	else
		io.set(EV_WRITE);
}
// -----------------------------------------------------------------------------
void LogDB::Log::close()
{
	tcp->disconnect();
	//tcp = nullptr;
}
// -----------------------------------------------------------------------------
#ifndef DISABLE_REST_API
// -----------------------------------------------------------------------------
class LogDBRequestHandler:
	public Poco::Net::HTTPRequestHandler
{
	public:

		LogDBRequestHandler( LogDB* l ): logdb(l) {}

		virtual void handleRequest( Poco::Net::HTTPServerRequest& request,
									Poco::Net::HTTPServerResponse& response ) override
		{
			logdb->handleRequest(request, response);
		}

	private:
		LogDB* logdb;
};
// -----------------------------------------------------------------------------
class LogDBWebSocketRequestHandler:
	public Poco::Net::HTTPRequestHandler
{
	public:

		LogDBWebSocketRequestHandler( LogDB* l ): logdb(l) {}

		virtual void handleRequest( Poco::Net::HTTPServerRequest& request,
									Poco::Net::HTTPServerResponse& response ) override
		{
			logdb->onWebSocketSession(request, response);
		}

	private:
		LogDB* logdb;
};
// -----------------------------------------------------------------------------
Poco::Net::HTTPRequestHandler* LogDB::createRequestHandler( const Poco::Net::HTTPServerRequest& req )
{
	if( req.find("Upgrade") != req.end() && Poco::icompare(req["Upgrade"], "websocket") == 0 )
		return new LogDBWebSocketRequestHandler(this);

	return new LogDBRequestHandler(this);
}
// -----------------------------------------------------------------------------
void LogDB::handleRequest( Poco::Net::HTTPServerRequest& req, Poco::Net::HTTPServerResponse& resp )
{
	using Poco::Net::HTTPResponse;

	std::ostream& out = resp.send();
	resp.setContentType("text/json");

	try
	{
		// В этой версии API поддерживается только GET
		if( req.getMethod() != "GET" )
		{
			auto jdata = respError(resp, HTTPResponse::HTTP_BAD_REQUEST, "method must be 'GET'");
			jdata->stringify(out);
			out.flush();
			return;
		}

		Poco::URI uri(req.getURI());

		dblog3 << req.getHost() << ": query: " << uri.getQuery() << endl;

		std::vector<std::string> seg;
		uri.getPathSegments(seg);

		// example: http://host:port/api/version/logdb/..
		if( seg.size() < 4
				|| seg[0] != "api"
				|| seg[1] != uniset::UHttp::UHTTP_API_VERSION
				|| seg[2].empty()
				|| seg[2] != "logdb")
		{
			ostringstream err;
			err << "Bad request structure. Must be /api/" << uniset::UHttp::UHTTP_API_VERSION << "/logdb/xxx";
			auto jdata = respError(resp, HTTPResponse::HTTP_BAD_REQUEST, err.str());
			jdata->stringify(out);
			out.flush();
			return;
		}

		auto qp = uri.getQueryParameters();

		resp.setStatus(HTTPResponse::HTTP_OK);
		string cmd = seg[3];

		if( cmd == "help" )
		{
			using uniset::json::help::item;
			uniset::json::help::object myhelp("help");
			myhelp.emplace(item("help", "this help"));
			myhelp.emplace(item("list", "list of logs"));
			myhelp.emplace(item("count?logname", "count of logs for logname"));

			item l("logs", "read logs");
			l.param("from='YYYY-MM-DD'", "From date");
			l.param("to='YYYY-MM-DD'", "To date");
			l.param("last=XX[m|h|d|M]", "Last records (m - minute, h - hour, d - day, M - month)");
			l.param("offset=N", "offset");
			l.param("limit=M", "limit records for response");
			myhelp.add(l);

			myhelp.emplace(item("apidocs", "https://github.com/Etersoft/uniset2"));
			myhelp.get()->stringify(out);
		}
		else
		{
			auto json = httpGetRequest(cmd, qp);
			json->stringify(out);
		}
	}
	catch( std::exception& ex )
	{
		auto jdata = respError(resp, HTTPResponse::HTTP_INTERNAL_SERVER_ERROR, ex.what());
		jdata->stringify(out);
	}

	out.flush();
}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr LogDB::respError( Poco::Net::HTTPServerResponse& resp,
		Poco::Net::HTTPResponse::HTTPStatus estatus,
		const string& message )
{
	resp.setStatus(estatus);
	resp.setContentType("text/json");
	Poco::JSON::Object::Ptr jdata = new Poco::JSON::Object();
	jdata->set("error", resp.getReasonForStatus(resp.getStatus()));
	jdata->set("ecode", (int)resp.getStatus());
	jdata->set("message", message);
	return jdata;
}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr LogDB::httpGetRequest( const string& cmd, const Poco::URI::QueryParameters& p )
{
	if( cmd == "list" )
		return httpGetList(p);

	if( cmd == "logs" )
		return httpGetLogs(p);

	if( cmd == "count" )
		return httpGetCount(p);

	ostringstream err;
	err << "Unknown command '" << cmd << "'";
	throw uniset::SystemError(err.str());
}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr LogDB::httpGetList( const Poco::URI::QueryParameters& p )
{
	if( !db )
	{
		ostringstream err;
		err << "DB unavailable..";
		throw uniset::SystemError(err.str());
	}

	Poco::JSON::Object::Ptr jdata = new Poco::JSON::Object();

	Poco::JSON::Array::Ptr jlist = uniset::json::make_child_array(jdata, "logs");

#if 0
	// Получение из БД
	// хорошо тем, что возвращаем список реально доступных логов (т.е. тех что есть в БД)
	// плохо тем, что если в конфигурации добавили какие-то логи, но в БД
	// ещё ничего не попало, мы их не увидим

	ostringstream q;

	q << "SELECT COUNT(*), name FROM logs GROUP BY name";
	DBResult ret = db->query(q.str());

	if( !ret )
		return jdata;

	for( auto it = ret.begin(); it != ret.end(); ++it )
	{
		Poco::JSON::Object::Ptr j = new Poco::JSON::Object();
		j->set("name", it.as_string("name"));
		jlist->add(j);
	}

#else
	// Получение из конфигурации
	// хорошо тем, что если логов ещё не было
	// то всё-равно видно, какие доступны потенциально
	// плохо тем, что если конфигурацию поменяли (убрали какой-то лог)
	// а в БД записи по нему остались, то мы не получим к ним доступ

	/*! \todo пока список logservers формируется только в начале (в конструкторе)
	 * можно не защищаться mutex-ом, т.к. мы его не меняем
	 * если вдруг в REST API будет возможность добавлять логи.. нужно защищаться
	 * либо переделывать обработку
	 */
	for( const auto& s : logservers )
	{
		Poco::JSON::Object::Ptr j = new Poco::JSON::Object();
		j->set("name", s->name);
		j->set("description", s->description);
		jlist->add(j);
	}

#endif

	return jdata;
}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr LogDB::httpGetLogs( const Poco::URI::QueryParameters& params )
{
	Poco::JSON::Object::Ptr jdata = new Poco::JSON::Object();

	if( params.empty() || params[0].first.empty() )
	{
		ostringstream err;
		err << "BAD REQUEST: unknown logname";
		throw uniset::SystemError(err.str());
	}

	std::string logname = params[0].first;

	size_t offset = 0;
	size_t limit = 0;

	vector<std::string> q_where;

	for( const auto& p : params )
	{
		if( p.first == "offset" )
			offset = uni_atoi(p.second);
		else if( p.first == "limit" )
			limit = uni_atoi(p.second);
		else if( p.first == "from" )
			q_where.push_back("tms>='" + p.second + "'");
		else if( p.first == "to" )
			q_where.push_back("tms<='" + p.second + "'");
		else if( p.first == "last" )
			q_where.push_back(qLast(p.second));
	}

	Poco::JSON::Array::Ptr jlist = uniset::json::make_child_array(jdata, "logs");

	ostringstream q;

	q << "SELECT tms,"
	  << " strftime('%d-%m-%Y',datetime(tms,'unixepoch')) as date,"
	  << " strftime('%H:%M:%S',datetime(tms,'unixepoch')) as time,"
	  << " usec, text FROM logs WHERE name='" << logname << "'";

	if( !q_where.empty() )
	{
		for( const auto& w : q_where )
			q << " AND " << w;
	}

	if( limit > 0 )
		q << " ORDER BY tms ASC LIMIT " << offset << "," << limit;

	DBResult ret = db->query(q.str());

	if( !ret )
		return jdata;

	for( auto it = ret.begin(); it != ret.end(); ++it )
	{
		Poco::JSON::Object::Ptr j = new Poco::JSON::Object();
		j->set("tms", it.as_string("tms"));
		j->set("date", it.as_string("date"));
		j->set("time", it.as_string("time"));
		j->set("usec", it.as_string("usec"));
		j->set("text", it.as_string("text"));
		jlist->add(j);
	}

	return jdata;
}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr LogDB::httpGetCount( const Poco::URI::QueryParameters& params )
{
	Poco::JSON::Object::Ptr jdata = new Poco::JSON::Object();

	std::string logname = params[0].first;

	if( logname.empty() )
	{
		ostringstream err;
		err << "BAD REQUEST: unknown logname";
		throw uniset::SystemError(err.str());
	}

	ostringstream q;

	q << "SELECT count(*) FROM logs WHERE name='" << logname << "'";

	DBResult ret = db->query(q.str());

	if( !ret )
	{
		jdata->set("name", logname);
		jdata->set("count", 0);
		return jdata;
	}

	auto it = ret.begin();

	jdata->set("name", logname);
	jdata->set("count", it.as_int(0));
	return jdata;
}
// -----------------------------------------------------------------------------
string LogDB::qLast( const string& p )
{
	if( p.empty() )
		return "";

	char unit =  p[p.size() - 1];
	std::string sval = p.substr(0, p.size() - 1);

	if( unit == 'h' || unit == 'H' )
	{
		size_t h = uni_atoi(sval);
		ostringstream q;
		q << "tms >= strftime('%s',datetime('now')) - " << h << "*60*60";
		return q.str();
	}
	else if( unit == 'd' || unit == 'D' )
	{
		size_t d = uni_atoi(sval);
		ostringstream q;
		q << "tms >= strftime('%s',datetime('now')) - " << d << "*24*60*60";
		return q.str();
	}
	else if( unit == 'M' )
	{
		size_t m = uni_atoi(sval);
		ostringstream q;
		q << "tms >= strftime('%s',datetime('now')) - " << m << "*30*24*60*60";
		return q.str();
	}
	else // по умолчанию минут
	{
		size_t m = (unit == 'm') ? uni_atoi(sval) : uni_atoi(p);
		ostringstream q;
		q << "tms >= strftime('%s',datetime('now')) - " << m << "*60";
		return q.str();
	}

	return "";
}
// -----------------------------------------------------------------------------
void LogDB::onWebSocketSession(Poco::Net::HTTPServerRequest& req, Poco::Net::HTTPServerResponse& resp)
{
	using Poco::Net::WebSocket;
	using Poco::Net::WebSocketException;
	using Poco::Net::HTTPResponse;
	using Poco::Net::HTTPServerRequest;

	std::vector<std::string> seg;

	Poco::URI uri(req.getURI());

	uri.getPathSegments(seg);

	// example: http://host:port/logdb/ws/logname
	if( seg.size() < 3
			|| seg[0] != "logdb"
			|| seg[1] != "ws"
			|| seg[2].empty())
	{

		resp.setStatus(HTTPResponse::HTTP_BAD_REQUEST);
		resp.setContentType("text/html");
		resp.setStatusAndReason(HTTPResponse::HTTP_BAD_REQUEST);
		resp.setContentLength(0);
		std::ostream& err = resp.send();
		err << "Bad request. Must be:  http://host:port/logdb/ws/logname";
		err.flush();
		return;
	}

	auto ws = newWebSocket(&req, &resp, seg[2]);

	if( !ws )
		return;

	LogWebSocketGuard lk(ws, this);

	dblog3 << myname << "(onWebSocketSession): start session for " << req.clientAddress().toString() << endl;

	// т.к. вся работа происходит в eventloop
	// то здесь просто ждём..
	ws->waitCompletion();

	dblog3 << myname << "(onWebSocketSession): finish session for " << req.clientAddress().toString() << endl;
}
// -----------------------------------------------------------------------------
std::shared_ptr<LogDB::LogWebSocket> LogDB::newWebSocket( Poco::Net::HTTPServerRequest* req,
		Poco::Net::HTTPServerResponse* resp,
		const std::string& logname )
{
	using Poco::Net::WebSocket;
	using Poco::Net::WebSocketException;
	using Poco::Net::HTTPResponse;
	using Poco::Net::HTTPServerRequest;

	std::shared_ptr<Log> log;

	for( const auto& s : logservers )
	{
		if( s->name == logname )
		{
			log = s;
			break;
		}
	}

	if( !log )
	{
		resp->setStatus(HTTPResponse::HTTP_BAD_REQUEST);
		resp->setContentType("text/html");
		resp->setStatusAndReason(HTTPResponse::HTTP_NOT_FOUND);
		std::ostream& err = resp->send();
		err << "Not found '" << logname << "'";
		err.flush();
		return nullptr;
	}


	uniset_rwmutex_wrlock lock(wsocksMutex);
	std::shared_ptr<LogWebSocket> ws = make_shared<LogWebSocket>(req, resp, log, loop);
	wsocks.emplace_back(ws);
	return ws;
}
// -----------------------------------------------------------------------------
void LogDB::delWebSocket( std::shared_ptr<LogWebSocket>& ws )
{
	uniset_rwmutex_wrlock lock(wsocksMutex);

	for( auto it = wsocks.begin(); it != wsocks.end(); it++ )
	{
		if( (*it).get() == ws.get() )
		{
			dblog3 << myname << ": delete websocket "
				   << endl;
			wsocks.erase(it);
			return;
		}
	}
}
// -----------------------------------------------------------------------------
void LogDB::onPingWebSockets( ev::timer& t, int revents )
{
	if (EV_ERROR & revents)
	{
		dbcrit << myname << "(onPingWebSockets): invalid event" << endl;
		return;
	}

	uniset_rwmutex_rlock lk(wsocksMutex);

	for( const auto& s : wsocks )
		s->ping();
}
// -----------------------------------------------------------------------------
LogDB::LogWebSocket::LogWebSocket(Poco::Net::HTTPServerRequest* _req,
								  Poco::Net::HTTPServerResponse* _resp,
								  std::shared_ptr<Log>& _log,
								  ev::dynamic_loop& loop ):
	Poco::Net::WebSocket(*_req, *_resp),
	req(_req),
	resp(_resp)
	//	log(_log)
{
	setBlocking(false);
	con = _log->signal_on_read().connect( sigc::mem_fun(*this, &LogWebSocket::add));
	io.set(loop);
	io.set<LogDB::LogWebSocket, &LogDB::LogWebSocket::event>(this);
	io.start();
}
// -----------------------------------------------------------------------------
LogDB::LogWebSocket::~LogWebSocket()
{
	if( !cancelled )
		term();

	// удаляем всё что осталось
	while(!wbuf.empty())
	{
		delete wbuf.front();
		wbuf.pop();
	}
}
// -----------------------------------------------------------------------------
void LogDB::LogWebSocket::event( ev::io& watcher, int revents )
{
	if( EV_ERROR & revents )
		return;

	if( revents & EV_WRITE )
		write(watcher);
}
// -----------------------------------------------------------------------------
void LogDB::LogWebSocket::ping()
{
	if( cancelled )
		return;

	wbuf.emplace(new UTCPCore::Buffer("."));
	io.set(ev::WRITE);
}
// -----------------------------------------------------------------------------
void LogDB::LogWebSocket::add( LogDB::Log* log, const string& txt )
{
	if( cancelled || txt.empty())
		return;

	wbuf.emplace(new UTCPCore::Buffer(txt));
	io.set(ev::WRITE);
}
// -----------------------------------------------------------------------------
void LogDB::LogWebSocket::write( ev::io& w )
{
	UTCPCore::Buffer* msg = 0;

	if( wbuf.empty() )
	{
		io.set(EV_NONE);
		return;
	}

	msg = wbuf.front();

	if( !msg )
		return;

	using Poco::Net::WebSocket;
	using Poco::Net::WebSocketException;
	using Poco::Net::HTTPResponse;
	using Poco::Net::HTTPServerRequest;

	int flags = WebSocket::FRAME_TEXT;

	if( msg->len == 1 ) // это пинг состоящий из "."
		flags = WebSocket::FRAME_FLAG_FIN | WebSocket::FRAME_OP_PING;

	try
	{
		ssize_t ret = sendFrame(msg->dpos(), msg->nbytes(), flags);

		if( ret < 0 )
		{
			cerr << "(websocket): " << req->clientAddress().toString()
				 << "  write to socket error(" << errno << "): " << strerror(errno) << endl;

			if( errno == EPIPE || errno == EBADF )
			{
				cerr << "(websocket): "
					 << req->clientAddress().toString()
					 << " write error.. terminate session.." << endl;

				term();
			}

			return;
		}

		msg->pos += ret;

		if( msg->nbytes() == 0 )
		{
			wbuf.pop();
			delete msg;
		}

		if( !wbuf.empty() )
			io.set(EV_WRITE);

		return;
	}
	catch( WebSocketException& exc )
	{
		switch( exc.code() )
		{
			case WebSocket::WS_ERR_HANDSHAKE_UNSUPPORTED_VERSION:
				resp->set("Sec-WebSocket-Version", WebSocket::WEBSOCKET_VERSION);

			case WebSocket::WS_ERR_NO_HANDSHAKE:
			case WebSocket::WS_ERR_HANDSHAKE_NO_VERSION:
			case WebSocket::WS_ERR_HANDSHAKE_NO_KEY:
				resp->setStatusAndReason(HTTPResponse::HTTP_BAD_REQUEST);
				resp->setContentLength(0);
				resp->send();
				break;
		}
	}
	catch( const Poco::Net::NetException& e )
	{
		cerr << "(websocket):NetException: "
			 << req->clientAddress().toString()
			 << " error: " << e.displayText()
			 << endl;
	}
	catch( Poco::IOException& ex )
	{
		cerr << "(websocket): IOException: "
			 << req->clientAddress().toString()
			 << " error: " << ex.displayText()
			 << endl;
	}

	term();
}
// -----------------------------------------------------------------------------
void LogDB::LogWebSocket::term()
{
	if( cancelled )
		return;

	cancelled = true;
	con.disconnect();
	io.stop();
	finish.notify_all();
}
// -----------------------------------------------------------------------------
void LogDB::LogWebSocket::waitCompletion()
{
	std::unique_lock<std::mutex> lk(finishmut);
	while( !cancelled )
		finish.wait(lk);
}
// -----------------------------------------------------------------------------
#endif
// -----------------------------------------------------------------------------
