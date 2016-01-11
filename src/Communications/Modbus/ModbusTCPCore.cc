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
#include "modbus/ModbusTCPCore.h"
// -------------------------------------------------------------------------
using namespace std;
using namespace ModbusRTU;
// -------------------------------------------------------------------------
size_t ModbusTCPCore::readNextData(UTCPStream* tcp,
								   std::queue<unsigned char>& qrecv, size_t max, timeout_t t )
{
	if( !tcp || !tcp->isConnected() )
		return 0;

	size_t i = 0;

	try
	{
		for( ; i < max; i++ )
		{
			unsigned char c;
			ssize_t l = tcp->readData(&c, sizeof(c), 0, t);

			if( l <= 0 )
				break;

			qrecv.push(c);
		}
	}
	catch( ost::SockException& e )
	{
	}

	return i;
}
// ------------------------------------------------------------------------
size_t ModbusTCPCore::getNextData(UTCPStream* tcp,
								  std::queue<unsigned char>& qrecv,
								  unsigned char* buf, size_t len, timeout_t t )
{
	if( !tcp || !tcp->isConnected() )
		return 0;

	if( qrecv.empty() )
	{
		if( len <= 0 )
			len = 7;

		size_t ret = ModbusTCPCore::readNextData(tcp, qrecv, len, t);

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
	size_t i = 0;
#if 1

	for( size_t a = 0; a < attempts; a++ )
	{
		for( ; i < max; i++ )
		{
			// читаем один символ за раз..
			unsigned char c;
			ssize_t l = ::read(fd, &c, sizeof(c));

			if( l <= 0 )
				break;

			qrecv.push(c);
		}
	}

#else
	char* buf = new char[max];
	ssize_t l = 0;

	for( size_t a = 0; a < attempts; a++ )
	{
		l = ::read(fd, buf, sizeof(buf));

		if( l > 0 )
			break;
	}

	if( l > 0 )
	{
		for( int i = 0; i < l; i++ )
			qrecv.push(buf[i]);
	}

	delete [] buf;
#endif

	return ( qrecv.size() >= max ? max : qrecv.size() );
}
// ------------------------------------------------------------------------
size_t ModbusTCPCore::getDataFD( int fd, std::queue<unsigned char>& qrecv,
								 unsigned char* buf, size_t len, size_t attempts )
{
	if( qrecv.empty()  || qrecv.size() < len )
	{
		if( len == 0 )
			len = 7;

		size_t ret = ModbusTCPCore::readDataFD(fd, qrecv, len, attempts);

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
mbErrCode ModbusTCPCore::sendData( UTCPStream* tcp, unsigned char* buf, size_t len, timeout_t t )
{
	if( !tcp || !tcp->isConnected() )
		return erTimeOut;

	try
	{
		ssize_t l = tcp->writeData(buf, len, t);

		if( l == len )
			return erNoError;
	}
	catch( ost::SockException& e )
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
