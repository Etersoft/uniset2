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
                    attr(attrName), nsIndex(nsIdx)
                {
                    nodeId = UA_NODEID_STRING_ALLOC(nsIndex, attr.c_str());
                }
                Result32(int num, int nsIdx = 0 ):
                    attrNum(num), nsIndex(nsIdx)
                {
                    nodeId = UA_NODEID_NUMERIC(nsIndex, num);
                }

                UA_NodeId getNodeId()
                {
                    if( attrNum > 0 )
                        return UA_NODEID_NUMERIC(nsIndex, attrNum);

                    return UA_NODEID_STRING_ALLOC(nsIndex, attr.c_str());
                }

                void makeNodeId()
                {
                    nodeId = getNodeId();
                }

                int32_t value;
                std::string attr;
                int type = { UA_TYPES_INT32 };
                int attrNum = { 0 };
                int nsIndex;
                UA_NodeId nodeId;
            };

            using ErrorCode = int;

            ErrorCode read32( std::vector<Result32*>& attrs );
            ErrorCode read32( Result32& res );
            ErrorCode write32( const std::vector<Result32*>& attrs );
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
