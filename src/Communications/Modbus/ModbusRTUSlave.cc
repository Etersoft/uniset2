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
ModbusRTUSlave::ModbusRTUSlave( const string& dev, bool use485, bool tr_ctl ):
	port(NULL),
	myport(true)
{
	recvMutex.setName("(ModbusRTUSlave): dev='" + dev + "' recvMutex:");

	if( use485 )
	{
		ComPort485F* cp;

		if( dev == "/dev/ttyS2" )
			cp = new ComPort485F(dev, 5, tr_ctl);
		else if( dev == "/dev/ttyS3" )
			cp = new ComPort485F(dev, 6, tr_ctl);
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
	port->setTimeout(recvTimeOut_ms);
	//    port->setBlocking(false);
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
	port->setTimeout(recvTimeOut_ms);
	//    port->setBlocking(false);
}

// -------------------------------------------------------------------------
ModbusRTUSlave::~ModbusRTUSlave()
{
	if( myport )
		delete port;
}
// -------------------------------------------------------------------------
mbErrCode ModbusRTUSlave::receive(const std::unordered_set<ModbusAddr>& vmbaddr, timeout_t timeout )
{
	uniset_mutex_lock lck(recvMutex, timeout);

	if( !lck.lock_ok() )
	{
		if( dlog->is_crit() )
			dlog->crit() << "(ModbusRTUSlave::receive): Don`t lock " << recvMutex << endl;

		return erTimeOut;
	}

	ModbusMessage buf;
	mbErrCode res = erBadReplyNodeAddress;

	do
	{
		res = recv(vmbaddr, buf, timeout);

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

			if( aftersend_msec >= 0 )
				msleep(aftersend_msec);
//			usleep(10000);
			return res;
		}

		// если полученный пакет адресован
		// не данному узлу (и не широковещательный)
		// то ждать следующий...
	}
	while( res == erBadReplyNodeAddress );

	if( res != erNoError )
		return res;

	return processing(buf);
}
// --------------------------------------------------------------------------------
mbErrCode ModbusRTUSlave::receive(ModbusAddr addr, timeout_t msec)
{
	return ModbusServer::receive(addr,msec);
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
void ModbusRTUSlave::setSpeed( const std::string& s )
{
	if( port != NULL )
		port->setSpeed(s);
}
// --------------------------------------------------------------------------------
int ModbusRTUSlave::getNextData( unsigned char* buf, int len )
{
	//    if( !port ) return 0;
	return port->receiveBlock(buf, len);
}
// --------------------------------------------------------------------------------
void ModbusRTUSlave::setChannelTimeout( timeout_t msec )
{
	if( msec == UniSetTimer::WaitUpTime )
		port->setTimeout(15 * 60 * 1000); // используем просто большое время (15 минут)
	else
		port->setTimeout(msec);
}
// --------------------------------------------------------------------------------
mbErrCode ModbusRTUSlave::sendData( unsigned char* buf, int len )
{
	try
	{
		port->sendBlock(buf, len);
	}
	catch( const Exception& ex ) // SystemError
	{
		if( dlog->is_crit() )
			dlog->crit() << "(send): " << ex << endl;

		return erHardwareError;
	}

	return erNoError;
}
// -------------------------------------------------------------------------
void ModbusRTUSlave::terminate()
{
	try
	{
	}
	catch(...) {}
}
