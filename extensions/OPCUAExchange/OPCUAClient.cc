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

#include <limits>
#include <iomanip>
#include <chrono>

#include <open62541pp/open62541pp.hpp>
#include "OPCUAClient.h"

// -----------------------------------------------------------------------------
using namespace uniset;
using namespace std;
// -----------------------------------------------------------------------------
/// Get name of log level.
constexpr std::string_view getLogLevelName(opcua::LogLevel level)
{
    switch (level)
    {
        case opcua::LogLevel::Trace:
            return "trace";

        case opcua::LogLevel::Debug:
            return "debug";

        case opcua::LogLevel::Info:
            return "info";

        case opcua::LogLevel::Warning:
            return "warning";

        case opcua::LogLevel::Error:
            return "error";

        case opcua::LogLevel::Fatal:
            return "fatal";

        default:
            return "unknown";
    }
}

/// Get name of log category.
constexpr std::string_view getLogCategoryName(opcua::LogCategory category)
{
    switch (category)
    {
        case opcua::LogCategory::Network:
            return "network";

        case opcua::LogCategory::SecureChannel:
            return "channel";

        case opcua::LogCategory::Session:
            return "session";

        case opcua::LogCategory::Server:
            return "server";

        case opcua::LogCategory::Client:
            return "client";

        case opcua::LogCategory::Userland:
            return "userland";

        case opcua::LogCategory::SecurityPolicy:
            return "securitypolicy";

        default:
            return "unknown";
    }
}
// -----------------------------------------------------------------------------
namespace opcua
{
    static auto log = [](Client& client, auto level, auto category, auto msg)
    {
        std::cout << "[" << getLogLevelName(level) << "] "
                  << "[" << getLogCategoryName(category) << "] " << msg << std::endl;
    };
}
// -----------------------------------------------------------------------------
OPCUAClient::OPCUAClient()
{
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
        //cerr << addr << " (exception) "<< ex.what()<<endl;
        opcua::log(client, opcua::LogLevel::Error, opcua::LogCategory::Client, addr + " (exception) " + ex.what());
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
        //cerr << addr << " (exception) " << ex.what() << endl;
        opcua::log(client, opcua::LogLevel::Error, opcua::LogCategory::Client, addr + " (exception) " + ex.what());
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
void OPCUAClient::processResult(opcua::String node_name, const opcua::DataValue& in, OPCUAClient::ResultVar& out)
{
    std::stringstream ss;
    ss << " Read nodeId: [" << node_name;
    out.status = in.status();
 
    if(in.status().isGood())
    {
        
        ss << "] status: [" << in.status().name();
        if ( in.value().isType(&UA_TYPES[UA_TYPES_DOUBLE]) )
        {
            out.type = VarType::Float;
            out.value = (float)in.value().to<double>();
            ss << "] value(double): [" << out.as<float>();
        } else if ( in.value().isType(&UA_TYPES[UA_TYPES_FLOAT]) )
        {
            out.type = VarType::Float;
            out.value = (float)in.value().to<float>();
            ss << "] value(float): [" << out.as<float>();
        } else if ( in.value().isType(&UA_TYPES[UA_TYPES_BOOLEAN]) )
        {
            out.type = VarType::Int32;
            out.value = (int32_t)in.value().to<bool>();
            ss << "] value(int32_t): [" << (int32_t)out.get();
        } else if ( in.value().isType(&UA_TYPES[UA_TYPES_INT32]) )
        {
            out.type = VarType::Int32;
            out.value = (int32_t)in.value().to<int32_t>();
            ss << "] value(int32_t): [" << (int32_t)out.get();
        } else if ( in.value().isType(&UA_TYPES[UA_TYPES_UINT32]) )
        {
            out.type = VarType::Int32;
            out.value = (int32_t)in.value().to<uint32_t>();
            ss << "] value(int32_t): [" << (int32_t)out.get();
        } else if ( in.value().isType(&UA_TYPES[UA_TYPES_INT64]) )
        {
            out.type = VarType::Int32;
            out.value = (int32_t)in.value().to<int64_t>();
            ss << "] value(int32_t): [" << (int32_t)out.get();
        } else if ( in.value().isType(&UA_TYPES[UA_TYPES_UINT64]) )
        {
            out.type = VarType::Int32;
            out.value = (int32_t)in.value().to<uint64_t>();
            ss << "] value(int32_t): [" << (int32_t)out.get();
        } else if ( in.value().isType(&UA_TYPES[UA_TYPES_INT16]) )
        {
            out.type = VarType::Int32;
            out.value = (int32_t)in.value().to<int16_t>();
            ss << "] value(int32_t): [" << (int32_t)out.get();
        } else if ( in.value().isType(&UA_TYPES[UA_TYPES_UINT16]) )
        {
            out.type = VarType::Int32;
            out.value = (int32_t)in.value().to<uint16_t>();
            ss << "] value(int32_t): [" << (int32_t)out.get();
        } else if ( in.value().isType(&UA_TYPES[UA_TYPES_BYTE]) )
        {
            out.type = VarType::Int32;
            out.value = (int32_t)in.value().to<uint8_t>();
            ss << "] value(int32_t): [" << (int32_t)out.get();
        }
        else
        {
            ss << " unknown type: [" << in.value().type()->typeName << "]";
            opcua::log( client, opcua::LogLevel::Error, opcua::LogCategory::Client, ss.str());
            return;
        }     
        ss << "]";
        opcua::log( client, opcua::LogLevel::Info, opcua::LogCategory::Client, ss.str());
    }
    else
    { 
        ss << "] error with status: [" << in.status().name() << "]";
        opcua::log( client, opcua::LogLevel::Error, opcua::LogCategory::Client, ss.str());
    }
}
// -----------------------------------------------------------------------------
const opcua::StatusCode OPCUAClient::read(const std::vector<opcua::ua::ReadValueId>& attrs, std::vector<OPCUAClient::ResultVar>& results )
{
    opcua::log(client, opcua::LogLevel::Info, opcua::LogCategory::Client, "Read attributes");
    const auto request = opcua::services::detail::makeReadRequest(opcua::ua::TimestampsToReturn{}, attrs);
    const auto response = opcua::services::read(client, opcua::asWrapper<opcua::ua::ReadRequest>(request));
    const opcua::StatusCode serviceResult = opcua::services::detail::getServiceResult(response);

    if( serviceResult.isGood() )
    {
        if( response.results().size() > results.size() )
        {
            std::stringstream ss;
            ss << "Response items size mismatched: " << response.results().size() << " != " << results.size();
            opcua::log(client, opcua::LogLevel::Error, opcua::LogCategory::Client, ss.str());
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
const opcua::StatusCode OPCUAClient::write32( std::vector<opcua::ua::WriteValue>& values )
{
    opcua::log(client, opcua::LogLevel::Info, opcua::LogCategory::Client, "Write attributes");
    const auto request = opcua::services::detail::makeWriteRequest({values.data(), values.size()});
    const auto response = opcua::services::write(client, opcua::asWrapper<opcua::ua::WriteRequest>(request));
    const opcua::StatusCode serviceResult = opcua::services::detail::getServiceResult(response);
    return serviceResult;
}
// -----------------------------------------------------------------------------
const opcua::StatusCode OPCUAClient::write( const opcua::ua::WriteValue& writeValue )
{
    const auto request = opcua::services::detail::makeWriteRequest({&writeValue, 1});
    const auto response = opcua::services::write(client, opcua::asWrapper<opcua::ua::WriteRequest>(request));
    const opcua::StatusCode serviceResult = opcua::services::detail::getServiceResult(response);
    return serviceResult;
}
// -----------------------------------------------------------------------------
const opcua::StatusCode OPCUAClient::set( const std::string& attr, bool set )
{
    const opcua::StatusCode serviceResult = opcua::services::writeAttribute(client,
                                            opcua::NodeId::parse(attr.c_str()),
                                            opcua::AttributeId::Value,
                                            opcua::DataValue{opcua::Variant{set}});
    return serviceResult;
}
// -----------------------------------------------------------------------------
const opcua::StatusCode OPCUAClient::write32( const std::string& attr, int32_t value )
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
const opcua::StatusCode OPCUAClient::subscribeDataChanges(std::vector<opcua::ua::ReadValueId>& ids,
                                                          std::vector<OPCUAClient::ResultVar>& results,
                                                          float samplingInterval,
                                                          float publishingInterval)
{
    assert(ids.size() == results.size());

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
