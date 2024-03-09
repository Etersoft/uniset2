// -------------------------------------------------------------------------
#ifndef ModbusTCPServerSlot_H_
#define ModbusTCPServerSlot_H_
// -------------------------------------------------------------------------
#include <string>
#include "ModbusTCPServer.h"
#include "ModbusServerSlot.h"
// -------------------------------------------------------------------------
namespace uniset
{
    // -------------------------------------------------------------------------
    /*!    ModbusTCP server (slot version) */
    class ModbusTCPServerSlot:
        public ModbusServerSlot,
        public ModbusTCPServer
    {
        public:
            ModbusTCPServerSlot( const std::string& ia, int port = 502 );
            virtual ~ModbusTCPServerSlot();

            virtual void terminate() override;

        protected:

            virtual ModbusRTU::mbErrCode readCoilStatus( const ModbusRTU::ReadCoilMessage& query,
                    ModbusRTU::ReadCoilRetMessage& reply ) override;

            virtual ModbusRTU::mbErrCode readInputStatus( const ModbusRTU::ReadInputStatusMessage& query,
                    ModbusRTU::ReadInputStatusRetMessage& reply ) override;

            virtual ModbusRTU::mbErrCode readOutputRegisters( const ModbusRTU::ReadOutputMessage& query,
                    ModbusRTU::ReadOutputRetMessage& reply ) override;

            virtual ModbusRTU::mbErrCode readInputRegisters( const ModbusRTU::ReadInputMessage& query,
                    ModbusRTU::ReadInputRetMessage& reply ) override;

            virtual ModbusRTU::mbErrCode forceSingleCoil( const ModbusRTU::ForceSingleCoilMessage& query,
                    ModbusRTU::ForceSingleCoilRetMessage& reply ) override;

            virtual ModbusRTU::mbErrCode writeOutputSingleRegister( const ModbusRTU::WriteSingleOutputMessage& query,
                    ModbusRTU::WriteSingleOutputRetMessage& reply ) override;

            virtual ModbusRTU::mbErrCode forceMultipleCoils( const ModbusRTU::ForceCoilsMessage& query,
                    ModbusRTU::ForceCoilsRetMessage& reply ) override;

            virtual ModbusRTU::mbErrCode writeOutputRegisters( const ModbusRTU::WriteOutputMessage& query,
                    ModbusRTU::WriteOutputRetMessage& reply ) override;

            virtual ModbusRTU::mbErrCode diagnostics( const ModbusRTU::DiagnosticMessage& query,
                    ModbusRTU::DiagnosticRetMessage& reply ) override;

            virtual ModbusRTU::mbErrCode read4314( const ModbusRTU::MEIMessageRDI& query,
                                                   ModbusRTU::MEIMessageRetRDI& reply ) override;

            virtual ModbusRTU::mbErrCode journalCommand( const ModbusRTU::JournalCommandMessage& query,
                    ModbusRTU::JournalCommandRetMessage& reply ) override;

            virtual ModbusRTU::mbErrCode setDateTime( const ModbusRTU::SetDateTimeMessage& query,
                    ModbusRTU::SetDateTimeRetMessage& reply ) override;

            virtual ModbusRTU::mbErrCode remoteService( const ModbusRTU::RemoteServiceMessage& query,
                    ModbusRTU::RemoteServiceRetMessage& reply ) override;

            virtual ModbusRTU::mbErrCode fileTransfer( const ModbusRTU::FileTransferMessage& query,
                    ModbusRTU::FileTransferRetMessage& reply ) override;

        private:

    };
    // -------------------------------------------------------------------------
} // end of namespace uniset
// -------------------------------------------------------------------------
#endif // ModbusTCPServerSlot_H_
// -------------------------------------------------------------------------
