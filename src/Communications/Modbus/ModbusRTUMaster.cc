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
#include <string.h>
#include <errno.h>
#include <iostream>
#include <sstream>
#include "Exceptions.h"
#include "ComPort485F.h"
#include "modbus/ModbusRTUMaster.h"
// -------------------------------------------------------------------------
namespace uniset
{
	// -------------------------------------------------------------------------
	using namespace std;
	using namespace ModbusRTU;
	using namespace uniset;

	// -------------------------------------------------------------------------
	ModbusRTUMaster::ModbusRTUMaster( const string& dev, bool use485, bool tr_ctl ):
		port(NULL),
		myport(true)
	{
		if( use485 )
		{
#ifndef DISABLE_COMPORT_485F
			ComPort485F* cp;

			if( dev == "/dev/ttyS2" )
				cp = new ComPort485F(dev, 5, tr_ctl);
			else if( dev == "/dev/ttyS3" )
				cp = new ComPort485F(dev, 6, tr_ctl);
			else
				throw Exception("Open ComPort FAILED! dev must be /dev/ttyS2 or /dev/tytS3");

			port = cp;
#else
			throw Exception("Open ComPort485F FAILED! DISABLE_COMPORT_485F");
#endif // #ifndef DISABLE_COMPORT_485F
		}
		else
			port = new ComPort(dev);

		port->setSpeed(ComPort::ComSpeed38400);
		port->setParity(ComPort::NoParity);
		port->setCharacterSize(ComPort::CSize8);
		port->setStopBits(ComPort::OneBit);
		port->setWaiting(true);
		port->setTimeout(replyTimeOut_ms);
		//    port->setBlocking(false);
	}
	// -------------------------------------------------------------------------
	ModbusRTUMaster::ModbusRTUMaster( ComPort* com ):
		port(com),
		myport(false)
	{
		if( !port )
			throw Exception("comport=NULL!!");

		port->setSpeed(ComPort::ComSpeed38400);
		port->setParity(ComPort::NoParity);
		port->setCharacterSize(ComPort::CSize8);
		port->setStopBits(ComPort::OneBit);
		port->setWaiting(true);
		port->setTimeout(replyTimeOut_ms);
		//    port->setBlocking(false);
	}
	// -------------------------------------------------------------------------
	ModbusRTUMaster::~ModbusRTUMaster()
	{
		if( myport )
			delete port;
	}
	// -------------------------------------------------------------------------
	void ModbusRTUMaster::cleanupChannel()
	{
		if( port )
			port->cleanupChannel();
	}
	// -------------------------------------------------------------------------
	void ModbusRTUMaster::setSpeed( ComPort::Speed s )
	{
		if( port != NULL )
			port->setSpeed(s);
	}
	// --------------------------------------------------------------------------------
	void ModbusRTUMaster::setSpeed( const std::string& s )
	{
		if( port != NULL )
			port->setSpeed(s);
	}
	// --------------------------------------------------------------------------------
	ComPort::Speed ModbusRTUMaster::getSpeed()
	{
		if( port == NULL )
			return ComPort::ComSpeed0;

		return port->getSpeed();
	}
	// -------------------------------------------------------------------------
	ComPort::Parity ModbusRTUMaster::getParity()
	{
		if( port != NULL)
			port->getParity();

		return ComPort::NoParity;
	}
    // -------------------------------------------------------------------------
	void ModbusRTUMaster::setParity( ComPort::Parity parity )
	{
		if( port != NULL)
			port->setParity(parity);
	}
	// -------------------------------------------------------------------------
	void ModbusRTUMaster::setCharacterSize( ComPort::CharacterSize csize )
	{
		if( port != NULL)
			port->setCharacterSize(csize);
	}
	// -------------------------------------------------------------------------
    ComPort::CharacterSize ModbusRTUMaster::getCharacterSize()
    {
        if( port != NULL)
            return port->getCharacterSize();

        return ComPort::CSize8;
    }
    // -------------------------------------------------------------------------
    ComPort::StopBits ModbusRTUMaster::getStopBits()
    {
        if( port != NULL)
            port->getStopBits();

        return ComPort::OneBit;
    }
    // -------------------------------------------------------------------------
	void ModbusRTUMaster::setStopBits( ComPort::StopBits sBit )
	{
		if( port != NULL)
			port->setStopBits(sBit);
	}
	// -------------------------------------------------------------------------
	timeout_t ModbusRTUMaster::getTimeout() const
	{
		if( port == NULL )
			return 0;

		return port->getTimeout();
	}
	// -------------------------------------------------------------------------
	size_t ModbusRTUMaster::getNextData( unsigned char* buf, size_t len )
	{
		//    if( !port ) return 0;
		return port->receiveBlock(buf, len);
	}
	// --------------------------------------------------------------------------------
	void ModbusRTUMaster::setChannelTimeout( timeout_t msec )
	{
		if( port )
			port->setTimeout(msec);
	}
	// --------------------------------------------------------------------------------
	mbErrCode ModbusRTUMaster::sendData(unsigned char* buf, size_t len )
	{
		try
		{
			port->sendBlock(buf, len);
		}
		catch( const uniset::Exception& ex ) // SystemError
		{
			dlog->crit() << "(send): " << ex << endl;
			return erHardwareError;
		}

		return erNoError;
	}
	// -------------------------------------------------------------------------
	mbErrCode ModbusRTUMaster::query( ModbusAddr addr, ModbusMessage& msg,
									  ModbusMessage& reply, timeout_t timeout )
	{
		mbErrCode res = send(msg);

		if( res != erNoError )
			return res;

		return recv(addr, msg.pduhead.func, reply, timeout);
	}
	// --------------------------------------------------------------------------------
} // end of namespace uniset
