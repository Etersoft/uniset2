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
#include "modbus/ModbusHelpers.h"
#include "modbus/ModbusRTUMaster.h"
#include "Exceptions.h"
#include "ComPort.h"
// -------------------------------------------------------------------------
using namespace std;
using namespace ModbusRTU;
using namespace UniSetTypes;
// -------------------------------------------------------------------------
static ComPort::Speed checkSpeed[] =
{
	ComPort::ComSpeed9600,
	ComPort::ComSpeed19200,
	ComPort::ComSpeed38400,
	ComPort::ComSpeed57600,
	ComPort::ComSpeed4800,
	ComPort::ComSpeed115200,
	ComPort::ComSpeed0
};
// -------------------------------------------------------------------------
ModbusAddr ModbusHelpers::autodetectSlave( ModbusRTUMaster* m,
		ModbusAddr beg, ModbusAddr end,
		ModbusData reg,
		SlaveFunctionCode fn )
{
	if( beg > end )
	{
		ModbusAddr tmp = beg;
		beg = end;
		end = tmp;
	}

	for( ModbusAddr a = beg; a <= end; a++ )
	{
		try
		{
			if( fn == fnReadInputRegisters )
			{
				m->read04(a, reg, 1);
			}
			else if( fn == fnReadInputStatus )
			{
				m->read02(a, reg, 1);
			}
			else if( fn == fnReadCoilStatus )
			{
				m->read01(a, reg, 1);
			}
			else if( fn == fnReadOutputRegisters )
			{
				m->read03(a, reg, 1);
			}
			else
				throw mbException(erOperationFailed);

			return a;
		}
		catch( ModbusRTU::mbException& ex )
		{
			if( ex.err < erInternalErrorCode )
				return a; // узел ответил ошибкой (но связь то есть)
		}
		catch(...) {}

		if( (beg == 0xff) || (end == 0xff) )
			break;
	}

	throw TimeOut();
}
// -------------------------------------------------------------------------

ModbusAddr ModbusHelpers::autodetectSlave(  std::string dev, ComPort::Speed s, int tout,
		ModbusAddr beg, ModbusAddr end,
		ModbusData reg,
		SlaveFunctionCode fn )
{
	ModbusRTUMaster mb(dev);
	mb.setSpeed(s);
	mb.setTimeout(tout);
	return autodetectSlave( &mb, beg, end, reg, fn );
}
// -------------------------------------------------------------------------

ComPort::Speed ModbusHelpers::autodetectSpeed( ModbusRTUMaster* m, ModbusAddr slave,
		ModbusData reg, SlaveFunctionCode fn )
{
	ComPort::Speed cur = m->getSpeed();
	ComPort::Speed s = ComPort::ComSpeed0;

	for( unsigned int i = 0; checkSpeed[i] != ComPort::ComSpeed0; i++ )
	{
		try
		{
			m->setSpeed( checkSpeed[i] );

			if( fn == fnReadInputRegisters )
			{
				m->read04(slave, reg, 1);
			}
			else if( fn == fnReadInputStatus )
			{
				m->read02(slave, reg, 1);
			}
			else if( fn == fnReadCoilStatus )
			{
				m->read01(slave, reg, 1);
			}
			else if( fn == fnReadOutputRegisters )
			{
				m->read03(slave, reg, 1);
			}
			else
				throw mbException(erOperationFailed);


			s = checkSpeed[i];
			break;
		}
		catch( ModbusRTU::mbException& ex )
		{
			if( ex.err < erInternalErrorCode )
			{
				s = checkSpeed[i];
				break; // узел ответил ошибкой (но связь то есть)
			}
		}
		catch(...) {}
	}

	m->setSpeed(cur);

	if( s != ComPort::ComSpeed0 )
		return s;

	throw TimeOut();
}
// -------------------------------------------------------------------------
ComPort::Speed ModbusHelpers::autodetectSpeed( std::string dev, ModbusRTU::ModbusAddr slave, int tout,
		ModbusData reg, SlaveFunctionCode fn )
{
	ModbusRTUMaster mb(dev);
	mb.setTimeout(tout);
	return autodetectSpeed( &mb, slave, reg, fn );
}
// -------------------------------------------------------------------------
