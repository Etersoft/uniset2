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
