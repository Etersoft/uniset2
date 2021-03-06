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
// --------------------------------------------------------------------------
/*! \file
 *  \author Pavel Vainerman
*/
// --------------------------------------------------------------------------
#ifndef HttpResolver_H_
#define HttpResolver_H_
// --------------------------------------------------------------------------
#include <Poco/JSON/Object.h>
#include "UniSetTypes.h"
#include "DebugStream.h"
#include "UHttpRequestHandler.h"
#include "UHttpServer.h"
#include "IORFile.h"
// -------------------------------------------------------------------------
const std::string HTTP_RESOLVER_API_VERSION = "v01";
// -------------------------------------------------------------------------
namespace uniset
{
    //------------------------------------------------------------------------------------------
    /*!
        \page page_HttpResolver Http-cервис для получения CORBA-ссылок на объекты (HttpResolver)

        - \ref sec_HttpResolver_Comm
        - \ref sec_HttpResolver_Conf

        \section sec_HttpResolver_Comm Общее описание работы HttpResolver
            HttpResolver это сервис, который отдаёт CORBA-ссылки (в виде строки)
        на объекты запущенные на данном узле в режиме LocalIOR. Т.е. когда ссылки
        публикуются в файлах.

        \sa \ref ConfigurationPage_secLocalIOR

        \section sec_HttpResolver_Conf Настройка работы HttpResolver
        Для запуска HttpResolver необходимо настроить на каком порту и сетевом интерфейсе будут приниматься запросы.
        Для этого необходимо в настройках прописать следующую секцию
        \code
        <UniSet>
          ...
          <HttpResolver name="HttpResolver" port="8008" host="0.0.0.0"/>
          ...
        </UniSet>
        \endcode
        - 0.0.0.0 - слушать на всех доступных интерфейсах

        Помимо этого, настройки можно указать в аргументах командной строки
        - `--httpresolver-host` - интерфейс на котором слушать запросы. По умолчанию: 0.0.0.0
        - `--httpresolver-port` - порт на котором слушать запросы. По умолчанию: 8008

        Длполнительные параметры:
        - `--httpresolver-max-queued num `     - Размер очереди запросов к http серверу. По умолчанию: 100.
        - `--httpresolver-max-threads num`     - Разрешённое количество потоков для http-сервера. По умолчанию: 3.
        - `--httpresolver-cors-allow addr`     - (CORS): Access-Control-Allow-Origin. Default: *
        - `--httpresolver-logs-add-levels any` - Управление логами
        - `--confile configure.xml`            - Файл с настройками
    */
    class HttpResolver:
        public Poco::Net::HTTPRequestHandler
    {
        public:
            HttpResolver( const std::string& name, int argc, const char* const* argv, const std::string& prefix );
            virtual ~HttpResolver();

            /*! глобальная функция для инициализации объекта */
            static std::shared_ptr<HttpResolver> init_resolver( int argc, const char* const* argv, const std::string& prefix = "httpresolver-" );

            /*! глобальная функция для вывода help-а */
            static void help_print();

            inline std::shared_ptr<DebugStream> log()
            {
                return rlog;
            }

            virtual void handleRequest( Poco::Net::HTTPServerRequest& req, Poco::Net::HTTPServerResponse& resp ) override;

            void run();

        protected:

            Poco::JSON::Object::Ptr respError( Poco::Net::HTTPServerResponse& resp, Poco::Net::HTTPResponse::HTTPStatus s, const std::string& message );
            Poco::JSON::Object::Ptr httpGetRequest( const std::string& cmd, const Poco::URI::QueryParameters& p );
            Poco::JSON::Object::Ptr httpJsonResolve( const std::string& query, const Poco::URI::QueryParameters& p );
            std::string httpTextResolve( const std::string& query, const Poco::URI::QueryParameters& p );

            std::shared_ptr<Poco::Net::HTTPServer> httpserv;
            std::string httpHost = { "" };
            int httpPort = { 8008 };
            std::string httpCORS_allow = { "*" };
            std::string httpReplyAddr = { "" };

            std::shared_ptr<DebugStream> rlog;
            std::string myname;

            std::shared_ptr<IORFile> iorfile;

            class HttpResolverRequestHandlerFactory:
                public Poco::Net::HTTPRequestHandlerFactory
            {
                public:
                    HttpResolverRequestHandlerFactory( HttpResolver* r ): resolver(r) {}
                    virtual ~HttpResolverRequestHandlerFactory() {}

                    virtual Poco::Net::HTTPRequestHandler* createRequestHandler( const Poco::Net::HTTPServerRequest& req ) override;

                private:
                    HttpResolver* resolver;
            };

        private:
    };
    // ----------------------------------------------------------------------------------
} // end of namespace uniset
//------------------------------------------------------------------------------------------
#endif
