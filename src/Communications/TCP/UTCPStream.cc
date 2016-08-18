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
#include <cc++/socket.h>
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
UTCPStream::UTCPStream():
	TCPStream(ost::Socket::IPV4, true)
{
}
// -------------------------------------------------------------------------
bool UTCPStream::setKeepAliveParams(timeout_t timeout_sec, int keepcnt, int keepintvl )
{
	return UTCPCore::setKeepAliveParams(so, timeout_sec, keepcnt, keepintvl);
}
// -------------------------------------------------------------------------
bool UTCPStream::isSetLinger() const
{
	return Socket::flags.linger;
}
// -------------------------------------------------------------------------
void UTCPStream::forceDisconnect()
{
	bool f = Socket::flags.linger;
	Socket::flags.linger = false;
	disconnect();
	Socket::flags.linger = f;
}
// -------------------------------------------------------------------------
bool UTCPStream::setNoDelay(bool enable)
{
	return ( TCPStream::setNoDelay(enable) == 0 );
}
// -------------------------------------------------------------------------
ssize_t UTCPStream::writeData(const void* buf, size_t len, timeout_t t)
{
	return TCPStream::writeData(buf, len, t);
}
// -------------------------------------------------------------------------
ssize_t UTCPStream::readData(void* buf, size_t len, char separator, timeout_t t)
{
	return TCPStream::readData(buf, len, separator, t);
}
// -------------------------------------------------------------------------
int UTCPStream::getSocket() const
{
	return TCPStream::so;
}
// -------------------------------------------------------------------------
timeout_t UTCPStream::getTimeout() const
{
	return TCPStream::timeout;
}
// -------------------------------------------------------------------------
void UTCPStream::create( const std::string& hname, int port, bool throwflag, timeout_t t )
{
	family = ost::Socket::IPV4;
	timeout = t;
	unsigned mss = 536;
	setError(throwflag);
	ost::IPV4Host h(hname.c_str());
	connect(h, port, mss);
	setKeepAlive(true);
	setLinger(true);
	setKeepAliveParams();
}
// -------------------------------------------------------------------------
