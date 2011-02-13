// -------------------------------------------------------------------------
#ifndef ModbusHelpers_H_
#define ModbusHelpers_H_
// -------------------------------------------------------------------------
#include <string>
#include "ModbusTypes.h"
#include "ComPort.h"
// -------------------------------------------------------------------------
class ModbusRTUMaster;
// -------------------------------------------------------------------------
namespace ModbusHelpers
{
	ModbusRTU::ModbusAddr autodetectSlave( ModbusRTUMaster* m, 
											ModbusRTU::ModbusAddr beg=0, 
											ModbusRTU::ModbusAddr end=255,
											ModbusRTU::ModbusData reg=0,
											ModbusRTU::SlaveFunctionCode fn=ModbusRTU::fnReadInputRegisters
											 ); // throw UniSetTypes::TimeOut();

	ModbusRTU::ModbusAddr autodetectSlave( std::string dev,
											ComPort::Speed s, int tout=1000,
											ModbusRTU::ModbusAddr beg=0, 
											ModbusRTU::ModbusAddr end=255,
											ModbusRTU::ModbusData reg=0,
											ModbusRTU::SlaveFunctionCode fn=ModbusRTU::fnReadInputRegisters
											 ); // throw UniSetTypes::TimeOut();

	ComPort::Speed autodetectSpeed( ModbusRTUMaster* m, ModbusRTU::ModbusAddr slave,
									ModbusRTU::ModbusData reg=0,
									ModbusRTU::SlaveFunctionCode fn=ModbusRTU::fnReadInputRegisters
									 ); // throw UniSetTypes::TimeOut();

	ComPort::Speed autodetectSpeed( std::string dev,
									ModbusRTU::ModbusAddr slave, 
									int timeout_msec=1000,
									ModbusRTU::ModbusData reg=0,
									ModbusRTU::SlaveFunctionCode fn=ModbusRTU::fnReadInputRegisters
									); // throw UniSetTypes::TimeOut();

} // end of namespace ModbusHelpers
// ---------------------------------------------------------------------------
#endif // ModbusHelpers_H_
// ---------------------------------------------------------------------------
