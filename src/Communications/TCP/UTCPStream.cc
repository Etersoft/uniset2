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
	Poco::Net::StreamSocket::getLinger(on,sec);
	return on;
}
// -------------------------------------------------------------------------
void UTCPStream::forceDisconnect()
{
	bool on;
	int sec;
	Poco::Net::StreamSocket::getLinger(on,sec);
	setLinger(false,0);
	shutdown();
	Poco::Net::StreamSocket::setLinger(on,sec);
}
// -------------------------------------------------------------------------
bool UTCPStream::setNoDelay(bool enable)
{
	Poco::Net::StreamSocket::setNoDelay(enable);
	return (Poco::Net::StreamSocket::getNoDelay() == enable);
}
// -------------------------------------------------------------------------
ssize_t UTCPStream::writeData(const void* buf, size_t len, timeout_t t)
{
	return Poco::Net::StreamSocket::sendBytes(buf, len);
}
// -------------------------------------------------------------------------
ssize_t UTCPStream::readData(void* buf, size_t len, char separator, timeout_t t)
{
	return Poco::Net::StreamSocket::receiveBytes(buf, len);
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
	Poco::Net::SocketAddress sa(hname,port);
	connect(sa,tout_msec*1000);
	setKeepAlive(true);
	Poco::Net::StreamSocket::setLinger(true,1);
	setKeepAliveParams();
}
// -------------------------------------------------------------------------
bool UTCPStream::isConnected()
{
	//return ( Poco::Net::StreamSocket::sockfd() > 0 );
	return ( Poco::Net::StreamSocket::peerAddress().addr() != 0 );
}
// -------------------------------------------------------------------------
