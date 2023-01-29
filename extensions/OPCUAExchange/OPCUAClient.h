/*
 * Copyright (c) 2023 Pavel Vainerman.
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
// -----------------------------------------------------------------------------
#ifndef OPCUAClient_H_
#define OPCUAClient_H_
// -----------------------------------------------------------------------------
#include <string>
#include <vector>
#include <open62541/client_config_default.h>
#include "Exceptions.h"
//--------------------------------------------------------------------------
namespace uniset
{
    // -----------------------------------------------------------------------------
    /*! Интерфейс для работы с OPC UA */
    class OPCUAClient
    {
        public:
            OPCUAClient();
            virtual ~OPCUAClient();

            bool connect( const std::string& addr );
            bool connect( const std::string& addr, const std::string& user, const std::string& pass );

            struct Result32
            {
                Result32() {}
                Result32(const std::string& attrName, int nsIdx = 0 ):
                    attr(attrName), nsIndex(nsIdx) {}

                int32_t value;
                std::string attr;
                int attrNum = { 0 };
                int nsIndex;
            };

            using ErrorCode = int;

            ErrorCode read( std::vector<Result32*>& attrs );
            ErrorCode read( Result32& res );
            ErrorCode write32( std::vector<Result32*>& attrs );
            ErrorCode write32( const std::string& attr, int32_t value, int nsIndex = 0 );
            ErrorCode write64( const std::string& attr, int64_t value, int nsIndex = 0 );
            ErrorCode set( const std::string& attr, bool set, int nsIndex = 0 );

        protected:
            UA_Client* client = { nullptr };
            UA_Variant* val = { nullptr };
            UA_SessionState ss;

        private:
    };
    // --------------------------------------------------------------------------
} // end of namespace uniset
// -----------------------------------------------------------------------------
#endif // OPCUAClient_H_
// -----------------------------------------------------------------------------
