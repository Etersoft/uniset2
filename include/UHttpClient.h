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
// -------------------------------------------------------------------------
namespace uniset
{
    namespace UHttp
    {
        class UHttpClient
        {
            public:

                UHttpClient();
                virtual ~UHttpClient();

                // http://site.com/query?params
                // \return ""
                std::string get( const std::string& host, int port, const std::string& request );

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
