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

// glibc..
#include <netinet/tcp.h>
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
	SOCKET fd = TCPStream::so;
	int enable = 1;
	bool ok = true;

	if( setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void*)&enable, sizeof(enable)) == -1 )
		ok = false;

	if( setsockopt(fd, SOL_TCP, TCP_KEEPCNT, (void*) &keepcnt, sizeof(keepcnt)) == -1 )
		ok = false;

	if( setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, (void*) &keepintvl, sizeof (keepintvl)) == -1 )
		ok = false;

	if( setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, (void*) &timeout_sec, sizeof (timeout_sec)) == -1 )
		ok = false;

	return ok;
}
// -------------------------------------------------------------------------
bool UTCPStream::isSetLinger()
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
