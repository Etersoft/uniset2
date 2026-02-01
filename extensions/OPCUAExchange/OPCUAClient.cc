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
#include <open62541pp/open62541pp.hpp>
#include "OPCUAClient.h"

// -----------------------------------------------------------------------------
using namespace uniset;
using namespace std;
// -----------------------------------------------------------------------------
OPCUAClient::OPCUAClient()
    : dlog( make_shared<DebugStream>() )
{
}
// -----------------------------------------------------------------------------
std::shared_ptr<DebugStream> OPCUAClient::log()
{
    return dlog;
}
// -----------------------------------------------------------------------------
void OPCUAClient::setLog( const std::shared_ptr<DebugStream>& _dlog )
{
    dlog = _dlog;
}
// -----------------------------------------------------------------------------
OPCUAClient::~OPCUAClient()
{
    client.disconnect();
}
// -----------------------------------------------------------------------------
bool OPCUAClient::connect( const std::string& addr )
{
    UA_SessionState sessionState{};
    UA_Client_getState(client.handle(), nullptr, &sessionState, nullptr);

    if( sessionState == UA_SESSIONSTATE_ACTIVATED )
        return true;

    try
    {
        client.connect(addr.c_str());
    }
    catch( const std::exception& ex )
    {
        if( dlog->debugging(Debug::CRIT) )
            dlog->crit() << addr << " (exception) " << ex.what() << endl;
        return false;
    }

    UA_Client_getState(client.handle(), nullptr, &sessionState, nullptr);
    return (sessionState == UA_SESSIONSTATE_ACTIVATED);
}

// -----------------------------------------------------------------------------
bool OPCUAClient::connect( const std::string& addr, const std::string& user, const std::string& pass )
{
    UA_SessionState sessionState{};
    UA_Client_getState(client.handle(), nullptr, &sessionState, nullptr);

    if( sessionState == UA_SESSIONSTATE_ACTIVATED )
        return true;

    try
    {
        if( !user.empty() )
            client.config().setUserIdentityToken(opcua::UserNameIdentityToken{user, pass});

        client.connect(addr);
    }
    catch( const std::exception& ex )
    {
        if( dlog->debugging(Debug::CRIT) )
            dlog->crit() << addr << " (exception) " << ex.what() << endl;
        return false;
    }

    UA_Client_getState(client.handle(), nullptr, &sessionState, nullptr);
    return (sessionState == UA_SESSIONSTATE_ACTIVATED);
}
// -----------------------------------------------------------------------------
void OPCUAClient::disconnect() noexcept
{
    client.disconnect();
}

