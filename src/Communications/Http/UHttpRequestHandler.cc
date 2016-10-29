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
using namespace Poco::Net;
using namespace UniSetTypes;
// -------------------------------------------------------------------------
UHttpRequestHandler::UHttpRequestHandler( std::shared_ptr<IHttpRequestRegistry> _supplier ):
	supplier(_supplier)
{

}
// -------------------------------------------------------------------------
void UHttpRequestHandler::handleRequest( Poco::Net::HTTPServerRequest& req, Poco::Net::HTTPServerResponse& resp )
{
	if( !supplier )
	{
		resp.setStatus(HTTPResponse::HTTP_NOT_FOUND);
		resp.setContentType("text/json");
		std::ostream& out = resp.send();
		out.flush();
		return;
	}

	Poco::URI uri(req.getURI());

	// в текущей версии подразумевается, что запрос идёт в формате
	// http://xxx.host:port/ObjectName
	// поэтому сразу передаём uri.getQuery() в качестве имени объекта

	resp.setStatus(HTTPResponse::HTTP_OK);
	resp.setContentType("text/json");
	std::ostream& out = resp.send();

	auto json = supplier->getData(uri.getQuery());
	out << json.dump();
	out.flush();
}
// -------------------------------------------------------------------------

UHttpRequestHandlerFactory::UHttpRequestHandlerFactory( std::shared_ptr<IHttpRequestRegistry>& _supplier ):
	supplier(_supplier)
{

}
// -------------------------------------------------------------------------
HTTPRequestHandler* UHttpRequestHandlerFactory::createRequestHandler( const HTTPServerRequest& req )
{
	return new UHttpRequestHandler(supplier);
}
// -------------------------------------------------------------------------
