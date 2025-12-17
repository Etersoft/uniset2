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
#include <cstring>
#include <stdexcept>
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
    namespace
    {
        NetworkRule makeRule(const std::string& cidr)
        {
            NetworkRule rule;

            auto rangePos = cidr.find('-');
            if( rangePos != std::string::npos )
            {
                const std::string startStr = cidr.substr(0, rangePos);
                const std::string endStr = cidr.substr(rangePos + 1);

                rule.address = Poco::Net::IPAddress(startStr);
                rule.rangeEnd = Poco::Net::IPAddress(endStr);
                rule.isRange = true;

                if( rule.address.family() != rule.rangeEnd.family() )
                    throw std::invalid_argument("range family mismatch");

                const int len = rule.address.length();
                if( std::memcmp(rule.address.addr(), rule.rangeEnd.addr(), len) > 0 )
                    throw std::invalid_argument("range start greater than end");

                rule.prefixLength = 0;
                return rule;
            }

            auto pos = cidr.find('/');
            if( pos == std::string::npos )
            {
                rule.address = Poco::Net::IPAddress(cidr);
                rule.prefixLength = rule.address.length() * 8;
                return rule;
            }

            rule.address = Poco::Net::IPAddress(cidr.substr(0, pos));
            rule.prefixLength = static_cast<unsigned int>(std::stoul(cidr.substr(pos + 1)));

            const unsigned int maxPrefix = rule.address.length() * 8;
            if( rule.prefixLength > maxPrefix )
                throw std::invalid_argument("prefix is greater than address length");

            rule.isRange = false;
            return rule;
        }

        NetworkRules buildRules(const std::vector<std::string>& values, std::shared_ptr<DebugStream> log)
        {
            NetworkRules res;

            for( const auto& v: values )
            {
                if( v.empty() )
                    continue;

                auto first = v.find_first_not_of(" \t");
                auto last = v.find_last_not_of(" \t");

                if( first == std::string::npos )
                    continue;

                const std::string trimmed = v.substr(first, last - first + 1);

                try
                {
                    res.push_back(makeRule(trimmed));
                }
                catch( std::exception& ex )
                {
                    if( log && log->is_warn() )
                        log->warn() << "(UHttpServer::buildRules): skip '" << v << "': " << ex.what() << std::endl;
                }
            }

            return res;
        }

        BearerTokens buildTokens(const std::vector<std::string>& values)
        {
            BearerTokens res;

            for( const auto& v : values )
            {
                if( v.empty() )
                    continue;

                auto first = v.find_first_not_of(" \t");
                auto last = v.find_last_not_of(" \t");

                if( first == std::string::npos )
                    continue;

                const std::string trimmed = v.substr(first, last - first + 1);
                if( !trimmed.empty() )
                    res.insert(trimmed);
            }

            return res;
        }
    }
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
	void UHttpServer::setDefaultContentType( const std::string& ct)
	{
		reqFactory->setDefaultContentType(ct);
	}
	// -------------------------------------------------------------------------
	void UHttpServer::setWhitelist( const std::vector<std::string>& wl )
	{
		whitelist = buildRules(wl, mylog);
		reqFactory->setWhitelist(whitelist);
	}
	// -------------------------------------------------------------------------
	void UHttpServer::setBlacklist( const std::vector<std::string>& bl )
	{
		blacklist = buildRules(bl, mylog);
		reqFactory->setBlacklist(blacklist);
	}
	// -------------------------------------------------------------------------
	void UHttpServer::setTrustedProxies( const std::vector<std::string>& proxies )
	{
		trustedProxies = buildRules(proxies, mylog);
		reqFactory->setTrustedProxies(trustedProxies);
	}
	// -------------------------------------------------------------------------
	void UHttpServer::setBearerRequired( bool required )
	{
		bearerRequired = required;
		reqFactory->setBearerRequired(required);
	}
	// -------------------------------------------------------------------------
	void UHttpServer::setBearerTokens( const std::vector<std::string>& tokens )
	{
		bearerTokens = buildTokens(tokens);
		reqFactory->setBearerTokens(bearerTokens);
	}
	// -------------------------------------------------------------------------
} // end of namespace uniset
// -------------------------------------------------------------------------
#endif // #ifndef DISABLE_REST_API