// -----------------------------------------------------------------------------
OPCUAClient::VarType OPCUAClient::str2vtype( std::string_view s )
{
    if( s == "float" || s == "double" )
        return VarType::Float;

    return VarType::Int32;
}
// -----------------------------------------------------------------------------
int32_t OPCUAClient::ResultVar::get()
{
    if( type == VarType::Int32 )
    {
        try
        {
            return std::get<int32_t>(value);
        }
        catch(const std::bad_variant_access&) {}

        return 0;
    }

    if( type == VarType::Float )
    {
        try
        {
            return (int32_t) std::get<float>(value);
        }
        catch (const std::bad_variant_access&) {}

        return 0;
    }

    return 0;
}
// -----------------------------------------------------------------------------
void OPCUAClient::processResult(const opcua::String& node_name, const opcua::DataValue& in, OPCUAClient::ResultVar& out)
{
    out.status = in.status();

    if( !in.status().isGood() )
    {
        if( dlog->debugging(Debug::CRIT) )
            dlog->crit() << " Read nodeId: [" << node_name
                         << "] error with status: [" << in.status().name() << "]" << endl;
        return;
    }

    if( in.value().isType(&UA_TYPES[UA_TYPES_DOUBLE]) )
    {
        out.type = VarType::Float;
        out.value = (float)in.value().to<double>();
    }
    else if( in.value().isType(&UA_TYPES[UA_TYPES_FLOAT]) )
    {
        out.type = VarType::Float;
        out.value = (float)in.value().to<float>();
    }
    else if( in.value().isType(&UA_TYPES[UA_TYPES_BOOLEAN]) )
    {
        out.type = VarType::Int32;
        out.value = (int32_t)in.value().to<bool>();
    }
    else if( in.value().isType(&UA_TYPES[UA_TYPES_INT32]) )
    {
        out.type = VarType::Int32;
        out.value = (int32_t)in.value().to<int32_t>();
    }
    else if( in.value().isType(&UA_TYPES[UA_TYPES_UINT32]) )
    {
        out.type = VarType::Int32;
        out.value = (int32_t)in.value().to<uint32_t>();
    }
    else if( in.value().isType(&UA_TYPES[UA_TYPES_INT64]) )
    {
        out.type = VarType::Int32;
        out.value = (int32_t)in.value().to<int64_t>();
    }
    else if( in.value().isType(&UA_TYPES[UA_TYPES_UINT64]) )
    {
        out.type = VarType::Int32;
        out.value = (int32_t)in.value().to<uint64_t>();
    }
    else if( in.value().isType(&UA_TYPES[UA_TYPES_INT16]) )
    {
        out.type = VarType::Int32;
        out.value = (int32_t)in.value().to<int16_t>();
    }
    else if( in.value().isType(&UA_TYPES[UA_TYPES_UINT16]) )
    {
        out.type = VarType::Int32;
        out.value = (int32_t)in.value().to<uint16_t>();
    }
    else if( in.value().isType(&UA_TYPES[UA_TYPES_BYTE]) )
    {
        out.type = VarType::Int32;
        out.value = (int32_t)in.value().to<uint8_t>();
    }
    else
    {
        if( dlog->debugging(Debug::CRIT) )
            dlog->crit() << " Read nodeId: [" << node_name
                         << "] unknown type: [" << in.value().type()->typeName << "]" << endl;
        return;
    }

    if( dlog->debugging(Debug::LEVEL4) )
    {
        if( out.type == VarType::Float )
            dlog->level4() << " Read nodeId: [" << node_name << "] value: [" << out.as<float>() << "]" << endl;
        else
            dlog->level4() << " Read nodeId: [" << node_name << "] value: [" << out.get() << "]" << endl;
    }
}
// -----------------------------------------------------------------------------
opcua::StatusCode OPCUAClient::read(const std::vector<opcua::ua::ReadValueId>& attrs, std::vector<OPCUAClient::ResultVar>& results )
{
    if( dlog->debugging(Debug::LEVEL4) )
        dlog->level4() << "Read attributes" << endl;

    const auto request = opcua::services::detail::makeReadRequest(opcua::ua::TimestampsToReturn{}, attrs);
    const auto response = opcua::services::read(client, opcua::asWrapper<opcua::ua::ReadRequest>(request));
    const opcua::StatusCode serviceResult = opcua::services::detail::getServiceResult(response);

    if( serviceResult.isGood() )
    {
        if( response.results().size() > results.size() )
        {
            if( dlog->debugging(Debug::CRIT) )
                dlog->crit() << "Response items size mismatched: " << response.results().size() << " != " << results.size() << endl;
            return opcua::StatusCode{ UA_STATUSCODE_BADRESPONSETOOLARGE };
        } else
        {
            for(size_t i = 0; i < response.results().size(); i++)
            {
                processResult(opcua::toString(attrs[i].nodeId()), response.results()[i], results[i]);
            }
        }
    }
    return serviceResult;
}
// -----------------------------------------------------------------------------
opcua::StatusCode OPCUAClient::write32( std::vector<opcua::ua::WriteValue>& values )
{
    if( dlog->debugging(Debug::LEVEL4) )
        dlog->level4() << "Write attributes" << endl;
    const auto request = opcua::services::detail::makeWriteRequest({values.data(), values.size()});
    const auto response = opcua::services::write(client, opcua::asWrapper<opcua::ua::WriteRequest>(request));
    const opcua::StatusCode serviceResult = opcua::services::detail::getServiceResult(response);
    return serviceResult;
}
// -----------------------------------------------------------------------------
opcua::StatusCode OPCUAClient::write( const opcua::ua::WriteValue& writeValue )
{
    const auto request = opcua::services::detail::makeWriteRequest({&writeValue, 1});
    const auto response = opcua::services::write(client, opcua::asWrapper<opcua::ua::WriteRequest>(request));
    const opcua::StatusCode serviceResult = opcua::services::detail::getServiceResult(response);
    return serviceResult;
}
// -----------------------------------------------------------------------------
opcua::StatusCode OPCUAClient::set( const std::string& attr, bool set )
{
    const opcua::StatusCode serviceResult = opcua::services::writeAttribute(client,
                                            opcua::NodeId::parse(attr.c_str()),
                                            opcua::AttributeId::Value,
                                            opcua::DataValue{opcua::Variant{set}});
    return serviceResult;
}
// -----------------------------------------------------------------------------
opcua::StatusCode OPCUAClient::write32( const std::string& attr, int32_t value )
{
    const opcua::StatusCode serviceResult = opcua::services::writeAttribute(client,
                                            opcua::NodeId::parse(attr.c_str()),
                                            opcua::AttributeId::Value,
                                            opcua::DataValue{opcua::Variant{value}});
    return serviceResult;
}
// -----------------------------------------------------------------------------
opcua::ua::WriteValue OPCUAClient::makeWriteValue32( const std::string& name, int32_t val )
{
    return opcua::ua::WriteValue(opcua::NodeId::parse(name.c_str()),
                                 opcua::AttributeId::Value,
                                 {},
                                 opcua::DataValue{opcua::Variant{val}});
}
// -----------------------------------------------------------------------------
opcua::ua::ReadValueId OPCUAClient::makeReadValue32( const std::string& name )
{
    return opcua::ua::ReadValueId{opcua::NodeId::parse(name.c_str()), opcua::AttributeId::Value};
}
// -----------------------------------------------------------------------------
opcua::StatusCode OPCUAClient::subscribeDataChanges(std::vector<opcua::ua::ReadValueId>& ids,
                                                          std::vector<OPCUAClient::ResultVar>& results,
                                                          float samplingInterval,
                                                          float publishingInterval)
{
    if( ids.size() != results.size() )
        throw uniset::SystemError("subscribeDataChanges: ids.size(" + std::to_string(ids.size()) + ") != results.size(" + std::to_string(results.size()) + ")");

    opcua::SubscriptionParameters subscriptionParameters{};
    subscriptionParameters.publishingInterval = publishingInterval;
    auto subscriptionId = createSubscription(subscriptionParameters);

    std::vector<UA_MonitoredItemCreateRequest> requests(ids.size());
    std::vector<std::unique_ptr<opcua::services::detail::MonitoredItemContext>> contexts(ids.size());
    opcua::services::MonitoringParametersEx monitoringParameters{};
    monitoringParameters.samplingInterval = samplingInterval;

    for (size_t i = 0; i < ids.size(); ++i)
    {
        auto&& [id, res] = std::tie(ids[i], results[i]);

        //CreateMonitoredItemsRequest
        requests[i] = opcua::services::detail::makeMonitoredItemCreateRequest(
                                                id,
                                                opcua::MonitoringMode::Reporting,
                                                monitoringParameters);

        //MonitoredItemContexts
        contexts[i] = std::make_unique<opcua::services::detail::MonitoredItemContext>();
        contexts[i]->catcher = &opcua::detail::getExceptionCatcher(client);
        contexts[i]->itemToMonitor = id;
        contexts[i]->dataChangeCallback =
        [&](uint32_t subId, uint32_t monId, const opcua::DataValue& value)
        {
            processResult(opcua::toString(id.nodeId()), value, res);
        };
        contexts[i]->eventCallback = {};
        contexts[i]->deleteCallback = {};
    }

    const auto request = opcua::services::detail::makeCreateMonitoredItemsRequest(
                                                    subscriptionId,
                                                    monitoringParameters.timestamps,
                                                    {requests.data(), requests.size()});

    std::vector<opcua::services::detail::MonitoredItemContext*> contextsPtr(contexts.size());
    std::vector<UA_Client_DataChangeNotificationCallback> dataChangeCallbacks(contexts.size());
    std::vector<UA_Client_DeleteMonitoredItemCallback> deleteCallbacks(contexts.size());

    opcua::services::detail::convertMonitoredItemContexts(
        contexts, contextsPtr, dataChangeCallbacks, {}/*EventNotificationCallback*/, deleteCallbacks
    );

    opcua::ua::CreateMonitoredItemsResponse response = UA_Client_MonitoredItems_createDataChanges(
        client.handle(),
        request,
        reinterpret_cast<void**>(contextsPtr.data()),
        dataChangeCallbacks.data(),
        deleteCallbacks.data()
    );

    opcua::services::detail::storeMonitoredItemContexts(client, opcua::asWrapper<opcua::ua::CreateMonitoredItemsRequest>(request).subscriptionId(), response, contexts);

    const opcua::StatusCode serviceResult = opcua::services::detail::getServiceResult(response);
    opcua::throwIfBad(serviceResult);
 
    return serviceResult;
}
// -----------------------------------------------------------------------------
