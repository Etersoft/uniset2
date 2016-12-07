#ifndef DISABLE_REST_API
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
#include <Poco/JSON/Parser.h>
#include "Exceptions.h"
#include "UHttpRequestHandler.h"
// -------------------------------------------------------------------------
using namespace std;
using namespace Poco::Net;
using namespace uniset;
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
		Poco::JSON::Object::Ptr jdata = new Poco::JSON::Object();
		jdata->set("error", resp.getReasonForStatus(resp.getStatus()));
		jdata->set("ecode", resp.getStatus());
		jdata->set("message", "Unknown 'registry of objects'");
		jdata->stringify(out);
		out.flush();
		return;
	}

	// В этой версии API поддерживается только GET
	if( req.getMethod() != "GET" )
	{
		resp.setStatus(HTTPResponse::HTTP_BAD_REQUEST);
		resp.setContentType("text/json");
		std::ostream& out = resp.send();
		Poco::JSON::Object jdata;
		jdata.set("error", resp.getReasonForStatus(resp.getStatus()));
		jdata.set("ecode", (int)resp.getStatus());
		jdata.set("message", "method must be 'GET'");
		jdata.stringify(out);
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
		Poco::JSON::Object jdata;
		jdata.set("error", resp.getReasonForStatus(resp.getStatus()));
		jdata.set("ecode", (int)resp.getStatus());
		jdata.set("message", "BAD REQUEST STRUCTURE");
		jdata.stringify(out);
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
			out << "{ \"help\": ["
				"{\"help\": {\"desc\": \"this help\"}},"
				"{\"list\": {\"desc\": \"list of objects\"}},"
				"{\"ObjectName\": {\"desc\": \"ObjectName information\"}},"
				"{\"ObjectName/help\": {\"desc\": \"help for ObjectName\"}},"
				"{\"apidocs\": {\"desc\": \"https://github.com/Etersoft/uniset2\"}}"
				"]}";
		}
		else if( objectName == "list" )
		{
			auto json = registry->httpGetObjectsList(qp);
			json->stringify(out);
		}
		else if( seg.size() == 4 && seg[3] == "help" ) // /api/version/ObjectName/help
		{
			auto json = registry->httpHelpByName(objectName, qp);
			json->stringify(out);
		}
		else if( seg.size() >= 4 ) // /api/version/ObjectName/xxx..
		{
			auto json = registry->httpRequestByName(objectName, seg[3], qp);
			json->stringify(out);
		}
		else
		{
			auto json = registry->httpGetByName(objectName, qp);
			json->stringify(out);
		}
	}
	//	catch( Poco::JSON::JSONException jsone )
	//	{
	//		std::cout << "JSON ERROR: " << jsone.message() << std::endl;
	//	}
	catch( std::exception& ex )
	{
		ostringstream err;
		err << ex.what();
		resp.setStatus(HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
		resp.setContentType("text/json");
		Poco::JSON::Object jdata;
		jdata.set("error", err.str());
		jdata.set("ecode", (int)resp.getStatus());
		jdata.stringify(out);
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
Poco::JSON::Object::Ptr IHttpRequest::httpRequest( const string& req, const Poco::URI::QueryParameters& p )
{
	std::ostringstream err;
	err << "(IHttpRequest::Request): " << req << " not supported";
	throw uniset::SystemError(err.str());
}
// -------------------------------------------------------------------------
#endif
