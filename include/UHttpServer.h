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
#include <vector>
#include <Poco/Net/HTTPServer.h>
#include "DebugStream.h"
#include "ThreadCreator.h"
#include "UHttpRequestHandler.h"
// -------------------------------------------------------------------------
/*! \page pgUHttpServer Http сервер
    Http сервер предназначен для получения информации о UniSetObject-ах через http (json).
    \n\n
    Примеры задания разрешений/запретов по IP для CORS/ACL:
    \code
    auto srv = std::make_shared<uniset::UHttp::UHttpServer>(registry, "0.0.0.0", 8080);

    // CIDR-подсети
    srv->setWhitelist({"192.168.1.0/24", "10.0.0.0/8"});

    // Конкретный адрес (префикс = длина адреса)
    srv->setBlacklist({"192.168.1.100"});

    // Диапазон адресов, начало и конец через '-'
    srv->setBlacklist({"172.16.0.10-172.16.0.20"});

    // Доверенные фронты, от которых читаем X-Forwarded-For/X-Real-IP
    srv->setTrustedProxies({"127.0.0.1", "10.0.0.0/24"});

    // Bearer-аутентификация: включить и задать список токенов
    srv->setBearerRequired(true);
    srv->setBearerTokens({"token123", "another-token"});
    \endcode
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

                // (CORS): Access-Control-Allow-Origin. Default: *
                void setCORS_allow( const std::string& CORS_allow );
                void setDefaultContentType( const std::string& ct);
                void setWhitelist( const std::vector<std::string>& wl );
                void setBlacklist( const std::vector<std::string>& bl );
                // Доверенные прокси, от которых принимаем X-Forwarded-For/X-Real-IP
                void setTrustedProxies( const std::vector<std::string>& proxies );
                // Bearer auth: включение и список допустимых токенов
                void setBearerRequired( bool required );
                void setBearerTokens( const std::vector<std::string>& tokens );
            protected:
                UHttpServer();

            private:

                std::shared_ptr<DebugStream> mylog;
                Poco::Net::SocketAddress sa;

                std::shared_ptr<Poco::Net::HTTPServer> http;
                std::shared_ptr<UHttpRequestHandlerFactory> reqFactory;
                UHttp::NetworkRules whitelist;
                UHttp::NetworkRules blacklist;
                UHttp::NetworkRules trustedProxies;
                UHttp::BearerTokens bearerTokens;
                bool bearerRequired = { false };

        };
    }
}
// -------------------------------------------------------------------------
#endif // UHttpServer_H_
// -------------------------------------------------------------------------
#endif
