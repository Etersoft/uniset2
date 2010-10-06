#include "modbus/ModbusTCPServerSlot.h"
// -------------------------------------------------------------------------
using namespace ModbusRTU;
using namespace std;
// -------------------------------------------------------------------------
ModbusTCPServerSlot::ModbusTCPServerSlot( ost::InetAddress &ia, int port ):
	ModbusTCPServer(ia,port)
{
}
// -------------------------------------------------------------------------
ModbusTCPServerSlot::~ModbusTCPServerSlot()
{

}
// -------------------------------------------------------------------------
mbErrCode ModbusTCPServerSlot::readCoilStatus( ReadCoilMessage& query, 
												ReadCoilRetMessage& reply )
{
	if( !slReadCoil )
		return erOperationFailed;

	return slReadCoil(query,reply);
}

// -------------------------------------------------------------------------
mbErrCode ModbusTCPServerSlot::readInputStatus( ReadInputStatusMessage& query, 
												ReadInputStatusRetMessage& reply )
{
	if( !slReadInputStatus )
		return erOperationFailed;

	return slReadInputStatus(query,reply);
}

// -------------------------------------------------------------------------

mbErrCode ModbusTCPServerSlot::readOutputRegisters( ReadOutputMessage& query, 
												ReadOutputRetMessage& reply )
{
	if( !slReadOutputs )
		return erOperationFailed;

	return slReadOutputs(query,reply);
}

// -------------------------------------------------------------------------
mbErrCode ModbusTCPServerSlot::readInputRegisters( ReadInputMessage& query, 
												ReadInputRetMessage& reply )
{
	if( !slReadInputs )
		return erOperationFailed;

	return slReadInputs(query,reply);
}

// -------------------------------------------------------------------------
mbErrCode ModbusTCPServerSlot::forceMultipleCoils( ForceCoilsMessage& query, 
												ForceCoilsRetMessage& reply )
{
	if( !slForceCoils )
		return erOperationFailed;

	return slForceCoils(query,reply);
}

// -------------------------------------------------------------------------

mbErrCode ModbusTCPServerSlot::writeOutputRegisters( WriteOutputMessage& query, 
												WriteOutputRetMessage& reply )
{
	if( !slWriteOutputs )
		return erOperationFailed;

	return slWriteOutputs(query,reply);
}

// -------------------------------------------------------------------------
mbErrCode ModbusTCPServerSlot::forceSingleCoil( ForceSingleCoilMessage& query, 
											ForceSingleCoilRetMessage& reply )
{
	if( !slForceSingleCoil )
		return erOperationFailed;

	return slForceSingleCoil(query,reply);
}

// -------------------------------------------------------------------------
mbErrCode ModbusTCPServerSlot::writeOutputSingleRegister( WriteSingleOutputMessage& query, 
												WriteSingleOutputRetMessage& reply )
{
	if( !slWriteSingleOutputs )
		return erOperationFailed;

	return slWriteSingleOutputs(query,reply);
}

// -------------------------------------------------------------------------
mbErrCode ModbusTCPServerSlot::journalCommand( JournalCommandMessage& query, 
												JournalCommandRetMessage& reply )
{
	if( !slJournalCommand )
		return erOperationFailed;

	return slJournalCommand(query,reply);
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode ModbusTCPServerSlot::setDateTime( ModbusRTU::SetDateTimeMessage& query, 
									ModbusRTU::SetDateTimeRetMessage& reply )
{
	if( !slSetDateTime )
		return erOperationFailed;

	return slSetDateTime(query,reply);
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode ModbusTCPServerSlot::remoteService( ModbusRTU::RemoteServiceMessage& query, 
									ModbusRTU::RemoteServiceRetMessage& reply )
{
	if( !slRemoteService )
		return erOperationFailed;

	return slRemoteService(query,reply);
}									
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode ModbusTCPServerSlot::fileTransfer( ModbusRTU::FileTransferMessage& query, 
									ModbusRTU::FileTransferRetMessage& reply )
{
	if( !slFileTransfer )
		return erOperationFailed;

	return slFileTransfer(query,reply);
}									
// -------------------------------------------------------------------------
