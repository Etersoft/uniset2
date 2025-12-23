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
#include <unordered_map>
#include <map>
#include <variant>

#include "open62541pp/open62541pp.hpp"
#include "open62541pp/detail/exceptioncatcher.hpp"
#include "open62541pp/detail/client_utils.hpp"

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
            void disconnect() noexcept;

            // supported types (other types are converted to these if possible)
            enum class VarType : int
            {
                Int32 = 0,
                Float = 1
            };
            static VarType str2vtype( std::string_view s );

            struct ResultVar
            {
                ResultVar() {}
                std::variant<int32_t, float> value = { 0 };
                opcua::StatusCode status;
                VarType type = { VarType::Int32 }; // by default

                // get as int32_t (cast to int32_t if possible)
                int32_t get();

                template<class VType>
                VType as()
                {
                    try
                    {
                        return std::get<VType>(value);
                    }
                    catch(const std::bad_variant_access&) {}

                    return {};
                }
            };

            const opcua::StatusCode read(const std::vector<opcua::ua::ReadValueId>& attrs, std::vector<ResultVar>& results);
            const opcua::StatusCode write32( std::vector<opcua::ua::WriteValue>& values );
            const opcua::StatusCode write32( const std::string& attr, int32_t value );
            const opcua::StatusCode set( const std::string& attr, bool set );
            const opcua::StatusCode write( const opcua::ua::WriteValue& writeValue );
            
            static opcua::ua::WriteValue makeWriteValue32( const std::string& name, int32_t val );
            static opcua::ua::ReadValueId makeReadValue32( const std::string& name );

            void onSessionActivated(opcua::StateCallback callback)
            {
                client.onSessionActivated(std::move(callback));
            }

            void onConnected(opcua::StateCallback callback)
            {
                client.onConnected(std::move(callback));
            }

            void onSessionClosed(opcua::StateCallback callback)
            {
                client.onSessionClosed(std::move(callback));
            }

            void onDisconnected(opcua::StateCallback callback)
            {
                client.onDisconnected(std::move(callback));
            }

            void runIterate(uint16_t timeoutMilliseconds)
            {
                client.runIterate(timeoutMilliseconds);
            }

            void onInactive(opcua::InactivityCallback callback)
            {
                client.onInactive(std::move(callback));
            }

            void onSubscriptionInactive(opcua::SubscriptionInactivityCallback callback)
            {
                client.onSubscriptionInactive(std::move(callback));
            }

            opcua::ua::IntegerId createSubscription(const opcua::SubscriptionParameters& parameters)
            {
                auto subscription = opcua::Subscription<opcua::Client>{client, parameters};
                subscription.setPublishingMode(true);
                return subscription.subscriptionId();
            }

            void rethrowException()
            {
                auto& exceptionCatcher = opcua::detail::getExceptionCatcher(client);
                exceptionCatcher.rethrow(); // Работает только один раз, после повторной отправки удаляется!
            }

            const opcua::StatusCode subscribeDataChanges(std::vector<opcua::ua::ReadValueId>& ids,
                                                                                   std::vector<OPCUAClient::ResultVar>& results,
                                                                                   float samplingInterval,
                                                                                   float publishingInterval);

            size_t getSubscriptionSize()
            {
                size_t count = 0;
                auto subscriptions = client.subscriptions();
                for (auto& sub : subscriptions) {
                    count += sub.monitoredItems().size();
                }
                return count;
            }

            const opcua::StatusCode deleteSubscription(opcua::ua::IntegerId subId)
            {
                return opcua::services::deleteSubscription(client, subId); 
            }

            inline opcua::Node<opcua::Client> createNode(opcua::ua::VariableId nodeName)
            {
                return opcua::Node{client, nodeName};
            }

        protected:

            opcua::Client client;

        private:
            void processResult(opcua::String node_name, const opcua::DataValue& in, ResultVar& out);
    };
    // --------------------------------------------------------------------------
} // end of namespace uniset
// -----------------------------------------------------------------------------
#endif // OPCUAClient_H_
// -----------------------------------------------------------------------------
