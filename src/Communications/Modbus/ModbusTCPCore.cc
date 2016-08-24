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
#include <Poco/Net/NetException.h>
#include "modbus/ModbusTCPCore.h"
#include "Exceptions.h"
// -------------------------------------------------------------------------
using namespace std;
using namespace ModbusRTU;
// -------------------------------------------------------------------------
#define USE_BUFFER_FOR_READ 1
#define DEFAULT_BUFFER_SIZE_FOR_READ 255
// -------------------------------------------------------------------------
size_t ModbusTCPCore::readNextData(UTCPStream* tcp,
								   std::queue<unsigned char>& qrecv, size_t max )
{
	if( !tcp ) // || !tcp->available() )
		return 0;

	size_t i = 0;
	bool commfail = false;

#ifdef USE_BUFFER_FOR_READ

	max = std::max(max, (size_t)DEFAULT_BUFFER_SIZE_FOR_READ);

	char* buf = new char[max];

	if( buf == 0 )
		return 0;

	try
	{
		ssize_t l = tcp->receiveBytes(buf, max);

		if( l > 0 )
		{
			for( ssize_t k = 0; k < l; k++ )
				qrecv.push(buf[k]);

			i = l;
		}

		// канал закрыт!
		if( l == 0 )
			commfail = true;
	}
	catch( Poco::TimeoutException& ex )
	{

	}
	catch( Poco::Net::NetException& e )
	{
		commfail = true;
	}

	delete [] buf;
#else

	try
	{
		for( ; i < max; i++ )
		{
			// (не оптимально): читаем один символ за раз..
			unsigned char c;
			ssize_t l = tcp->readData(&c, sizeof(c), 0, t);

			if( l == 0 )
			{
				commfail = true;
				break;
			}

			if( l < 0 )
				break;

			qrecv.push(c);
		}
	}
	catch( ost::SockException& e )
	{
	}

#endif

	if( commfail )
		throw UniSetTypes::CommFailed();

	return i;
}
// ------------------------------------------------------------------------
size_t ModbusTCPCore::getNextData(UTCPStream* tcp,
								  std::queue<unsigned char>& qrecv,
								  unsigned char* buf, size_t len)
{
	if( qrecv.empty() || qrecv.size() < len )
	{
		if( !tcp ) // || !tcp->available() )
			return 0;

		if( len <= 0 )
			len = 7;

		size_t ret = ModbusTCPCore::readNextData(tcp, qrecv, len);

		if( ret == 0 )
			return 0;
	}

	size_t i = 0;

	for( ; i < len && !qrecv.empty(); i++ )
	{
		buf[i] = qrecv.front();
		qrecv.pop();
	}

	return i;
}
// -------------------------------------------------------------------------
size_t ModbusTCPCore::readDataFD( int fd, std::queue<unsigned char>& qrecv, size_t max , size_t attempts )
{
	bool commfail = false;

#ifdef USE_BUFFER_FOR_READ

	max = std::max(max, (size_t)DEFAULT_BUFFER_SIZE_FOR_READ);

	char* buf = new char[max];

	if( buf == 0 )
		return 0;

	ssize_t l = 0;
	size_t cnt = 0;

	for( size_t a = 0; a < attempts; a++ )
	{
		l = ::read(fd, buf, max);

		if( l > 0 )
		{
			for( int k = 0; k < l; k++ )
				qrecv.push(buf[k]);

			cnt += l;

			if( cnt >= max )
				break;
		}

		// канал закрыт!
		if( l == 0 )
			commfail = true;
	}

	delete [] buf;
#else

	size_t i = 0;

	for( size_t a = 0; a < attempts; a++ )
	{
		for( ; i < max; i++ )
		{
			// (не оптимально): читаем один символ за раз..
			unsigned char c;
			ssize_t l = ::read(fd, &c, sizeof(c));

			if( l == 0 )
			{
				commfail = true;
				break;
			}

			if( l < 0 )
				break;

			qrecv.push(c);
		}
	}

#endif

	if( commfail )
		throw UniSetTypes::CommFailed();

	return std::min(qrecv.size(), max);
}
// ------------------------------------------------------------------------
size_t ModbusTCPCore::getDataFD( int fd, std::queue<unsigned char>& qrecv,
								 unsigned char* buf, size_t len, size_t attempts )
{
	if( qrecv.empty() || qrecv.size() < len )
	{
		if( len == 0 )
			len = 7;

		size_t ret = ModbusTCPCore::readDataFD(fd, qrecv, len, attempts);

		if( ret == 0 && qrecv.empty() )
			return 0;
	}

	size_t i = 0;

	for( ; i < len && !qrecv.empty(); i++ )
	{
		buf[i] = qrecv.front();
		qrecv.pop();
	}

	return i;
}
// -------------------------------------------------------------------------
mbErrCode ModbusTCPCore::sendData(UTCPStream* tcp, unsigned char* buf, size_t len )
{
	if( !tcp ) // || !tcp->available() )
		return erTimeOut;

	try
	{
		ssize_t l = tcp->sendBytes(buf, len);

		if( l == len )
			return erNoError;
	}
	catch( Poco::Net::NetException& e )
	{
		//        cerr << "(send): " << e.getString() << ": " << e.getSystemErrorString() << endl;
	}
	catch(...)
	{
		//        cerr << "(send): cath..." << endl;
	}

	return erHardwareError;
}
// -------------------------------------------------------------------------
mbErrCode ModbusTCPCore::sendDataFD( int fd, unsigned char* buf, size_t len )
{
	ssize_t l = ::write(fd, buf, len);

	if( l == len )
		return erNoError;

	return erHardwareError;
}
// -------------------------------------------------------------------------
