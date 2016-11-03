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
#include <ostream>
#include "Exceptions.h"
#include "UHttpRequestHandler.h"
// -------------------------------------------------------------------------
using namespace std;
using namespace Poco::Net;
using namespace UniSetTypes;
using namespace UHttp;
// -------------------------------------------------------------------------
UHttpRequestHandler::UHttpRequestHandler(std::shared_ptr<IHttpRequestRegistry> _registry ):
	registry(_registry)
{
	log = make_shared<DebugStream>();
}
// -------------------------------------------------------------------------
void UHttpRequestHandler::handleRequest( Poco::Net::HTTPServerRequest& req, Poco::Net::HTTPServerResponse& resp )
{
	if( !registry )
	{
		resp.setStatus(HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
		resp.setContentType("text/json");
		std::ostream& out = resp.send();
		nlohmann::json jdata;
		jdata["error"] = resp.getReasonForStatus(resp.getStatus());
		jdata["ecode"] = resp.getStatus();
		out << jdata.dump();
		out.flush();
		return;
	}

	// В этой версии API поддерживается только GET
	if( req.getMethod() != "GET" )
	{
		resp.setStatus(HTTPResponse::HTTP_BAD_REQUEST);
		resp.setContentType("text/json");
		std::ostream& out = resp.send();
		nlohmann::json jdata;
		jdata["error"] = resp.getReasonForStatus(resp.getStatus());
		jdata["ecode"] = resp.getStatus();
		out << jdata.dump();
		out.flush();
		return;
	}

	Poco::URI uri(req.getURI());

	if( log->is_info() )
		log->info() << req.getHost() << ": query: " << uri.getQuery() << endl;

	std::vector<std::string> seg;
	uri.getPathSegments(seg);

	// example: http://host:port/api/version/ObjectName
	if( seg.size() < 3
		|| seg[0] != "api"
		|| seg[1] != UHTTP_API_VERSION
		|| seg[2].empty() )
	{
		resp.setStatus(HTTPResponse::HTTP_BAD_REQUEST);
		resp.setContentType("text/json");
		std::ostream& out = resp.send();
		nlohmann::json jdata;
		jdata["error"] = resp.getReasonForStatus(resp.getStatus());
		jdata["ecode"] = resp.getStatus();
		out << jdata.dump();
		out.flush();
		return;
	}

	const std::string objectName(seg[2]);
	auto qp = uri.getQueryParameters();

	resp.setStatus(HTTPResponse::HTTP_OK);
	resp.setContentType("text/json");
	std::ostream& out = resp.send();

	try
	{
		if( objectName == "help" )
		{
			nlohmann::json jdata;
			jdata["help"] = {
			  {"help", {"desc", "this help"}},
			  {"list", {"desc", "list of objects"}},
			  {"ObjectName", {"desc", "'ObjectName' information"}},
			  {"ObjectName/help", {"desc", "help for ObjectName"}},
			  {"apidocs", {"desc", "https://github.com/Etersoft/uniset2"}}
			};

			out << jdata.dump();
		}
		else if( objectName == "list" )
		{
			auto json = registry->httpGetObjectsList(qp);
			out << json.dump();
		}
		else if( seg.size() == 4 && seg[3] == "help" ) // /api/version/ObjectName/help
		{
			auto json = registry->httpHelpByName(objectName, qp);
			out << json.dump();
		}
		else if( seg.size() >= 4 ) // /api/version/ObjectName/xxx..
		{
			auto json = registry->httpRequestByName(objectName, seg[3], qp);
			out << json.dump();
		}
		else
		{
			auto json = registry->httpGetByName(objectName, qp);
			out << json.dump();
		}
	}
	catch( std::exception& ex )
	{
		ostringstream err;
		err << ex.what();
		resp.setStatus(HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
		resp.setContentType("text/json");
		nlohmann::json jdata;
		jdata["error"] = err.str();
		jdata["ecode"] = resp.getStatus();
		out << jdata.dump();
	}

	out.flush();
}
// -------------------------------------------------------------------------

UHttpRequestHandlerFactory::UHttpRequestHandlerFactory(std::shared_ptr<IHttpRequestRegistry>& _registry ):
	registry(_registry)
{

}
// -------------------------------------------------------------------------
HTTPRequestHandler* UHttpRequestHandlerFactory::createRequestHandler( const HTTPServerRequest& req )
{
	return new UHttpRequestHandler(registry);
}
// -------------------------------------------------------------------------
nlohmann::json IHttpRequest::httpRequest( const string& req, const Poco::URI::QueryParameters& p )
{
	std::ostringstream err;
	err << "(IHttpRequest::Request): " << req << " not supported";
	throw UniSetTypes::SystemError(err.str());
}
// -------------------------------------------------------------------------
