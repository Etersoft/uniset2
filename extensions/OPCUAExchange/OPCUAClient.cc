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
// -------------------------------------------------------------------------
#include <sstream>
#include <open62541/client_highlevel.h>
#include <open62541/client_subscriptions.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/types_generated_handling.h>
#include "OPCUAClient.h"
// -----------------------------------------------------------------------------
using namespace uniset;
using namespace std;
// -----------------------------------------------------------------------------
OPCUAClient::OPCUAClient()
{
    ss = UA_SESSIONSTATE_CLOSED;
    client = UA_Client_new();
    auto conf = UA_Client_getConfig(client);
    conf->logger = UA_Log_Stdout_withLevel( UA_LOGLEVEL_ERROR );
    UA_ClientConfig_setDefault(conf);

    val = UA_Variant_new();
}
// -----------------------------------------------------------------------------
OPCUAClient::~OPCUAClient()
{
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    UA_Variant_delete(val);
}
// -----------------------------------------------------------------------------
bool OPCUAClient::connect( const std::string& addr )
{
    if( client == nullptr )
        return false;

    UA_Client_getState(client, NULL, &ss, NULL);

    if( ss ==  UA_SESSIONSTATE_ACTIVATED )
        return true;

    return UA_Client_connect(client, addr.c_str()) == UA_STATUSCODE_GOOD;
}

