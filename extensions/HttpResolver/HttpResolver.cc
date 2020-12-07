/*
 * Copyright (c) 2020 Pavel Vainerman.
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
#include <unistd.h>

#include "unisetstd.h"
#include <Poco/Net/NetException.h>
#include "ujson.h"
#include "HttpResolver.h"
#include "Configuration.h"
#include "Exceptions.h"
#include "Debug.h"
#include "UniXML.h"
#include "HttpResolverSugar.h"
// --------------------------------------------------------------------------
using namespace uniset;
using namespace std;
// --------------------------------------------------------------------------
HttpResolver::HttpResolver( const string& name, int argc, const char* const* argv, const string& prefix ):
	myname(name)
{
	rlog = make_shared<DebugStream>();

	auto logLevels = uniset::getArgParam("--" + prefix + "log-add-levels", argc, argv, "");

	if( !logLevels.empty() )
		rlog->addLevel( Debug::value(logLevels) );

	std::string config = uniset::getArgParam("--confile", argc, argv, "configure.xml");

	if( config.empty() )
		throw SystemError("Unknown config file");

	std::shared_ptr<UniXML> xml;
	cout << myname << "(init): init from " << config << endl;
	xml = make_shared<UniXML>();

	try
	{
		xml->open(config);
	}
	catch( std::exception& ex )
	{
		throw ex;
	}

	xmlNode* cnode = xml->findNode(xml->getFirstNode(), "HttpResolver", name);

	if( !cnode )
	{
		ostringstream err;
		err << name << "(init): Not found confnode <HttpResolver name='" << name << "'...>";
		rcrit << err.str() << endl;
		throw uniset::SystemError(err.str());
	}

	UniXML::iterator it(cnode);

	UniXML::iterator dirIt = xml->findNode(xml->getFirstNode(), "LockDir");

	if( !dirIt )
	{
		ostringstream err;
		err << name << "(init): Not found confnode <LockDir name='..'/>";
		rcrit << err.str() << endl;
		throw uniset::SystemError(err.str());
	}


	iorfile = make_shared<IORFile>(dirIt.getProp("name"));
	rinfo << myname << "(init): iot directory: " << dirIt.getProp("name") << endl;

	httpHost = uniset::getArgParam("--" + prefix + "httpserver-host", argc, argv, "localhost");
	httpPort = uniset::getArgInt("--" + prefix + "httpserver-port", argc, argv, "8008");
	httpCORS_allow = uniset::getArgParam("--" + prefix + "httpserver-cors-allow", argc, argv, httpCORS_allow);
	httpReplyAddr = uniset::getArgParam("--" + prefix + "httpserver-reply-addr", argc, argv, "");

	rinfo << myname << "(init): http server parameters " << httpHost << ":" << httpPort << endl;
	Poco::Net::SocketAddress sa(httpHost, httpPort);

	try
	{
		Poco::Net::HTTPServerParams* httpParams = new Poco::Net::HTTPServerParams;

		int maxQ = uniset::getArgPInt("--" + prefix + "httpserver-max-queued", argc, argv, it.getProp("httpMaxQueued"), 100);
		int maxT = uniset::getArgPInt("--" + prefix + "httpserver-max-threads", argc, argv, it.getProp("httpMaxThreads"), 3);

		httpParams->setMaxQueued(maxQ);
		httpParams->setMaxThreads(maxT);
		httpserv = std::make_shared<Poco::Net::HTTPServer>(new HttpResolverRequestHandlerFactory(this), Poco::Net::ServerSocket(sa), httpParams );
	}
	catch( std::exception& ex )
	{
		std::stringstream err;
		err << myname << "(init): " << httpHost << ":" << httpPort << " ERROR: " << ex.what();
		throw uniset::SystemError(err.str());
	}
}
//--------------------------------------------------------------------------------------------
HttpResolver::~HttpResolver()
{
	if( httpserv )
		httpserv->stop();
}
//--------------------------------------------------------------------------------------------
std::shared_ptr<HttpResolver> HttpResolver::init_resolver( int argc, const char* const* argv, const std::string& prefix )
{
	string name = uniset::getArgParam("--" + prefix + "name", argc, argv, "HttpResolver");

	if( name.empty() )
	{
		cerr << "(HttpResolver): Unknown name. Use --" << prefix << "name" << endl;
		return nullptr;
	}

	return make_shared<HttpResolver>(name, argc, argv, prefix);
}
// -----------------------------------------------------------------------------
void HttpResolver::help_print()
{
	cout << "Default: prefix='logdb'" << endl;
	cout << "--prefix-single-confile conf.xml     - Отдельный конфигурационный файл (не требующий структуры uniset)" << endl;
	cout << "--prefix-name name                   - Имя. Для поиска настроечной секции в configure.xml" << endl;
	cout << "http: " << endl;
	cout << "--prefix-httpserver-host ip                 - IP на котором слушает http сервер. По умолчанию: localhost" << endl;
	cout << "--prefix-httpserver-port num                - Порт на котором принимать запросы. По умолчанию: 8080" << endl;
	cout << "--prefix-httpserver-max-queued num          - Размер очереди запросов к http серверу. По умолчанию: 100" << endl;
	cout << "--prefix-httpserver-max-threads num         - Разрешённое количество потоков для http-сервера. По умолчанию: 3" << endl;
	cout << "--prefix-httpserver-cors-allow addr         - (CORS): Access-Control-Allow-Origin. Default: *" << endl;
	cout << "--prefix-httpserver-reply-addr host[:port]  - Адрес отдаваемый клиенту для подключения. По умолчанию адрес узла где запущен logdb" << endl;
}
// -----------------------------------------------------------------------------
void HttpResolver::run()
{
	if( httpserv )
		httpserv->start();

	pause();
}
// -----------------------------------------------------------------------------
class HttpResolverRequestHandler:
	public Poco::Net::HTTPRequestHandler
{
	public:

		HttpResolverRequestHandler( HttpResolver* r ): resolver(r) {}

		virtual void handleRequest( Poco::Net::HTTPServerRequest& request,
									Poco::Net::HTTPServerResponse& response ) override
		{
			resolver->handleRequest(request, response);
		}

	private:
		HttpResolver* resolver;
};
// -----------------------------------------------------------------------------
Poco::Net::HTTPRequestHandler* HttpResolver::HttpResolverRequestHandlerFactory::createRequestHandler( const Poco::Net::HTTPServerRequest& req )
{
	return new HttpResolverRequestHandler(resolver);
}
// -----------------------------------------------------------------------------
void HttpResolver::handleRequest( Poco::Net::HTTPServerRequest& req, Poco::Net::HTTPServerResponse& resp )
{
	using Poco::Net::HTTPResponse;

	std::ostream& out = resp.send();

	resp.setContentType("text/json");
	resp.set("Access-Control-Allow-Methods", "GET");
	resp.set("Access-Control-Allow-Request-Method", "*");
	resp.set("Access-Control-Allow-Origin", httpCORS_allow /* req.get("Origin") */);

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

		rlog3 << req.getHost() << ": query: " << uri.getQuery() << endl;

		std::vector<std::string> seg;
		uri.getPathSegments(seg);

		// example: http://host:port/api/version/resolve/[json|text]
		if( seg.size() < 4
				|| seg[0] != "api"
				|| seg[1] != HTTP_RESOLVER_API_VERSION
				|| seg[2].empty()
				|| seg[2] != "resolve")
		{
			ostringstream err;
			err << "Bad request structure. Must be /api/" << HTTP_RESOLVER_API_VERSION << "/resolve/[json|text|help]?name";
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
			myhelp.emplace(item("resolve?name", "resolve name"));
			//			myhelp.emplace(item("apidocs", "https://github.com/Etersoft/uniset2"));

			item l("resolve?name", "resolve name");
			l.param("format=json|text", "response format");
			myhelp.add(l);
			myhelp.get()->stringify(out);
		}
		else if( cmd == "json" )
		{
			auto json = httpJsonResolve(uri.getQuery(), qp);
			json->stringify(out);
		}
		else if( cmd == "text" )
		{
			resp.setContentType("text/plain");
			auto txt = httpTextResolve(uri.getQuery(), qp);
			out << txt;
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
Poco::JSON::Object::Ptr HttpResolver::respError( Poco::Net::HTTPServerResponse& resp,
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
Poco::JSON::Object::Ptr HttpResolver::httpJsonResolve( const std::string& query, const Poco::URI::QueryParameters& p )
{
	auto iorstr = httpTextResolve(query, p);
	Poco::JSON::Object::Ptr jdata = new Poco::JSON::Object();
	jdata->set("ior", iorstr);
	return jdata;
}
// -----------------------------------------------------------------------------
std::string HttpResolver::httpTextResolve(  const std::string& query, const Poco::URI::QueryParameters& p )
{
	if( query.empty() )
	{
		uwarn << myname << "undefined parameters for resolve" << endl;
		return "";
	}

	if( uniset::is_digit(query) )
	{
		uinfo << myname << " resolve " << query << endl;
		return iorfile->getIOR( uniset::uni_atoi(query) );
	}

	uwarn << myname << "unknown parameter type '" << query << "'" << endl;
	return "";
}
// -----------------------------------------------------------------------------
