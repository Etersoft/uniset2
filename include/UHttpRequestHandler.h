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
#ifndef UHttpRequesrHandler_H_
#define UHttpRequesrHandler_H_
// -------------------------------------------------------------------------
#include <memory>
#include <vector>
#include <string>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/URI.h>
#include <Poco/JSON/Object.h>
#include "ujson.h"
#include "DebugStream.h"
// -------------------------------------------------------------------------
/*! \page pg_UHttpServer HTTP API v2
 *
 * Формат запроса: /api/v2/ObjectName[/path...]
 *
 * <br>Поддерживаются методы GET и POST
 * <br>Ответ в формате JSON
 * <br>Поддерживаются вложенные пути: /api/v2/ObjectName/sensors/count
 *
 * Версия API: \b v2
 * - /api/v2/list                     - Получение списка доступных объектов
 * - /api/v2/help                     - Получение списка доступных команд
 * - /api/v2/ObjectName               - получение информации об объекте ObjectName
 * - /api/v2/ObjectName/help          - получение списка доступных команд для объекта ObjectName
 * - /api/v2/ObjectName/path/to/cmd   - запрос к объекту ObjectName с вложенным путём
 *
 * Авторизация:
 * - Header: Authorization: Bearer <token>
 *
 *\code
 *  HELP FORMAT:
 *  myname {
 *      help [
 *          {
 *             "name": "command1",
 *             "desc": "text",
 *             "parameters": [
 *                  {
 *                   "name": "p1",
 *                   "desc": " description of p1"
 *                  }
 *             ]}
 *          }
 *      ]
 *  }
 *\endcode
 *
 * \sa \ref act_HttpAPI
*/
// -------------------------------------------------------------------------
namespace uniset
{
    namespace UHttp
    {
        // текущая версия API
        const std::string UHTTP_API_VERSION = "v2";

        // -------------------------------------------------------------------------
        /*! Контекст HTTP запроса.
         *  Простая data-структура с public полями.
         *  Создаётся один раз в handleRequest и передаётся во все handlers.
         */
        struct HttpRequestContext
        {
            // Ссылки на Poco объекты (всегда существуют)
            Poco::Net::HTTPServerRequest& request;
            Poco::Net::HTTPServerResponse& response;

            // Распарсенные данные (заполняются в конструкторе один раз)
            std::vector<std::string> path;      // путь после ObjectName: ["sensors", "count"]
            Poco::URI::QueryParameters params;  // query string параметры
            std::string objectName;             // имя объекта из URL

            // Конструктор — парсит URI один раз
            HttpRequestContext(Poco::Net::HTTPServerRequest& req,
                               Poco::Net::HTTPServerResponse& resp);

            // Минимум хелперов
            size_t depth() const { return path.size(); }

            const std::string& operator[](size_t i) const
            {
                static const std::string empty;
                return (i < path.size()) ? path[i] : empty;
            }

            // Для логов и сообщений об ошибках
            std::string pathString() const;
        };

        // -------------------------------------------------------------------------
        /*! Интерфейс для объекта, обрабатывающего HTTP запросы */
        class IHttpRequest
        {
            public:
                IHttpRequest() {}
                virtual ~IHttpRequest() {}

                // Основной метод обработки запросов
                // throw SystemError
                virtual Poco::JSON::Object::Ptr httpRequest(const HttpRequestContext& ctx) = 0;

                // Справка по командам объекта (отдельно, чтобы не забыть реализовать)
                // throw SystemError
                virtual Poco::JSON::Object::Ptr httpHelp(const Poco::URI::QueryParameters& p) = 0;
        };

        // -------------------------------------------------------------------------
        /*! Интерфейс реестра объектов (маршрутизация запросов) */
        class IHttpRequestRegistry
        {
            public:
                IHttpRequestRegistry() {}
                virtual ~IHttpRequestRegistry() {}

                // Основной метод — берёт objectName из ctx
                // throw SystemError, NameNotFound
                virtual Poco::JSON::Object::Ptr httpRequest(const HttpRequestContext& ctx) = 0;

                // Список объектов
                // throw SystemError
                virtual Poco::JSON::Array::Ptr httpGetObjectsList(const HttpRequestContext& ctx) = 0;

                // Справка по объекту (диспетчеризация к конкретному объекту по ctx.objectName)
                // throw SystemError, NameNotFound
                virtual Poco::JSON::Object::Ptr httpHelpRequest(const HttpRequestContext& ctx) = 0;
        };

        // -------------------------------------------------------------------------
        class UHttpRequestHandler:
            public Poco::Net::HTTPRequestHandler
        {
            public:
                UHttpRequestHandler( std::shared_ptr<IHttpRequestRegistry> _registry, const std::string& httpCORS_allow = "*",
                                                                        const std::string& contentType="text/json; charset=UTF-8");

                virtual void handleRequest( Poco::Net::HTTPServerRequest& req, Poco::Net::HTTPServerResponse& resp ) override;

            private:

                std::shared_ptr<IHttpRequestRegistry> registry;
                std::shared_ptr<DebugStream> log;
                const std::string httpCORS_allow = { "*" };
                std::string httpDefaultContentType = {"text/json; charset=UTF-8" };
        };
        // -------------------------------------------------------------------------
        class UHttpRequestHandlerFactory:
            public Poco::Net::HTTPRequestHandlerFactory
        {
            public:

                UHttpRequestHandlerFactory( std::shared_ptr<IHttpRequestRegistry>& _registry );

                virtual Poco::Net::HTTPRequestHandler* createRequestHandler( const Poco::Net::HTTPServerRequest& ) override;

                // (CORS): Access-Control-Allow-Origin. Default: *
                void setCORS_allow( const std::string& allow );
                void setDefaultContentType( const std::string& ct );
            private:
                std::shared_ptr<IHttpRequestRegistry> registry;
                std::string httpCORS_allow = { "*" };
                std::string httpDefaultContentType = {"text/json; charset=UTF-8" };
        };
    }
    // -------------------------------------------------------------------------
} // end of uniset namespace
// -------------------------------------------------------------------------
#endif // UHttpRequesrHandler_H_
// -------------------------------------------------------------------------
#endif
