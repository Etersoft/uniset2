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
#include <sstream>
#include <Poco/JSON/Parser.h>
#include "Exceptions.h"
#include "UHttpRequestHandler.h"
// -------------------------------------------------------------------------
using namespace std;
using namespace Poco::Net;
// -------------------------------------------------------------------------
namespace uniset
{
    using namespace UHttp;

    // -------------------------------------------------------------------------
    // HttpRequestContext implementation
    // -------------------------------------------------------------------------
    HttpRequestContext::HttpRequestContext(Poco::Net::HTTPServerRequest& req,
                                           Poco::Net::HTTPServerResponse& resp)
        : request(req)
        , response(resp)
    {
        Poco::URI uri(request.getURI());
        std::vector<std::string> segments;
        uri.getPathSegments(segments);
        params = uri.getQueryParameters();

        // segments: ["api", "v2", "ObjectName", "path1", "path2", ...]
        if (segments.size() > 2)
            objectName = segments[2];

        if (segments.size() > 3)
            path.assign(segments.begin() + 3, segments.end());
    }
    // -------------------------------------------------------------------------
    std::string HttpRequestContext::pathString() const
    {
        std::ostringstream ss;
        for (size_t i = 0; i < path.size(); ++i)
        {
            if (i > 0)
                ss << '/';
            ss << path[i];
        }
        return ss.str();
    }

    // -------------------------------------------------------------------------
    // UHttpRequestHandler implementation
    // -------------------------------------------------------------------------
    UHttpRequestHandler::UHttpRequestHandler(std::shared_ptr<IHttpRequestRegistry> _registry
            , const std::string& allow
            , const std::string& contentType)
        : registry(_registry)
        , httpCORS_allow(allow)
        , httpDefaultContentType(contentType)
    {
        log = make_shared<DebugStream>();
    }
    // -------------------------------------------------------------------------
    void UHttpRequestHandler::handleRequest( Poco::Net::HTTPServerRequest& req
            , Poco::Net::HTTPServerResponse& resp )
    {
        // Поддерживаем GET и POST
        resp.set("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        resp.set("Access-Control-Allow-Headers", "Content-Type, Authorization");
        resp.set("Access-Control-Allow-Request-Method", "*");
        resp.set("Access-Control-Allow-Origin", httpCORS_allow);
        resp.setContentType(httpDefaultContentType);

        if( !registry )
        {
            resp.setStatus(HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
            std::ostream& out = resp.send();
            Poco::JSON::Object::Ptr jdata = new Poco::JSON::Object();
            jdata->set("error", resp.getReasonForStatus(resp.getStatus()));
            jdata->set("ecode", resp.getStatus());
            jdata->set("message", "Unknown 'registry of objects'");
            jdata->stringify(out);
            out.flush();
            return;
        }

        // Обработка preflight запросов (CORS)
        if( req.getMethod() == "OPTIONS" )
        {
            resp.setStatus(HTTPResponse::HTTP_OK);
            resp.send();
            return;
        }

        // Поддерживаем только GET и POST
        if( req.getMethod() != "GET" && req.getMethod() != "POST" )
        {
            resp.setStatus(HTTPResponse::HTTP_BAD_REQUEST);
            std::ostream& out = resp.send();
            Poco::JSON::Object jdata;
            jdata.set("error", resp.getReasonForStatus(resp.getStatus()));
            jdata.set("ecode", (int)resp.getStatus());
            jdata.set("message", "method must be 'GET' or 'POST'");
            jdata.stringify(out);
            out.flush();
            return;
        }

        // Создаём контекст один раз
        HttpRequestContext ctx(req, resp);

        if( log->is_info() )
        {
            Poco::URI uri(req.getURI());
            log->info() << req.getHost() << ": " << req.getMethod() << " " << uri.getPath()
                        << " query: " << uri.getQuery() << endl;
        }

        // Проверяем структуру URL: /api/v2/xxx
        Poco::URI uri(req.getURI());
        std::vector<std::string> seg;
        uri.getPathSegments(seg);

        if( seg.size() < 3
                || seg[0] != "api"
                || seg[1] != UHTTP_API_VERSION
                || seg[2].empty() )
        {
            resp.setStatus(HTTPResponse::HTTP_BAD_REQUEST);
            std::ostream& out = resp.send();
            Poco::JSON::Object jdata;
            jdata.set("error", resp.getReasonForStatus(resp.getStatus()));
            jdata.set("ecode", (int)resp.getStatus());
            jdata.set("message", "BAD REQUEST STRUCTURE. Expected: /api/" + UHTTP_API_VERSION + "/ObjectName[/path...]");
            jdata.stringify(out);
            out.flush();
            return;
        }

        resp.setStatus(HTTPResponse::HTTP_OK);

        try
        {
            // Системные команды
            if( ctx.objectName == "help" )
            {
                std::ostream& out = resp.send();
                out << "{ \"help\": ["
                    "{\"help\": {\"desc\": \"this help\"}},"
                    "{\"list\": {\"desc\": \"list of objects\"}},"
                    "{\"ObjectName\": {\"desc\": \"ObjectName information\"}},"
                    "{\"ObjectName/help\": {\"desc\": \"help for ObjectName\"}},"
                    "{\"ObjectName/path/to/cmd\": {\"desc\": \"nested path request\"}},"
                    "{\"apidocs\": {\"desc\": \"https://github.com/Etersoft/uniset2\"}}"
                    "]}";
                out.flush();
                return;
            }

            if( ctx.objectName == "list" )
            {
                auto json = registry->httpGetObjectsList(ctx);
                std::ostream& out = resp.send();
                json->stringify(out);
                out.flush();
                return;
            }

            // /api/v2/ObjectName/help
            if( ctx.depth() == 1 && ctx[0] == "help" )
            {
                auto json = registry->httpHelpRequest(ctx);
                std::ostream& out = resp.send();
                json->stringify(out);
                out.flush();
                return;
            }

            // Все остальные запросы — через registry->httpRequest()
            auto json = registry->httpRequest(ctx);
            std::ostream& out = resp.send();
            json->stringify(out);
            out.flush();
        }
        catch( std::exception& ex )
        {
            ostringstream err;
            err << ex.what();

            // Если статус ещё не был установлен handler'ом — ставим 500
            if( resp.getStatus() == HTTPResponse::HTTP_OK )
                resp.setStatus(HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);

            Poco::JSON::Object jdata;
            jdata.set("error", err.str());
            jdata.set("ecode", (int)resp.getStatus());
            std::ostream& out = resp.send();
            jdata.stringify(out);
            out.flush();
        }
    }

    // -------------------------------------------------------------------------
    // UHttpRequestHandlerFactory implementation
    // -------------------------------------------------------------------------
    UHttpRequestHandlerFactory::UHttpRequestHandlerFactory(std::shared_ptr<IHttpRequestRegistry>& _registry ):
        registry(_registry)
    {
    }
    // -------------------------------------------------------------------------
    HTTPRequestHandler* UHttpRequestHandlerFactory::createRequestHandler( const HTTPServerRequest& req )
    {
        return new UHttpRequestHandler(registry, httpCORS_allow, httpDefaultContentType);
    }
    // -------------------------------------------------------------------------
    void UHttpRequestHandlerFactory::setCORS_allow( const std::string& allow )
    {
        httpCORS_allow = allow;
    }
    // -------------------------------------------------------------------------
    void UHttpRequestHandlerFactory::setDefaultContentType( const std::string& ct )
    {
        httpDefaultContentType = ct;
    }
    // -------------------------------------------------------------------------
} // end of namespace uniset
// -------------------------------------------------------------------------
#endif
