/*
 * Copyright (c) 2015 Pavel Vainerman.
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
// -------------------------------------------------------------------------
#include "modbus/ModbusServerSlot.h"
// -------------------------------------------------------------------------
namespace uniset
{
	// -------------------------------------------------------------------------
	using namespace ModbusRTU;
	using namespace std;
	// -------------------------------------------------------------------------
	ModbusServerSlot::ModbusServerSlot()
	{
	}
	// -------------------------------------------------------------------------
	ModbusServerSlot::~ModbusServerSlot()
	{

	}
	// -------------------------------------------------------------------------
	void ModbusServerSlot::connectReadCoil( ReadCoilSlot sl )
	{
		slReadCoil = sl;
	}
	// -------------------------------------------------------------------------
	void ModbusServerSlot::connectReadInputStatus( ReadInputStatusSlot sl )
	{
		slReadInputStatus = sl;
	}
	// -------------------------------------------------------------------------

	void ModbusServerSlot::connectReadOutput( ReadOutputSlot sl )
	{
		slReadOutputs = sl;
	}
	// -------------------------------------------------------------------------
	void ModbusServerSlot::connectReadInput( ReadInputSlot sl )
	{
		slReadInputs = sl;
	}
	// -------------------------------------------------------------------------
	void ModbusServerSlot::connectForceCoils( ForceCoilsSlot sl )
	{
		slForceCoils = sl;
	}
	// -------------------------------------------------------------------------
	void ModbusServerSlot::connectWriteOutput( WriteOutputSlot sl )
	{
		slWriteOutputs = sl;
	}

	// ------------------------------------------------------------------------
	void ModbusServerSlot::connectWriteSingleOutput( WriteSingleOutputSlot sl )
	{
		slWriteSingleOutputs = sl;
	}

	// -------------------------------------------------------------------------
	void ModbusServerSlot::connectDiagnostics( DiagnosticsSlot sl )
	{
		slDiagnostics = sl;
	}
	// -------------------------------------------------------------------------
	void ModbusServerSlot::connectMEIRDI( MEIRDISlot sl )
	{
		slMEIRDI = sl;
	}
	// -------------------------------------------------------------------------
	void ModbusServerSlot::connectForceSingleCoil( ForceSingleCoilSlot sl )
	{
		slForceSingleCoil = sl;
	}
	// -------------------------------------------------------------------------

	void ModbusServerSlot::connectJournalCommand( JournalCommandSlot sl )
	{
		slJournalCommand = sl;
	}

	// -------------------------------------------------------------------------
	void ModbusServerSlot::connectSetDateTime( SetDateTimeSlot sl )
	{
		slSetDateTime = sl;
	}
	// -------------------------------------------------------------------------
	void ModbusServerSlot::connectRemoteService( RemoteServiceSlot sl )
	{
		slRemoteService = sl;
	}
	// -------------------------------------------------------------------------
	void ModbusServerSlot::connectFileTransfer( FileTransferSlot sl )
	{
		slFileTransfer = sl;
	}
	// -------------------------------------------------------------------------
} // end of namespace uniset
