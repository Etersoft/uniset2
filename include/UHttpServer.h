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
#ifndef UHttpServer_H_
#define UHttpServer_H_
// -------------------------------------------------------------------------
#include <string>
#include <memory>
#include <Poco/Net/HTTPServer.h>
#include "DebugStream.h"
#include "ThreadCreator.h"
#include "UHttpRequestHandler.h"
// -------------------------------------------------------------------------
/*! \page pgUHttpServer Http сервер
	Http сервер предназначен для получения информации о UniSetObject-ах через http (json).
*/
// -------------------------------------------------------------------------
namespace uniset
{
namespace UHttp
{
class UHttpServer
{
	public:

		UHttpServer( std::shared_ptr<IHttpRequestRegistry>& supplier, const std::string& host, int port );
		virtual ~UHttpServer();

		void start();
		void stop();

		std::shared_ptr<DebugStream> log();

	protected:
		UHttpServer();

	private:

		std::shared_ptr<DebugStream> mylog;
		Poco::Net::SocketAddress sa;

		std::shared_ptr<Poco::Net::HTTPServer> http;
		std::shared_ptr<UHttpRequestHandlerFactory> reqFactory;

};
}
}
// -------------------------------------------------------------------------
#endif // UHttpServer_H_
// -------------------------------------------------------------------------
#endif
