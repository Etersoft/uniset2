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
#ifndef JSOPCUAClient_H_
#define JSOPCUAClient_H_
// --------------------------------------------------------------------------
#include <memory>
#include <string>
#include <vector>
#include <variant>

#include <open62541/client_highlevel.h>
#include <open62541/types_generated_handling.h>
#include <open62541pp/open62541pp.h>

#include "DebugStream.h"

namespace uniset
{
    /**
     * Облегченная реализация OPC UA клиента для скриптового движка.
     * Поддерживает только подключения и операции read/write.
     */
    class JSOPCUAClient
    {
        public:
            JSOPCUAClient();
            ~JSOPCUAClient();

            void setLog(const std::shared_ptr<DebugStream>& log);

            bool connect(const std::string& endpoint);
            bool connect(const std::string& endpoint, const std::string& user, const std::string& passwd);
            void disconnect() noexcept;

            bool isConnected() const noexcept;

            enum class VarType : int
            {
                Int32 = 0,
                Float = 1
            };

            struct ResultVar
            {
                std::variant<int32_t, float> value = {0};
                UA_StatusCode status = UA_STATUSCODE_BAD;
                VarType type = VarType::Int32;

                bool statusOk() const noexcept
                {
                    return status == UA_STATUSCODE_GOOD;
                }

                template<class VType>
                VType as() const
                {
                    try
                    {
                        return std::get<VType>(value);
                    }
                    catch(const std::bad_variant_access&)
                    {
                        return {};
                    }
                }
            };

            struct WriteItem
            {
                std::string nodeId;
                std::variant<int32_t, float, bool> value;
            };

            using ErrorCode = UA_StatusCode;

            std::vector<ResultVar> read(const std::vector<std::string>& nodeIds);
            ErrorCode write(const std::vector<WriteItem>& items);

        private:
            UA_ReadValueId makeReadValue(const std::string& nodeId);
            void logError(const std::string& msg) const;

            std::shared_ptr<DebugStream> log;
            opcua::Client client;
            bool connected = false;
    };
} // namespace uniset

#endif // JSOPCUAClient_H_
