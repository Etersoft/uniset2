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
#include <Poco/URI.h>
#include "UHttpServer.h"
// -------------------------------------------------------------------------
using namespace Poco::Net;
using namespace UniSetTypes;
// -------------------------------------------------------------------------

UHttpServer::UHttpServer(std::shared_ptr<IHttpRequestRegistry>& supplier, const std::string _host, int _port ):
    sa(_host,_port)
{
    /*! \FIXME: доделать конфигурирование параметров */
    HTTPServerParams* httpParams = new HTTPServerParams;
    httpParams->setMaxQueued(100);
    httpParams->setMaxThreads(1);

    reqFactory = std::make_shared<UHttpRequestHandlerFactory>(supplier);

    http = std::make_shared<Poco::Net::HTTPServer>(reqFactory.get(), ServerSocket(sa), httpParams );
}
// -------------------------------------------------------------------------
UHttpServer::~UHttpServer()
{

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
