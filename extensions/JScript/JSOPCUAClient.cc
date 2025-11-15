/*
 * Copyright (c) 2025 Pavel Vainerman.
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
#include "JSOPCUAClient.h"
#include "Exceptions.h"
// --------------------------------------------------------------------------
using namespace uniset;

JSOPCUAClient::JSOPCUAClient() = default;

JSOPCUAClient::~JSOPCUAClient()
{
    disconnect();
}

void JSOPCUAClient::setLog(const std::shared_ptr<DebugStream>& l)
{
    log = l;
}

bool JSOPCUAClient::connect(const std::string& endpoint)
{
    if( endpoint.empty() )
        throw SystemError("JSOPCUAClient: endpoint URL is empty");

    UA_SessionState session{};
    UA_Client_getState(client.handle(), nullptr, &session, nullptr);

    if( session == UA_SESSIONSTATE_ACTIVATED )
    {
        connected = true;
        return true;
    }

    try
    {
        client.connect(endpoint.c_str());
    }
    catch( const std::exception& ex )
    {
        logError(std::string("connect(") + endpoint + "): " + ex.what());
        throw SystemError("JSOPCUAClient: connect failed");
    }

    UA_Client_getState(client.handle(), nullptr, &session, nullptr);
    connected = (session == UA_SESSIONSTATE_ACTIVATED);

    if( !connected )
        throw SystemError("JSOPCUAClient: connect failed");

    return connected;
}

bool JSOPCUAClient::connect(const std::string& endpoint, const std::string& user, const std::string& passwd)
{
    if( endpoint.empty() )
        throw SystemError("JSOPCUAClient: endpoint URL is empty");

    UA_SessionState session{};
    UA_Client_getState(client.handle(), nullptr, &session, nullptr);

    if( session == UA_SESSIONSTATE_ACTIVATED )
    {
        connected = true;
        return true;
    }

    try
    {
        client.connect(endpoint, {user, passwd});
    }
    catch( const std::exception& ex )
    {
        logError(std::string("connect(") + endpoint + "): " + ex.what());
        throw SystemError("JSOPCUAClient: connect failed");
    }

    UA_Client_getState(client.handle(), nullptr, &session, nullptr);
    connected = (session == UA_SESSIONSTATE_ACTIVATED);

    if( !connected )
        throw SystemError("JSOPCUAClient: connect failed");

    return connected;
}

void JSOPCUAClient::disconnect() noexcept
{
    client.disconnect();
    connected = false;
}

bool JSOPCUAClient::isConnected() const noexcept
{
    return connected;
}

std::vector<JSOPCUAClient::ResultVar> JSOPCUAClient::read(const std::vector<std::string>& nodeIds)
{
    if( nodeIds.empty() )
        return {};

    if( !isConnected() )
        throw SystemError("JSOPCUAClient: not connected");

    std::vector<UA_ReadValueId> attrs;
    attrs.reserve(nodeIds.size());

    for( const auto& id : nodeIds )
        attrs.emplace_back(makeReadValue(id));

    std::vector<ResultVar> result(nodeIds.size());

    UA_ReadRequest request;
    UA_ReadRequest_init(&request);
    request.nodesToRead = attrs.data();
    request.nodesToReadSize = attrs.size();

    UA_ReadResponse response = UA_Client_Service_read(client.handle(), request);
    auto retval = response.responseHeader.serviceResult;

    if( retval == UA_STATUSCODE_GOOD )
    {
        for( size_t i = 0; i < response.resultsSize && i < result.size(); ++i )
        {
            result[i].status = response.results[i].status;
            result[i].value = 0;
            result[i].type = VarType::Int32;

            if( response.results[i].status == UA_STATUSCODE_GOOD )
            {
                UA_Variant* val = &response.results[i].value;

                if( val->type == &UA_TYPES[UA_TYPES_INT32] )
                    result[i].value = int32_t(*(UA_Int32*)val->data);
                else if( val->type == &UA_TYPES[UA_TYPES_UINT32] )
                    result[i].value = int32_t(*(UA_UInt32*)val->data);
                else if( val->type == &UA_TYPES[UA_TYPES_INT64] )
                    result[i].value = int32_t(*(UA_Int64*)val->data);
                else if( val->type == &UA_TYPES[UA_TYPES_UINT64] )
                    result[i].value = int32_t(*(UA_UInt64*)val->data);
                else if( val->type == &UA_TYPES[UA_TYPES_BOOLEAN] )
                    result[i].value = (*(UA_Boolean*)val->data) ? 1 : 0;
                else if( val->type == &UA_TYPES[UA_TYPES_UINT16] )
                    result[i].value = int32_t(*(UA_UInt16*)val->data);
                else if( val->type == &UA_TYPES[UA_TYPES_INT16] )
                    result[i].value = int32_t(*(UA_Int16*)val->data);
                else if( val->type == &UA_TYPES[UA_TYPES_BYTE] )
                    result[i].value = int32_t(*(UA_Byte*)val->data);
                else if( val->type == &UA_TYPES[UA_TYPES_FLOAT] )
                {
                    result[i].type = VarType::Float;
                    result[i].value = (float)(*(UA_Float*)val->data);
                }
                else if( val->type == &UA_TYPES[UA_TYPES_DOUBLE] )
                {
                    result[i].type = VarType::Float;
                    result[i].value = (float)(*(UA_Double*)val->data);
                }
            }
        }
    }
    else
    {
        logError(std::string("read failed: ") + UA_StatusCode_name(retval));

        for( auto& r : result )
            r.status = retval;
    }

    UA_ReadResponse_clear(&response);

    for( auto& attr : attrs )
        UA_ReadValueId_clear(&attr);

    return result;
}

JSOPCUAClient::ErrorCode JSOPCUAClient::write(const std::vector<WriteItem>& items)
{
    if( items.empty() )
        return UA_STATUSCODE_GOOD;

    if( !isConnected() )
        throw SystemError("JSOPCUAClient: not connected");

    std::vector<UA_WriteValue> values(items.size());

    for( size_t i = 0; i < items.size(); ++i )
    {
        UA_WriteValue_init(&values[i]);
        values[i].attributeId = UA_ATTRIBUTEID_VALUE;
        values[i].nodeId = UA_NODEID(items[i].nodeId.c_str());
        values[i].value.hasValue = true;

        const auto& val = items[i].value;

        if( std::holds_alternative<int32_t>(val) )
        {
            int32_t v = std::get<int32_t>(val);
            UA_Variant_setScalarCopy(&values[i].value.value, &v, &UA_TYPES[UA_TYPES_INT32]);
        }
        else if( std::holds_alternative<float>(val) )
        {
            float v = std::get<float>(val);
            UA_Variant_setScalarCopy(&values[i].value.value, &v, &UA_TYPES[UA_TYPES_FLOAT]);
        }
        else if( std::holds_alternative<bool>(val) )
        {
            UA_Boolean v = std::get<bool>(val) ? UA_TRUE : UA_FALSE;
            UA_Variant_setScalarCopy(&values[i].value.value, &v, &UA_TYPES[UA_TYPES_BOOLEAN]);
        }
    }

    UA_WriteRequest request;
    UA_WriteRequest_init(&request);
    request.nodesToWrite = values.data();
    request.nodesToWriteSize = values.size();

    UA_WriteResponse response = UA_Client_Service_write(client.handle(), request);
    auto retval = response.responseHeader.serviceResult;

    for( size_t i = 0; i < response.resultsSize && i < values.size(); ++i )
    {
        if( response.results[i] != UA_STATUSCODE_GOOD )
            retval = response.results[i];
    }

    if( retval != UA_STATUSCODE_GOOD )
        logError(std::string("write failed: ") + UA_StatusCode_name(retval));

    UA_WriteResponse_clear(&response);

    for( auto& v : values )
        UA_WriteValue_clear(&v);

    return retval;
}

UA_ReadValueId JSOPCUAClient::makeReadValue(const std::string& nodeId)
{
    UA_ReadValueId rv;
    UA_ReadValueId_init(&rv);
    rv.attributeId = UA_ATTRIBUTEID_VALUE;
    rv.nodeId = UA_NODEID(nodeId.c_str());
    return rv;
}

void JSOPCUAClient::logError(const std::string& msg) const
{
    if( log && log->is_warn() )
        log->warn() << "(JSOPCUAClient): " << msg << std::endl;
}
