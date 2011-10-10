// -------------------------------------------------------------------------
#ifndef ModbusTCPServerSlot_H_
#define ModbusTCPServerSlot_H_
// -------------------------------------------------------------------------
#include <string>
#include <cc++/socket.h>
#include "ModbusTCPServer.h"
#include "ModbusServerSlot.h"
// -------------------------------------------------------------------------
/*!	ModbusTCP server (slot version) */
class ModbusTCPServerSlot:
	public ModbusServerSlot,
	public ModbusTCPServer
{
	public:
		ModbusTCPServerSlot( ost::InetAddress &ia, int port=502 );
		virtual ~ModbusTCPServerSlot();

		virtual void sigterm( int signo );
	
	protected:

		virtual ModbusRTU::mbErrCode readCoilStatus( ModbusRTU::ReadCoilMessage& query, 
															ModbusRTU::ReadCoilRetMessage& reply );

		virtual ModbusRTU::mbErrCode readInputStatus( ModbusRTU::ReadInputStatusMessage& query, 
															ModbusRTU::ReadInputStatusRetMessage& reply );

		virtual ModbusRTU::mbErrCode readOutputRegisters( ModbusRTU::ReadOutputMessage& query, 
														ModbusRTU::ReadOutputRetMessage& reply );

		virtual ModbusRTU::mbErrCode readInputRegisters( ModbusRTU::ReadInputMessage& query, 
															ModbusRTU::ReadInputRetMessage& reply );

		virtual ModbusRTU::mbErrCode forceSingleCoil( ModbusRTU::ForceSingleCoilMessage& query, 
														ModbusRTU::ForceSingleCoilRetMessage& reply );

		virtual ModbusRTU::mbErrCode writeOutputSingleRegister( ModbusRTU::WriteSingleOutputMessage& query, 
														ModbusRTU::WriteSingleOutputRetMessage& reply );

		virtual ModbusRTU::mbErrCode forceMultipleCoils( ModbusRTU::ForceCoilsMessage& query, 
														ModbusRTU::ForceCoilsRetMessage& reply );

		virtual ModbusRTU::mbErrCode writeOutputRegisters( ModbusRTU::WriteOutputMessage& query, 
														ModbusRTU::WriteOutputRetMessage& reply );

		virtual ModbusRTU::mbErrCode diagnostics( ModbusRTU::DiagnosticMessage& query, 
														ModbusRTU::DiagnosticRetMessage& reply );

		virtual ModbusRTU::mbErrCode journalCommand( ModbusRTU::JournalCommandMessage& query, 
														ModbusRTU::JournalCommandRetMessage& reply );

		virtual ModbusRTU::mbErrCode setDateTime( ModbusRTU::SetDateTimeMessage& query, 
															ModbusRTU::SetDateTimeRetMessage& reply );

		virtual ModbusRTU::mbErrCode remoteService( ModbusRTU::RemoteServiceMessage& query, 
															ModbusRTU::RemoteServiceRetMessage& reply );

		virtual ModbusRTU::mbErrCode fileTransfer( ModbusRTU::FileTransferMessage& query, 
															ModbusRTU::FileTransferRetMessage& reply );

	private:

};
// -------------------------------------------------------------------------
#endif // ModbusTCPServerSlot_H_
// -------------------------------------------------------------------------
