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
#include <iostream>
#include <string>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <Poco/Net/NetException.h>
#include "UTCPStream.h"
#include "PassiveTimer.h"
#include "UniSetTypes.h"
#include "UTCPCore.h"
// -------------------------------------------------------------------------
using namespace std;
// -------------------------------------------------------------------------
UTCPStream::~UTCPStream()
{

}
// -------------------------------------------------------------------------
UTCPStream::UTCPStream(const Poco::Net::StreamSocket& so):
	Poco::Net::StreamSocket(so)
{

}

UTCPStream::UTCPStream()
{
}
// -------------------------------------------------------------------------
bool UTCPStream::setKeepAliveParams(timeout_t timeout_sec, int keepcnt, int keepintvl )
{
	return UTCPCore::setKeepAliveParams(Poco::Net::StreamSocket::sockfd(), timeout_sec, keepcnt, keepintvl);
}
// -------------------------------------------------------------------------
bool UTCPStream::isSetLinger() const
{
	bool on;
	int sec;
	Poco::Net::StreamSocket::getLinger(on, sec);
	return on;
}
// -------------------------------------------------------------------------
void UTCPStream::forceDisconnect()
{
	try
	{
		bool on;
		int sec;
		Poco::Net::StreamSocket::getLinger(on, sec);
		setLinger(false, 0);
		close();
		//shutdown();
		Poco::Net::StreamSocket::setLinger(on, sec);
	}
	catch( Poco::Net::NetException& )
	{

	}
}
// -------------------------------------------------------------------------
int UTCPStream::getSocket() const
{
	return Poco::Net::StreamSocket::sockfd();
}
// -------------------------------------------------------------------------
timeout_t UTCPStream::getTimeout() const
{
	auto tm = Poco::Net::StreamSocket::getReceiveTimeout();
	return tm.microseconds();
}
// -------------------------------------------------------------------------
void UTCPStream::create(const std::string& hname, int port, timeout_t tout_msec )
{
	Poco::Net::SocketAddress sa(hname, port);
	connect(sa, tout_msec * 1000);
	setKeepAlive(true);
	Poco::Net::StreamSocket::setLinger(true, 1);
	setKeepAliveParams();
}
// -------------------------------------------------------------------------
bool UTCPStream::isConnected()
{
	return ( Poco::Net::StreamSocket::sockfd() > 0 );
/*
	try
	{
		// Вариант 1
		//return ( Poco::Net::StreamSocket::peerAddress().addr() != 0 );

		// Варинт 2
		return ( Poco::Net::StreamSocket::peerAddress().port() != 0 );

		// Вариант 3
//		if( poll({0, 5}, Poco::Net::Socket::SELECT_READ) )
//			return (tcp->available() > 0);
	}
	catch( Poco::Net::NetException& ex )
	{
	}

	return false;
*/
}
// -------------------------------------------------------------------------
