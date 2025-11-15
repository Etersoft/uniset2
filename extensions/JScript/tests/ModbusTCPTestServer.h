/*
 * Simplified ModbusTCPServer helper for tests (based on Utilities/MBTester).
 */
#ifndef TESTS_MODBUS_TCP_TEST_SERVER_H_
#define TESTS_MODBUS_TCP_TEST_SERVER_H_

#include <unordered_set>
#include <string>
#include <random>
#include "modbus/ModbusTCPServerSlot.h"

namespace uniset
{
    class ModbusTCPServerTest
    {
        public:
            ModbusTCPServerTest( const std::unordered_set<ModbusRTU::ModbusAddr>& myaddr,
                                 const std::string& inetaddr,
                                 int port = 502,
                                 bool verbose = false );
            ~ModbusTCPServerTest();

            void setVerbose( bool state );
            void setReply( long val );
            void setRandomReply( long min, long max );
            void setFreezeReply( const std::unordered_map<uint16_t, uint16_t>& );
            void setMaxSessions( size_t max );

            bool execute();
            void stop();
            bool isActive() const;

        protected:
            ModbusRTU::mbErrCode readCoilStatus( const ModbusRTU::ReadCoilMessage& query,
                                                 ModbusRTU::ReadCoilRetMessage& reply );
            ModbusRTU::mbErrCode readInputStatus( const ModbusRTU::ReadInputStatusMessage& query,
                                                  ModbusRTU::ReadInputStatusRetMessage& reply );
            ModbusRTU::mbErrCode readOutputRegisters( const ModbusRTU::ReadOutputMessage& query,
                    ModbusRTU::ReadOutputRetMessage& reply );
            ModbusRTU::mbErrCode readInputRegisters( const ModbusRTU::ReadInputMessage& query,
                    ModbusRTU::ReadInputRetMessage& reply );
            ModbusRTU::mbErrCode forceSingleCoil( const ModbusRTU::ForceSingleCoilMessage& query,
                                                  ModbusRTU::ForceSingleCoilRetMessage& reply );
            ModbusRTU::mbErrCode forceMultipleCoils( const ModbusRTU::ForceCoilsMessage& query,
                    ModbusRTU::ForceCoilsRetMessage& reply );
            ModbusRTU::mbErrCode writeOutputRegisters( const ModbusRTU::WriteOutputMessage& query,
                    ModbusRTU::WriteOutputRetMessage& reply );
            ModbusRTU::mbErrCode writeOutputSingleRegister( const ModbusRTU::WriteSingleOutputMessage& query,
                    ModbusRTU::WriteSingleOutputRetMessage& reply );
            ModbusRTU::mbErrCode diagnostics( const ModbusRTU::DiagnosticMessage& query,
                                              ModbusRTU::DiagnosticRetMessage& reply );
            ModbusRTU::mbErrCode read4314( const ModbusRTU::MEIMessageRDI& query,
                                           ModbusRTU::MEIMessageRetRDI& reply );
            ModbusRTU::mbErrCode journalCommand( const ModbusRTU::JournalCommandMessage& query,
                                                 ModbusRTU::JournalCommandRetMessage& reply );
            ModbusRTU::mbErrCode setDateTime( const ModbusRTU::SetDateTimeMessage& query,
                                              ModbusRTU::SetDateTimeRetMessage& reply );
            ModbusRTU::mbErrCode remoteService( const ModbusRTU::RemoteServiceMessage& query,
                                                ModbusRTU::RemoteServiceRetMessage& reply );
            ModbusRTU::mbErrCode fileTransfer( const ModbusRTU::FileTransferMessage& query,
                                               ModbusRTU::FileTransferRetMessage& reply );

            ModbusTCPServerSlot* sslot;
            std::unordered_set<ModbusRTU::ModbusAddr> vaddr;
            bool verbose;
            long replyVal;
            std::unordered_map<uint16_t, uint16_t> reglist;
            std::random_device rnd;
            std::unique_ptr<std::mt19937> gen;
            std::unique_ptr<std::uniform_int_distribution<>> rndgen;
    };
} // namespace uniset

#endif // TESTS_MODBUS_TCP_TEST_SERVER_H_
