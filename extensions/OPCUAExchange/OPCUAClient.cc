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

#include <open62541/client_highlevel.h>
#include <open62541/client_subscriptions.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/types_generated_handling.h>

#include <open62541pp/open62541pp.hpp>

#include "OPCUAClient.h"

namespace uniset
{
    static void dataChangeNotificationCallback(
        [[maybe_unused]] UA_Client* client,
        uint32_t subId,
        [[maybe_unused]] void* subContext,
        uint32_t monId,
        void* monContext,
        UA_DataValue* value
    ) noexcept
    {
        if (monContext == nullptr)
            return;

        auto* monitoredItem = static_cast<uniset::MonitoredItem*>(monContext);

        if (monitoredItem->dataChangeCallback)
        {
            monitoredItem->dataChangeCallback(subId, monId, opcua::asWrapper<opcua::DataValue>(*value));
        }
    }

    static void deleteMonitoredItemCallback(
        [[maybe_unused]] UA_Client* client,
        uint32_t subId,
        [[maybe_unused]] void* subContext,
        uint32_t monId,
        void* monContext
    ) noexcept
    {
        if (monContext == nullptr)
            return;

        auto* monitoredItem = static_cast<uniset::MonitoredItem*>(monContext);

        if (monitoredItem->deleteCallback)
        {
            monitoredItem->deleteCallback(subId, monId);
        }
    }

}// end of namespace uniset

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
    //    opcua::log(client, opcua::LogLevel::Info, opcua::LogCategory::Client, "create OPCUAClient");
    val = UA_Variant_new();
}
// -----------------------------------------------------------------------------
OPCUAClient::~OPCUAClient()
{
    client.disconnect();
    UA_Variant_delete(val);
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
bool OPCUAClient::ResultVar::statusOk()
{
    return status == UA_STATUSCODE_GOOD;
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
OPCUAClient::ErrorCode OPCUAClient::read(std::vector<UA_ReadValueId>& attrs, std::vector<ResultVar>& result )
{
    UA_ReadRequest request;
    UA_ReadRequest_init(&request);
    request.nodesToRead = attrs.data();
    request.nodesToReadSize = attrs.size();

    UA_ReadResponse response = UA_Client_Service_read(client.handle(), request);
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
                result[i].value = 0;

                if( response.results[i].status == UA_STATUSCODE_GOOD )
                {
                    result[i].type = VarType::Int32; // by default
                    UA_Variant* val = &response.results[i].value;

                    if (val->type == &UA_TYPES[UA_TYPES_INT32])
                        result[i].value = int32_t(*(UA_Int32*) val->data);
                    else if (val->type == &UA_TYPES[UA_TYPES_UINT32])
                        result[i].value = int32_t(*(UA_UInt32*) val->data);
                    else if (val->type == &UA_TYPES[UA_TYPES_INT64])
                        result[i].value = int32_t(*(UA_Int64*) val->data);
                    else if (val->type == &UA_TYPES[UA_TYPES_UINT64])
                        result[i].value = int32_t(*(UA_UInt64*) val->data);
                    else if (val->type == &UA_TYPES[UA_TYPES_BOOLEAN])
                        result[i].value = *(UA_Boolean*) val->data ? 1 : 0;
                    else if (val->type == &UA_TYPES[UA_TYPES_UINT16])
                        result[i].value = int32_t(*(UA_UInt16*) val->data);
                    else if (val->type == &UA_TYPES[UA_TYPES_INT16])
                        result[i].value = int32_t(*(UA_Int16*) val->data);
                    else if (val->type == &UA_TYPES[UA_TYPES_BYTE])
                        result[i].value = int32_t(*(UA_Byte*) val->data);
                    else if (val->type == &UA_TYPES[UA_TYPES_FLOAT])
                    {
                        result[i].type  = VarType::Float;
                        result[i].value = (float)(*(UA_Float*) val->data);
                    }
                    else if (val->type == &UA_TYPES[UA_TYPES_DOUBLE])
                    {
                        result[i].type  = VarType::Float;
                        result[i].value = (float)(*(UA_Double*)val->data);
                    }
                }
            }
        }
    }
    else
    {
        opcua::log(client, opcua::LogLevel::Warning, opcua::LogCategory::Client, "read error!");

        for( auto&& r : result )
            r.status = retval;
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
    UA_WriteResponse response = UA_Client_Service_write(client.handle(), request);

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
    UA_WriteResponse response = UA_Client_Service_write(client.handle(), request);

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
OPCUAClient::ErrorCode OPCUAClient::set( const std::string& attr, bool set )
{
    UA_Variant_clear(val);
    UA_Variant_setScalarCopy(val, &set, &UA_TYPES[UA_TYPES_BOOLEAN]);

    return UA_Client_writeValueAttribute(client.handle(), UA_NODEID(attr.c_str()), val);
}
// -----------------------------------------------------------------------------
OPCUAClient::ErrorCode OPCUAClient::write32( const std::string& attr, int32_t value )
{
    UA_Variant_clear(val);
    UA_Variant_setScalarCopy(val, &value, &UA_TYPES[UA_TYPES_INT32]);
    return UA_Client_writeValueAttribute(client.handle(), UA_NODEID(attr.c_str()), val);
}
// -----------------------------------------------------------------------------
UA_WriteValue OPCUAClient::makeWriteValue32( const std::string& name, int32_t val )
{
    UA_WriteValue wv;
    UA_WriteValue_init(&wv);
    wv.attributeId = UA_ATTRIBUTEID_VALUE;
    wv.nodeId = UA_NODEID(name.c_str());
    wv.value.hasValue = true;
    UA_Variant_setScalarCopy(&wv.value.value, &val, &UA_TYPES[UA_TYPES_INT32]);
    return wv;
}
// -----------------------------------------------------------------------------
UA_ReadValueId OPCUAClient::makeReadValue32( const std::string& name )
{
    UA_ReadValueId rv;
    UA_ReadValueId_init(&rv);
    rv.attributeId = UA_ATTRIBUTEID_VALUE;
    rv.nodeId = UA_NODEID(name.c_str());
    return rv;
}
// -----------------------------------------------------------------------------
std::vector<opcua::MonitoredItem<opcua::Client>> OPCUAClient::subscribeDataChanges(
            opcua::Subscription<opcua::Client>& sub,
            std::vector<UA_ReadValueId>& attrs,
            std::vector<uniset::DataChangeCallback>& dataChangeCallbacks,
            std::vector<uniset::DeleteMonitoredItemCallback>& deleteMonItemCallbacks,
            bool stop /* флаг остановки при ошибках в процессе подписки */
        )
{
    std::vector<opcua::MonitoredItem<opcua::Client>> ret;
    auto subscriptionId_ = sub.subscriptionId();
    auto& exceptionCatcher = opcua::detail::getExceptionCatcher(client);
    std::vector<std::unique_ptr<uniset::MonitoredItem>> monItems;
    size_t attr_size = attrs.size();

    std::vector<UA_MonitoredItemCreateRequest> items;
    std::vector<UA_Client_DataChangeNotificationCallback> callbacks;
    std::vector<UA_Client_DeleteMonitoredItemCallback> deleteCallbacks;
    std::vector<void*> contexts;

    for( size_t i = 0 ; i < attr_size ; i++ )
    {
        /* monitor the server state */
        items.emplace_back(UA_MonitoredItemCreateRequest_default(attrs[i].nodeId));
        callbacks.emplace_back(dataChangeNotificationCallback);
        deleteCallbacks.emplace_back(deleteMonitoredItemCallback);

        auto context = std::make_unique<uniset::MonitoredItem>();
        context->itemToMonitor = opcua::ReadValueId(attrs[i]);
        context->dataChangeCallback = exceptionCatcher.wrapCallback(
                                          [this, callback = std::move(dataChangeCallbacks[i])](
                                              uint32_t subId, uint32_t monId, const opcua::DataValue & value
                                          )
        {
            auto it = this->monitoredItems.find({subId, monId});

            if (it == this->monitoredItems.end())
            {
                opcua::log(client, opcua::LogLevel::Error, opcua::LogCategory::Client, monId + ": monitored item not found!");
                throw opcua::BadStatus(UA_STATUSCODE_BADMONITOREDITEMIDINVALID);
            }

            callback(*(it->second.get()), value);
        });

        context->deleteCallback = exceptionCatcher.wrapCallback(
                                      [this, callback = std::move(deleteMonItemCallbacks[i])](
                                          uint32_t subId, uint32_t monId
                                      )
        {
            if(!monId)
                return;

            auto it = this->monitoredItems.find({subId, monId});

            if (it == this->monitoredItems.end())
            {
                opcua::log(client, opcua::LogLevel::Error, opcua::LogCategory::Client, monId + ": monitored item not found!");
                throw opcua::BadStatus(UA_STATUSCODE_BADMONITOREDITEMIDINVALID);
            }
            else
            {
                opcua::log(client, opcua::LogLevel::Info, opcua::LogCategory::Client, monId + ": monitored item remove!");
                this->monitoredItems.erase({subId, monId});
            }

            if(callback)
                callback(subId, monId);
        });

        contexts.emplace_back(context.get());
        /* NOTE!
            В monitoredItems хранится context только для элементов, на которые успешно подпишется клиент.
           Поэтому contexts здесь хранит все элементы пока не получим ответ на запрос.
           В самом клиенте также создается элемент UA_Client_MonitoredItem только при успешной подписке.
           В нем сохраняется raw указатель на элемент contexts, но за существование и удаление отвечает
           OPCUAClient! Поэтому он должен гарантировать существование monitoredItems пока выполняется runIterate
           и существует клиент opcua::Client. Возможность отписки здесь не реализована, так что удаление из
           monitoredItems происходит только при удалении подписки, при этом вызывается специальный delete_callback.
           И так же если возможность привязать пользователю OPCUAClient дополнительный обработчик вызова delete_callback.
        */
        monItems.emplace_back(std::move(context));
    }

    UA_CreateMonitoredItemsRequest createRequest;
    UA_CreateMonitoredItemsRequest_init(&createRequest);
    createRequest.subscriptionId = subscriptionId_;
    createRequest.timestampsToReturn = UA_TIMESTAMPSTORETURN_BOTH;
    createRequest.itemsToCreate = items.data();
    createRequest.itemsToCreateSize = attr_size;

    using createResponse = opcua::TypeWrapper<UA_CreateMonitoredItemsResponse, UA_TYPES_CREATEMONITOREDITEMSRESPONSE>;
    const createResponse response =
        UA_Client_MonitoredItems_createDataChanges(client.handle(), createRequest, contexts.data(),
                callbacks.data(), deleteCallbacks.data());

    for(size_t i = 0; i < response->resultsSize; i++)
    {
        auto code = response->results[i].statusCode;

        // stop - флаг для остановки процесса инициализации подписки, если не указано игнорировать ошибки в процессе
        // подписки. Если указано игнорировать, то появится только лог с ошибкой ниже и дальше продолжится обработка ответа.
        if(stop)
            opcua::throwIfBad(code);

        if(code == UA_STATUSCODE_GOOD)
        {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Good monitoring for attr = '%-16.*s' with monitoring id = '%u'",
                        (int)attrs[i].nodeId.identifier.string.length,
                        attrs[i].nodeId.identifier.string.data,
                        response->results[i].monitoredItemId);
            // Сохраняем список обработчиков для отслеживаемых элементов
            monitoredItems.insert_or_assign({subscriptionId_, response->results[i].monitoredItemId}, std::move(monItems[i]));
        }
        else
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Bad monitoring for attr = '%-16.*s' with error '%s'",
                        (int)attrs[i].nodeId.identifier.string.length,
                        attrs[i].nodeId.identifier.string.data,
                        UA_StatusCode_name(code));

        // Возвращаем список opcua::MonitoredItem<opcua::Client> для удобства настройки параметров
        ret.emplace_back(sub.connection(), subscriptionId_, response->results[i].monitoredItemId);
    }

    return ret;
}
// -----------------------------------------------------------------------------
