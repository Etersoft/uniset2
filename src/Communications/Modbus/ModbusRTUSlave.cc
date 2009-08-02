/*! $Id: ModbusRTUSlave.cc,v 1.3 2009/02/24 20:27:25 vpashka Exp $ */
// -------------------------------------------------------------------------
#include <sys/time.h>
#include <cstdlib>
#include <errno.h>
#include <iostream>
#include <sstream>
#include "Exceptions.h"
#include "ComPort485F.h"
#include "modbus/ModbusRTUSlave.h"
// -------------------------------------------------------------------------
using namespace std;
using namespace ModbusRTU;
using namespace UniSetTypes;
// -------------------------------------------------------------------------
ModbusRTUSlave::ModbusRTUSlave( const string dev, bool use485 ):
	port(NULL),
	myport(true)
{
	if( use485 )
	{
		ComPort485F* cp;
		if( dev == "/dev/ttyS2" )
			cp = new ComPort485F(dev,5);
		else if( dev == "/dev/ttyS3" )
			cp = new ComPort485F(dev,6);
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
	port->setTimeout(recvTimeOut_ms*1000);
//	port->setBlocking(false); 
}

// -------------------------------------------------------------------------
ModbusRTUSlave::ModbusRTUSlave( ComPort* com ):
	port(com),
	myport(false)
{
	port->setSpeed(ComPort::ComSpeed38400);
	port->setParity(ComPort::NoParity);
	port->setCharacterSize(ComPort::CSize8);
	port->setStopBits(ComPort::OneBit);
	port->setWaiting(true);
	port->setTimeout(recvTimeOut_ms*1000);
//	port->setBlocking(false); 
}

// -------------------------------------------------------------------------
ModbusRTUSlave::~ModbusRTUSlave()
{
	if( myport )
		delete port;
}
// -------------------------------------------------------------------------
mbErrCode ModbusRTUSlave::receive( ModbusRTU::ModbusAddr addr, int timeout )
{
	uniset_mutex_lock lck(recvMutex,timeout);
	ModbusMessage buf;
	mbErrCode res = erBadReplyNodeAddress;
	do
	{
		res = recv(addr,buf,timeout);
		if( res != erNoError && res != erBadReplyNodeAddress )
		{
			// Если ошибка подразумевает посылку ответа с сообщением об ошибке
			// то посылаем
			if( res < erInternalErrorCode )
			{
				ErrorRetMessage em( buf.addr, buf.func, res ); 
				buf = em.transport_msg();
				send(buf);
				printProcessingTime();
			}

//			dlog[Debug::WARN] << "(receive): " << mbErr2Str(res) << endl;
			usleep(10000);
			return res;
		}

		// если полученный пакет адресован 
		// не данному узлу (и не широковещательный)
		// то ждать следующий...
	}
	while( res == erBadReplyNodeAddress );

	if( res!=erNoError )
		return res;

	return processing(buf);
}

// --------------------------------------------------------------------------------
ComPort::Speed ModbusRTUSlave::getSpeed()
{
	if( port == NULL )
		return ComPort::ComSpeed0;
	
	return port->getSpeed();
}
// -------------------------------------------------------------------------

void ModbusRTUSlave::setSpeed( ComPort::Speed s )
{
	if( port != NULL )
		port->setSpeed(s);
}
// --------------------------------------------------------------------------------
void ModbusRTUSlave::setSpeed( const std::string s )
{
	if( port != NULL )
		port->setSpeed(s);
}
// --------------------------------------------------------------------------------
int ModbusRTUSlave::getNextData( unsigned char* buf, int len )
{
//	if( !port ) return 0;
	return port->receiveBlock(buf, len);
}
// --------------------------------------------------------------------------------
void ModbusRTUSlave::setChannelTimeout( int msec )
{
	if( msec == UniSetTimer::WaitUpTime )
		port->setTimeout(15*60*1000*1000); // используем просто большое время (15 минут). Переведя его в наносекунды.
	else
		port->setTimeout(msec*1000);
}
// --------------------------------------------------------------------------------
mbErrCode ModbusRTUSlave::sendData( unsigned char* buf, int len )
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
