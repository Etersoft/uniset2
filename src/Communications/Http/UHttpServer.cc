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
#include <sstream>
#include <Poco/URI.h>
#include "UHttpServer.h"
#include "Exceptions.h"
// -------------------------------------------------------------------------
using namespace Poco::Net;
// -------------------------------------------------------------------------
namespace uniset
{
	using namespace UHttp;
	// -------------------------------------------------------------------------

	UHttpServer::UHttpServer(std::shared_ptr<IHttpRequestRegistry>& supplier, const std::string& _host, int _port ):
		sa(_host, _port)
	{
		try
		{
			mylog = std::make_shared<DebugStream>();

			/*! \FIXME: доделать конфигурирование параметров */
			HTTPServerParams* httpParams = new HTTPServerParams;
			httpParams->setMaxQueued(100);
			httpParams->setMaxThreads(1);

			reqFactory = std::make_shared<UHttpRequestHandlerFactory>(supplier);

			http = std::make_shared<Poco::Net::HTTPServer>(reqFactory.get(), ServerSocket(sa), httpParams );
		}
		catch( std::exception& ex )
		{
			std::stringstream err;
			err << "(UHttpServer::init): " << _host << ":" << _port << " ERROR: " << ex.what();
			throw uniset::SystemError(err.str());
		}

		mylog->info() << "(UHttpServer::init): init " << _host << ":" << _port << std::endl;
	}
	// -------------------------------------------------------------------------
	UHttpServer::~UHttpServer()
	{
		if( http )
			http->stop();
	}
	// -------------------------------------------------------------------------
	void UHttpServer::start()
	{
		http->start();
	}
	// -------------------------------------------------------------------------
	void UHttpServer::stop()
	{
		http->stop();
	}
	// -------------------------------------------------------------------------
	UHttpServer::UHttpServer()
	{
	}
	// -------------------------------------------------------------------------
	std::shared_ptr<DebugStream> UHttpServer::log()
	{
		return mylog;
	}
	// -------------------------------------------------------------------------
	void UHttpServer::setCORS_allow( const std::string& allow )
	{
		reqFactory->setCORS_allow(allow);
	}
	// -------------------------------------------------------------------------
} // end of namespace uniset
// -------------------------------------------------------------------------
#endif // #ifndef DISABLE_REST_API
