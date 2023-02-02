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
#include "OPCUAClient.h"
// -----------------------------------------------------------------------------
using namespace uniset;
using namespace std;
// -----------------------------------------------------------------------------
OPCUAClient::OPCUAClient()
{
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
OPCUAClient::ErrorCode  OPCUAClient::read32( Result32& res )
{
    UA_Variant_clear(val);
    auto retval = UA_Client_readValueAttribute(client, UA_NODEID_STRING(res.nsIndex, const_cast<char*>(res.attr.c_str())), val);

    if( retval == UA_STATUSCODE_GOOD && UA_Variant_isScalar(val) )
    {
        if( val->type == &UA_TYPES[UA_TYPES_INT32] )
            res.value = *(UA_Int32*)val->data;
        else if( val->type == &UA_TYPES[UA_TYPES_UINT32] )
            res.value = int32_t(*(UA_UInt32*)val->data);
        else if( val->type == &UA_TYPES[UA_TYPES_INT64] )
            res.value = int32_t(*(UA_Int64*)val->data);
        else if( val->type == &UA_TYPES[UA_TYPES_UINT64] )
            res.value = int32_t(*(UA_UInt64*)val->data);
        else if( val->type == &UA_TYPES[UA_TYPES_BOOLEAN] )
            res.value = *(UA_Boolean*)val->data ? 1 : 0;
        else if( val->type == &UA_TYPES[UA_TYPES_UINT16] )
            res.value = *(UA_UInt16*)val->data;
        else if( val->type == &UA_TYPES[UA_TYPES_INT16] )
            res.value =  *(UA_Int16*)val->data;
        else if( val->type == &UA_TYPES[UA_TYPES_BYTE] )
            res.value = *(UA_Byte*)val->data;
    }

    return (OPCUAClient::ErrorCode)retval;
}
// -----------------------------------------------------------------------------
OPCUAClient::ErrorCode OPCUAClient::read32( std::vector<Result32*>& attrs )
{
    UA_ReadRequest request;
    UA_ReadRequest_init(&request);
    UA_ReadValueId ids[attrs.size()];
    int i = 0;

    for( const auto& v : attrs )
    {
        UA_ReadValueId_init(&ids[i]);
        ids[i].attributeId = UA_ATTRIBUTEID_VALUE;
        ids[i].nodeId = v->nodeId;
        i++;
    }

    request.nodesToRead = ids;
    request.nodesToReadSize = attrs.size();

    UA_ReadResponse response = UA_Client_Service_read(client, request);
    auto retval = response.responseHeader.serviceResult;

    if( retval == UA_STATUSCODE_GOOD )
    {
        for( i = 0; i < response.resultsSize; i++ )
        {
            if( response.results[i].status == UA_STATUSCODE_GOOD )
            {
                UA_Variant* val = &response.results[i].value;

                if( val->type == &UA_TYPES[UA_TYPES_INT32] )
                    attrs[i]->value = *(UA_Int32*)val->data;
                else if( val->type == &UA_TYPES[UA_TYPES_UINT32] )
                    attrs[i]->value = int32_t(*(UA_UInt32*)val->data);
                else if( val->type == &UA_TYPES[UA_TYPES_INT64] )
                    attrs[i]->value = int32_t(*(UA_Int64*)val->data);
                else if( val->type == &UA_TYPES[UA_TYPES_UINT64] )
                    attrs[i]->value = int32_t(*(UA_UInt64*)val->data);
                else if( val->type == &UA_TYPES[UA_TYPES_BOOLEAN] )
                    attrs[i]->value = *(UA_Boolean*)val->data ? 1 : 0;
                else if( val->type == &UA_TYPES[UA_TYPES_UINT16] )
                    attrs[i]->value = *(UA_UInt16*)val->data;
                else if( val->type == &UA_TYPES[UA_TYPES_INT16] )
                    attrs[i]->value =  *(UA_Int16*)val->data;
                else if( val->type == &UA_TYPES[UA_TYPES_BYTE] )
                    attrs[i]->value = *(UA_Byte*)val->data;
            }
        }
    }

    UA_ReadResponse_clear(&response);
    return (OPCUAClient::ErrorCode)retval;
}
// -----------------------------------------------------------------------------
OPCUAClient::ErrorCode OPCUAClient::write32( const std::vector<Result32*>& attrs )
{
    UA_WriteRequest request;
    UA_WriteRequest_init(&request);
    UA_WriteValue ids[attrs.size()];
    int i = 0;

    for( const auto& v : attrs )
    {
        UA_WriteValue_init(&ids[i]);
        ids[i].attributeId = UA_ATTRIBUTEID_VALUE;
        ids[i].value.hasValue = true;
        ids[i].nodeId = v->nodeId;

        if( v->type == UA_TYPES_BOOLEAN )
        {
            bool set = v->value != 0;
            UA_Variant_setScalar(&ids[i].value.value, &set, &UA_TYPES[v->type]);
        }
        else
        {
            UA_Variant_setScalar(&ids[i].value.value, &v->value, &UA_TYPES[v->type]);
        }

        i++;
    }

    request.nodesToWrite = ids;
    request.nodesToWriteSize = attrs.size();

    UA_WriteResponse response = UA_Client_Service_write(client, request);
    auto retval = response.responseHeader.serviceResult;
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
    return UA_Client_writeValueAttribute(client, UA_NODEID_STRING(nsIndex, const_cast<char*>(attr.c_str())), val);
}
// -----------------------------------------------------------------------------
OPCUAClient::ErrorCode OPCUAClient::write64(const std::string& attr, int64_t value, int nsIndex)
{
    UA_Variant_clear(val);
    UA_Variant_setScalarCopy(val, &value, &UA_TYPES[UA_TYPES_INT64]);
    return UA_Client_writeValueAttribute(client, UA_NODEID_STRING(nsIndex, const_cast<char*>(attr.c_str())), val);
}
// -----------------------------------------------------------------------------