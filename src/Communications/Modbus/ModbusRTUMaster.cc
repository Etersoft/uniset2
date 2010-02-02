/*! $Id: ModbusRTUMaster.cc,v 1.3 2009/02/24 20:27:25 vpashka Exp $ */
// -------------------------------------------------------------------------
#include <string.h>
#include <errno.h>
#include <iostream>
#include <sstream>
#include "Exceptions.h"
#include "ComPort485F.h"
#include "modbus/ModbusRTUMaster.h"
// -------------------------------------------------------------------------
using namespace std;
using namespace ModbusRTU;
using namespace UniSetTypes;

// -------------------------------------------------------------------------
ModbusRTUMaster::ModbusRTUMaster( const string dev, bool use485, bool tr_ctl ):
	port(NULL),
	myport(true)
{
	if( use485 )
	{
		ComPort485F* cp;
		if( dev == "/dev/ttyS2" )
			cp = new ComPort485F(dev,5,tr_ctl);
		else if( dev == "/dev/ttyS3" )
			cp = new ComPort485F(dev,6,tr_ctl);
		else
			throw Exception("Open ComPort FAILED! dev must be /dev/ttyS2 or /dev/tytS3");

		port = cp;
	}
	else
		port = new ComPort(dev);

	port->setSpeed(ComPort::ComSpeed38400);
	port->setParity(ComPort::NoParity);
	port->setCharacterSize(ComPort::CSize8);
	port->setStopBits(ComPort::OneBit);
	port->setWaiting(true);
	port->setTimeout(replyTimeOut_ms*1000);
//	port->setBlocking(false); 
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
	port->setTimeout(replyTimeOut_ms*1000);
//	port->setBlocking(false); 
}
// -------------------------------------------------------------------------
ModbusRTUMaster::~ModbusRTUMaster()
{
	if( myport ) 
		delete port;
}
// -------------------------------------------------------------------------
void ModbusRTUMaster::setSpeed( ComPort::Speed s )
{
	if( port != NULL )
		port->setSpeed(s);
}
// --------------------------------------------------------------------------------
void ModbusRTUMaster::setSpeed( const std::string s )
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
void ModbusRTUMaster::setStopBits( ComPort::StopBits sBit )
{
	if( port != NULL)
		port->setStopBits(sBit);
}
// -------------------------------------------------------------------------
int ModbusRTUMaster::getTimeout()
{
	if( port == NULL )
		return 0;
	
	return port->getTimeout();
}
// -------------------------------------------------------------------------
int ModbusRTUMaster::getNextData( unsigned char* buf, int len )
{
//	if( !port ) return 0;
	return port->receiveBlock(buf, len);
}
// --------------------------------------------------------------------------------
void ModbusRTUMaster::setChannelTimeout( timeout_t msec )
{
	if( port )
		port->setTimeout(msec*1000);
}
// --------------------------------------------------------------------------------
mbErrCode ModbusRTUMaster::sendData( unsigned char* buf, int len )
{
	try
	{
		port->sendBlock(buf,len);
	}
	catch( Exception& ex ) // SystemError
	{
		dlog[Debug::CRIT] << "(send): " << ex << endl;
		return erHardwareError;
	}

	return erNoError;
}
// -------------------------------------------------------------------------
mbErrCode ModbusRTUMaster::query( ModbusAddr addr, ModbusMessage& msg, 
									ModbusMessage& reply, timeout_t timeout )
{
	mbErrCode res = send(msg);
	if( res!=erNoError )
		return res;

	return recv(addr,msg.func,reply,timeout);
}
// --------------------------------------------------------------------------------
