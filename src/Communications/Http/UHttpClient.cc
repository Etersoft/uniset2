#ifndef DISABLE_REST_API
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
// -------------------------------------------------------------------------
#include <sstream>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/StreamCopier.h>
#include "UHttpClient.h"
#include "Exceptions.h"
// -------------------------------------------------------------------------
using namespace Poco::Net;
// -------------------------------------------------------------------------
namespace uniset
{
	using namespace UHttp;
	// -------------------------------------------------------------------------

	UHttpClient::UHttpClient()
	{
	}
	// -------------------------------------------------------------------------
	UHttpClient::~UHttpClient()
	{
	}
	// -------------------------------------------------------------------------
	std::string UHttpClient::get( const std::string& host, int port, const std::string& query )
	{
		session.setHost(host);
		session.setPort(port);
		HTTPRequest request(HTTPRequest::HTTP_GET, query, HTTPMessage::HTTP_1_1);
		session.sendRequest(request);
		HTTPResponse response;
		std::istream& rs = session.receiveResponse(response);

		if( response.getStatus() != Poco::Net::HTTPResponse::HTTP_OK )
			return "";

		std::stringstream ret;
		Poco::StreamCopier::copyStream(rs, ret);
		return ret.str();
	}
	// -------------------------------------------------------------------------
} // end of namespace uniset
// -------------------------------------------------------------------------
#endif // #ifndef DISABLE_REST_API
