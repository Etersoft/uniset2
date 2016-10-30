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
#include <Poco/URI.h>
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
		resp.setStatus(HTTPResponse::HTTP_NOT_FOUND);
		resp.setContentType("text/json");
		std::ostream& out = resp.send();
		out.flush();
		return;
	}

	Poco::URI uri(req.getURI());

	if( log->is_info() )
		log->info() << req.getHost() << ": query: " << uri.getQuery() << endl;

	std::vector<std::string> seg;
	uri.getPathSegments(seg);

	// example: http://host:port/api-version/get/ObjectName
	if( seg.size() < 3
		|| seg[0] != UHTTP_API_VERSION
		|| seg[1] != "get"
		|| seg[2].empty() )
	{
		resp.setStatus(HTTPResponse::HTTP_BAD_REQUEST);
		resp.setContentType("text/json");
		std::ostream& out = resp.send();
		out.flush();
		return;
	}

	const std::string objectName(seg[2]);

//	auto qp = uri.getQueryParameters();
//	cerr << "params: " << endl;
//	for( const auto& p: qp )
//		cerr << p.first << "=" << p.second << endl;

	resp.setStatus(HTTPResponse::HTTP_OK);
	resp.setContentType("text/json");
	std::ostream& out = resp.send();

	auto json = registry->getDataByName(objectName);
	out << json.dump();
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
