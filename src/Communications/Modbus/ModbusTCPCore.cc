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
size_t ModbusTCPCore::readNextData( ost::TCPStream* tcp,
									std::queue<unsigned char>& qrecv, int max )
{
	if( !tcp || !tcp->isConnected() )
		return 0;

	int i = 0;

	for( ; i < max; i++ )
	{
		char c;
		tcp->read(&c, sizeof(c));

		if( tcp->gcount() <= 0 )
			break;

		qrecv.push( (unsigned char)(c) );
	}

	return i;
}
// ------------------------------------------------------------------------
size_t ModbusTCPCore::getNextData(ost::TCPStream* tcp,
								  std::queue<unsigned char>& qrecv,
								  unsigned char* buf, size_t len )
{
	if( !tcp || !tcp->isConnected() )
		return 0;

	if( qrecv.empty() )
	{
		if( len <= 0 )
			len = 7;

		int ret = ModbusTCPCore::readNextData(tcp, qrecv, len);

		if( ret <= 0 )
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
mbErrCode ModbusTCPCore::sendData(ost::TCPStream* tcp, unsigned char* buf, size_t len )
{
	if( !tcp || !tcp->isConnected() )
		return erTimeOut;

	try
	{
		for( size_t i = 0; i < len; i++ )
			(*tcp) << buf[i];

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
