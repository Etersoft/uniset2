#include <iostream>
#include <string>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <cc++/socket.h>
#include "UTCPSocket.h"
#include "PassiveTimer.h"
#include "UniSetTypes.h"
#include "UTCPCore.h"
// -------------------------------------------------------------------------
using namespace std;
// -------------------------------------------------------------------------
UTCPSocket::~UTCPSocket()
{
	endSocket();
	// shutdown(so, SHUT_RDWR);
}
// -------------------------------------------------------------------------
UTCPSocket::UTCPSocket( int sock ):
	TCPSocket(NULL)
{
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);

	so = accept(sock, (struct sockaddr*)&client_addr, &client_len);

	if( so < 0 )
	{
		endSocket();
		error(errConnectRejected);
		return;
	}

	Socket::state = CONNECTED;
	init();
}
// -------------------------------------------------------------------------
UTCPSocket::UTCPSocket( const std::string& hname, unsigned backlog, unsigned mss ):
	TCPSocket(hname.c_str(), backlog, mss)
{
	init();
}
// -------------------------------------------------------------------------
UTCPSocket::UTCPSocket(const ost::IPV4Address& bind, ost::tpport_t port, unsigned backlog, unsigned mss):
	TCPSocket(bind, port, backlog, mss)
{
	init();
}
// -------------------------------------------------------------------------
bool UTCPSocket::setKeepAliveParams(timeout_t timeout_sec, int keepcnt, int keepintvl )
{
	return UTCPCore::setKeepAliveParams(so, timeout_sec, keepcnt, keepintvl);
}
// -------------------------------------------------------------------------
bool UTCPSocket::setNoDelay(bool enable)
{
	return ( TCPSocket::setNoDelay(enable) == 0 );
}
// -------------------------------------------------------------------------
int UTCPSocket::getSocket()
{
	return so;
}
// -------------------------------------------------------------------------
void UTCPSocket::init( bool throwflag )
{
	setError(throwflag);
	setKeepAlive(true);
	setLinger(true);
	setKeepAliveParams();
}
// -------------------------------------------------------------------------
ssize_t UTCPSocket::writeData(const void* buf, size_t len, timeout_t t)
{
	return TCPSocket::writeData(buf, len, t);
}
// -------------------------------------------------------------------------
ssize_t UTCPSocket::readData(void* buf, size_t len, char separator, timeout_t t)
{
	return TCPSocket::readData(buf, len, separator, t);
}
// -------------------------------------------------------------------------
