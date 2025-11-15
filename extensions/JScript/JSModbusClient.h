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
#ifndef JSModbusClient_H_
#define JSModbusClient_H_
// --------------------------------------------------------------------------
#include <memory>
#include <string>
#include <vector>
#include "DebugStream.h"
#include "modbus/ModbusTCPMaster.h"
#include "Exceptions.h"
// --------------------------------------------------------------------------
namespace uniset
{
    // ----------------------------------------------------------------------
    class JSModbusClient
    {
        public:
            JSModbusClient();
            ~JSModbusClient();

            void setLog( const std::shared_ptr<DebugStream>& log );

            void connectTCP( const std::string& host, int port, timeout_t timeout_msec, bool forceDisconnect );
            void disconnect();
            inline bool isConnected() const noexcept
            {
                return tcp && tcp->isConnection();
            }

            ModbusRTU::ReadCoilRetMessage read01( ModbusRTU::ModbusAddr addr,
                                                  ModbusRTU::ModbusData start, ModbusRTU::ModbusData count );
            ModbusRTU::ReadInputStatusRetMessage read02( ModbusRTU::ModbusAddr addr,
                                                         ModbusRTU::ModbusData start, ModbusRTU::ModbusData count );
            ModbusRTU::ReadOutputRetMessage read03( ModbusRTU::ModbusAddr addr,
                                                    ModbusRTU::ModbusData start, ModbusRTU::ModbusData count );
            ModbusRTU::ReadInputRetMessage read04( ModbusRTU::ModbusAddr addr,
                                                   ModbusRTU::ModbusData start, ModbusRTU::ModbusData count );

            ModbusRTU::ForceSingleCoilRetMessage write05( ModbusRTU::ModbusAddr addr,
                                                          ModbusRTU::ModbusData reg, bool state );
            ModbusRTU::WriteSingleOutputRetMessage write06( ModbusRTU::ModbusAddr addr,
                                                            ModbusRTU::ModbusData reg, ModbusRTU::ModbusData value );
            ModbusRTU::ForceCoilsRetMessage write0F( ModbusRTU::ModbusAddr addr,
                                                     ModbusRTU::ModbusData start,
                                                     const std::vector<uint8_t>& values );
            ModbusRTU::WriteOutputRetMessage write10( ModbusRTU::ModbusAddr addr,
                                                      ModbusRTU::ModbusData start,
                                                      const std::vector<ModbusRTU::ModbusData>& values );

            ModbusRTU::DiagnosticRetMessage diag08( ModbusRTU::ModbusAddr addr,
                                                    ModbusRTU::DiagnosticsSubFunction subfunc,
                                                    ModbusRTU::ModbusData data );
            ModbusRTU::MEIMessageRetRDI read4314( ModbusRTU::ModbusAddr addr,
                                                  ModbusRTU::ModbusByte devID,
                                                  ModbusRTU::ModbusByte objID );

        private:
            void ensureConnection();
            void ensureTCP();

            std::shared_ptr<DebugStream> log;
            std::unique_ptr<ModbusTCPMaster> tcp;
            std::string lastHost;
            int lastPort = { 0 };
            timeout_t lastTimeout = { 2000 };
            bool reconnectEachRequest = { false };
    };
    // ----------------------------------------------------------------------
} // namespace uniset
// --------------------------------------------------------------------------
#endif // JSModbusClient_H_
// --------------------------------------------------------------------------