// -----------------------------------------------------------------------------
bool OPCUAClient::connect( const std::string& addr, const std::string& user, const std::string& pass )
{
    if( client == nullptr )
        return false;

    UA_Client_getState(client, NULL, &ss, NULL);

    if( ss ==  UA_SESSIONSTATE_ACTIVATED )
        return true;

    return UA_Client_connectUsername(client, addr.c_str(), user.c_str(), pass.c_str()) == UA_STATUSCODE_GOOD;
}
// -----------------------------------------------------------------------------
OPCUAClient::ErrorCode OPCUAClient::read32( std::vector<UA_ReadValueId>& attrs, std::vector<Result32>& result )
{
    UA_ReadRequest request;
    UA_ReadRequest_init(&request);
    request.nodesToRead = attrs.data();
    request.nodesToReadSize = attrs.size();

    UA_ReadResponse response = UA_Client_Service_read(client, request);
    auto retval = response.responseHeader.serviceResult;

    if( retval == UA_STATUSCODE_GOOD )
    {
        if( response.resultsSize > result.size() )
            retval = UA_STATUSCODE_BADRESPONSETOOLARGE;
        else
        {
            for( size_t i = 0; i < response.resultsSize; i++ )
            {
                result[i].status = response.results[i].status;

                if (response.results[i].status == UA_STATUSCODE_GOOD)
                {
                    UA_Variant* val = &response.results[i].value;

                    if (val->type == &UA_TYPES[UA_TYPES_INT32])
                        result[i].value = *(UA_Int32*) val->data;
                    else if (val->type == &UA_TYPES[UA_TYPES_UINT32])
                        result[i].value = int32_t(*(UA_UInt32*) val->data);
                    else if (val->type == &UA_TYPES[UA_TYPES_INT64])
                        result[i].value = int32_t(*(UA_Int64*) val->data);
                    else if (val->type == &UA_TYPES[UA_TYPES_UINT64])
                        result[i].value = int32_t(*(UA_UInt64*) val->data);
                    else if (val->type == &UA_TYPES[UA_TYPES_BOOLEAN])
                        result[i].value = *(UA_Boolean*) val->data ? 1 : 0;
                    else if (val->type == &UA_TYPES[UA_TYPES_UINT16])
                        result[i].value = *(UA_UInt16*) val->data;
                    else if (val->type == &UA_TYPES[UA_TYPES_INT16])
                        result[i].value = *(UA_Int16*) val->data;
                    else if (val->type == &UA_TYPES[UA_TYPES_BYTE])
                        result[i].value = *(UA_Byte*) val->data;
                }
            }
        }
    }

    UA_ReadResponse_clear(&response);
    return (OPCUAClient::ErrorCode)retval;
}
// -----------------------------------------------------------------------------
OPCUAClient::ErrorCode OPCUAClient::write32( std::vector<UA_WriteValue>& values )
{
    UA_WriteRequest request;
    UA_WriteRequest_init(&request);
    request.nodesToWrite = values.data();
    request.nodesToWriteSize = values.size();
    UA_WriteResponse response = UA_Client_Service_write(client, request);

    auto retval = response.responseHeader.serviceResult;

    for( size_t i = 0; i < response.resultsSize; i++ )
    {
        values[i].value.status = response.results[i];

        if( response.results[i] != UA_STATUSCODE_GOOD )
            retval = response.results[i];
    }

    UA_WriteResponse_clear(&response);
    return (OPCUAClient::ErrorCode)retval;
}
// -----------------------------------------------------------------------------
OPCUAClient::ErrorCode OPCUAClient::write( const UA_WriteValue& val )
{
    UA_WriteRequest request;
    UA_WriteRequest_init(&request);
    UA_WriteValue wrval[1] = { val };
    request.nodesToWrite = wrval;
    request.nodesToWriteSize = 1;
    UA_WriteResponse response = UA_Client_Service_write(client, request);

    auto retval = response.responseHeader.serviceResult;

    for( size_t i = 0; i < response.resultsSize; i++ )
    {
        if( response.results[i] != UA_STATUSCODE_GOOD )
            retval = response.results[i];
    }

    UA_WriteResponse_clear(&response);
    return (OPCUAClient::ErrorCode)retval;
}
// -----------------------------------------------------------------------------
OPCUAClient::ErrorCode OPCUAClient::set(const std::string& attr, bool set, int nsIndex)
{
    UA_Variant_clear(val);
    UA_Variant_setScalarCopy(val, &set, &UA_TYPES[UA_TYPES_BOOLEAN]);
    return UA_Client_writeValueAttribute(client, UA_NODEID_STRING(nsIndex, const_cast<char*>(attr.c_str())), val);
}
// -----------------------------------------------------------------------------
OPCUAClient::ErrorCode OPCUAClient::write32(const std::string& attr, int32_t value, int nsIndex)
{
    UA_Variant_clear(val);
    UA_Variant_setScalarCopy(val, &value, &UA_TYPES[UA_TYPES_INT32]);
    return UA_Client_writeValueAttribute(client, UA_NODEID_STRING_ALLOC(nsIndex, const_cast<char*>(attr.c_str())), val);
}
// -----------------------------------------------------------------------------
UA_WriteValue OPCUAClient::makeWriteValue32( const std::string& name, int32_t val, int nsIndex )
{
    UA_WriteValue wv;
    UA_WriteValue_init(&wv);
    wv.attributeId = UA_ATTRIBUTEID_VALUE;
    std::string attr = name;

    if( name.size() > 2 && name[1] != '=')
        attr = "s=" + name;

    if( nsIndex > 0 )
        attr = "ns=" + std::to_string(nsIndex) + ";" + attr;

    wv.nodeId = UA_NODEID(attr.c_str());
    UA_Variant_setScalarCopy(&wv.value.value, &val, &UA_TYPES[UA_TYPES_INT32]);
    return wv;
}
// -----------------------------------------------------------------------------
UA_ReadValueId OPCUAClient::makeReadValue32( const std::string& name, int nsIndex )
{
    UA_ReadValueId rv;
    UA_ReadValueId_init(&rv);
    rv.attributeId = UA_ATTRIBUTEID_VALUE;
    std::string attr = name;

    if( name.size() > 2 && name[1] != '=' )
        attr = "s=" + name;

    if( nsIndex > 0 )
        attr = "ns=" + std::to_string(nsIndex) + ";" + attr;

    rv.nodeId = UA_NODEID(attr.c_str());
    return rv;
}
// -----------------------------------------------------------------------------