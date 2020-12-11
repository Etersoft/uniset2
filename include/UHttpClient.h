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
#ifndef UHttpClient_H_
#define UHttpClient_H_
// -------------------------------------------------------------------------
#include <string>
#include <Poco/Net/HTTPClientSession.h>
#include "DebugStream.h"
#include "PassiveTimer.h"
// -------------------------------------------------------------------------
namespace uniset
{
    namespace UHttp
    {
        /*! Simple http client interface */
        class UHttpClient
        {
            public:

                UHttpClient();
                virtual ~UHttpClient();

                // request example: http://site.com/query?params
                // \return "" if fail
                std::string get( const std::string& host, int port, const std::string& request );

                void setTimeout( uniset::timeout_t usec );

                // in microseconds
                uniset::timeout_t getTimeout();

            protected:
                Poco::Net::HTTPClientSession session;

            private:
        };
    }
}
// -------------------------------------------------------------------------
#endif // UHttpClient_H_
// -------------------------------------------------------------------------
#endif
