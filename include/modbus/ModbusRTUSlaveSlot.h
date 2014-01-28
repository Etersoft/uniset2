// -------------------------------------------------------------------------
#ifndef ModbusRTUSlaveSlot_H_
#define ModbusRTUSlaveSlot_H_
// -------------------------------------------------------------------------
#include <string>
#include <sigc++/sigc++.h>
#include "ModbusRTUSlave.h"
#include "ModbusServerSlot.h"
// -------------------------------------------------------------------------
/*!	
	Реализация позволяющая добавлять обработчики не наследуясь от ModbusRTUSlave.
	Основана на использовании слотов.
	\warning Пока реализована возможность подключения ТОЛЬКО ОДНОГО обработчика 
*/
class ModbusRTUSlaveSlot:
	public ModbusRTUSlave,
	public ModbusServerSlot
{
	public:
		ModbusRTUSlaveSlot( ComPort* com );
		ModbusRTUSlaveSlot( const std::string dev, bool use485=false, bool tr_ctl=false );
		virtual ~ModbusRTUSlaveSlot();

		virtual void sigterm( int signo );

		inline ComPort* getComPort(){ return port; }
	
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

		virtual ModbusRTU::mbErrCode read4314( ModbusRTU::MEIMessageRDI& query, 
														ModbusRTU::MEIMessageRetRDI& reply );

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
#endif // ModbusRTUSlaveSlot_H_
// -------------------------------------------------------------------------
