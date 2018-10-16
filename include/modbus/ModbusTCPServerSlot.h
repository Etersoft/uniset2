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

			virtual ModbusRTU::mbErrCode readCoilStatus( ModbusRTU::ReadCoilMessage& query,
					ModbusRTU::ReadCoilRetMessage& reply ) override;

			virtual ModbusRTU::mbErrCode readInputStatus( ModbusRTU::ReadInputStatusMessage& query,
					ModbusRTU::ReadInputStatusRetMessage& reply ) override;

			virtual ModbusRTU::mbErrCode readOutputRegisters( ModbusRTU::ReadOutputMessage& query,
					ModbusRTU::ReadOutputRetMessage& reply ) override;

			virtual ModbusRTU::mbErrCode readInputRegisters( ModbusRTU::ReadInputMessage& query,
					ModbusRTU::ReadInputRetMessage& reply ) override;

			virtual ModbusRTU::mbErrCode forceSingleCoil( ModbusRTU::ForceSingleCoilMessage& query,
					ModbusRTU::ForceSingleCoilRetMessage& reply ) override;

			virtual ModbusRTU::mbErrCode writeOutputSingleRegister( ModbusRTU::WriteSingleOutputMessage& query,
					ModbusRTU::WriteSingleOutputRetMessage& reply ) override;

			virtual ModbusRTU::mbErrCode forceMultipleCoils( ModbusRTU::ForceCoilsMessage& query,
					ModbusRTU::ForceCoilsRetMessage& reply ) override;

			virtual ModbusRTU::mbErrCode writeOutputRegisters( ModbusRTU::WriteOutputMessage& query,
					ModbusRTU::WriteOutputRetMessage& reply ) override;

			virtual ModbusRTU::mbErrCode diagnostics( ModbusRTU::DiagnosticMessage& query,
					ModbusRTU::DiagnosticRetMessage& reply ) override;

			virtual ModbusRTU::mbErrCode read4314( ModbusRTU::MEIMessageRDI& query,
												   ModbusRTU::MEIMessageRetRDI& reply ) override;

			virtual ModbusRTU::mbErrCode journalCommand( ModbusRTU::JournalCommandMessage& query,
					ModbusRTU::JournalCommandRetMessage& reply ) override;

			virtual ModbusRTU::mbErrCode setDateTime( ModbusRTU::SetDateTimeMessage& query,
					ModbusRTU::SetDateTimeRetMessage& reply ) override;

			virtual ModbusRTU::mbErrCode remoteService( ModbusRTU::RemoteServiceMessage& query,
					ModbusRTU::RemoteServiceRetMessage& reply ) override;

			virtual ModbusRTU::mbErrCode fileTransfer( ModbusRTU::FileTransferMessage& query,
					ModbusRTU::FileTransferRetMessage& reply ) override;

		private:

	};
	// -------------------------------------------------------------------------
} // end of namespace uniset
// -------------------------------------------------------------------------
#endif // ModbusTCPServerSlot_H_
// -------------------------------------------------------------------------
