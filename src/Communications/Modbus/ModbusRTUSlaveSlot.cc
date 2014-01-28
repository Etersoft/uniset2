#include "modbus/ModbusRTUSlaveSlot.h"
// -------------------------------------------------------------------------
using namespace ModbusRTU;
using namespace std;
// -------------------------------------------------------------------------
ModbusRTUSlaveSlot::ModbusRTUSlaveSlot( const std::string dev, bool use485, bool tr_ctl ):
	ModbusRTUSlave(dev,use485,tr_ctl)
{
}
// -------------------------------------------------------------------------
ModbusRTUSlaveSlot::ModbusRTUSlaveSlot( ComPort* c ):
	ModbusRTUSlave(c)
{
}
// -------------------------------------------------------------------------

ModbusRTUSlaveSlot::~ModbusRTUSlaveSlot()
{

}
// -------------------------------------------------------------------------
mbErrCode ModbusRTUSlaveSlot::readCoilStatus( ReadCoilMessage& query, 
												ReadCoilRetMessage& reply )
{
	if( !slReadCoil )
		return erOperationFailed;

	return slReadCoil(query,reply);
}

// -------------------------------------------------------------------------
mbErrCode ModbusRTUSlaveSlot::readInputStatus( ReadInputStatusMessage& query, 
												ReadInputStatusRetMessage& reply )
{
	if( !slReadInputStatus )
		return erOperationFailed;

	return slReadInputStatus(query,reply);
}

// -------------------------------------------------------------------------

mbErrCode ModbusRTUSlaveSlot::readOutputRegisters( ReadOutputMessage& query, 
												ReadOutputRetMessage& reply )
{
	if( !slReadOutputs )
		return erOperationFailed;

	return slReadOutputs(query,reply);
}

// -------------------------------------------------------------------------
mbErrCode ModbusRTUSlaveSlot::readInputRegisters( ReadInputMessage& query, 
												ReadInputRetMessage& reply )
{
	if( !slReadInputs )
		return erOperationFailed;

	return slReadInputs(query,reply);
}

// -------------------------------------------------------------------------
mbErrCode ModbusRTUSlaveSlot::forceMultipleCoils( ForceCoilsMessage& query, 
												ForceCoilsRetMessage& reply )
{
	if( !slForceCoils )
		return erOperationFailed;

	return slForceCoils(query,reply);
}

// -------------------------------------------------------------------------

mbErrCode ModbusRTUSlaveSlot::writeOutputRegisters( WriteOutputMessage& query, 
												WriteOutputRetMessage& reply )
{
	if( !slWriteOutputs )
		return erOperationFailed;

	return slWriteOutputs(query,reply);
}

// -------------------------------------------------------------------------
mbErrCode ModbusRTUSlaveSlot::diagnostics( DiagnosticMessage& query, 
												DiagnosticRetMessage& reply )
{
	if( !slDiagnostics )
		return erOperationFailed;

	return slDiagnostics(query,reply);
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode ModbusRTUSlaveSlot::read4314( ModbusRTU::MEIMessageRDI& query, 
										ModbusRTU::MEIMessageRetRDI& reply )
{
	if( !slMEIRDI )
		return erOperationFailed;

	return slMEIRDI(query,reply);
}
// -------------------------------------------------------------------------
mbErrCode ModbusRTUSlaveSlot::forceSingleCoil( ForceSingleCoilMessage& query, 
											ForceSingleCoilRetMessage& reply )
{
	if( !slForceSingleCoil )
		return erOperationFailed;

	return slForceSingleCoil(query,reply);
}

// -------------------------------------------------------------------------
mbErrCode ModbusRTUSlaveSlot::writeOutputSingleRegister( WriteSingleOutputMessage& query, 
												WriteSingleOutputRetMessage& reply )
{
	if( !slWriteSingleOutputs )
		return erOperationFailed;

	return slWriteSingleOutputs(query,reply);
}

// -------------------------------------------------------------------------
mbErrCode ModbusRTUSlaveSlot::journalCommand( JournalCommandMessage& query, 
												JournalCommandRetMessage& reply )
{
	if( !slJournalCommand )
		return erOperationFailed;

	return slJournalCommand(query,reply);
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode ModbusRTUSlaveSlot::setDateTime( ModbusRTU::SetDateTimeMessage& query, 
									ModbusRTU::SetDateTimeRetMessage& reply )
{
	if( !slSetDateTime )
		return erOperationFailed;

	return slSetDateTime(query,reply);
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode ModbusRTUSlaveSlot::remoteService( ModbusRTU::RemoteServiceMessage& query, 
									ModbusRTU::RemoteServiceRetMessage& reply )
{
	if( !slRemoteService )
		return erOperationFailed;

	return slRemoteService(query,reply);
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode ModbusRTUSlaveSlot::fileTransfer( ModbusRTU::FileTransferMessage& query, 
									ModbusRTU::FileTransferRetMessage& reply )
{
	if( !slFileTransfer )
		return erOperationFailed;

	return slFileTransfer(query,reply);
}
// -------------------------------------------------------------------------
void ModbusRTUSlaveSlot::sigterm( int signo )
{
	try
	{
		terminate();
	}
	catch(...){}
}
// -------------------------------------------------------------------------
