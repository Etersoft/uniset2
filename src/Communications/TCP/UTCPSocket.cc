#include <iostream>
#include <string>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include "UTCPSocket.h"
#include "PassiveTimer.h"
#include "UniSetTypes.h"
#include "UTCPCore.h"
// -------------------------------------------------------------------------
using namespace std;
// -------------------------------------------------------------------------
UTCPSocket::~UTCPSocket()
{
	close();
	//endSocket();
	// shutdown(so, SHUT_RDWR);

}
// -------------------------------------------------------------------------
UTCPSocket::UTCPSocket( int sock ):
	Poco::Net::RawSocket(sock)
{
/*
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
*/
	init();
}
// -------------------------------------------------------------------------
UTCPSocket::UTCPSocket( const string& host, int port ):
	Poco::Net::RawSocket(Poco::Net::SocketAddress(host,port),true)
{
	init();
}
// -------------------------------------------------------------------------
bool UTCPSocket::setKeepAliveParams(timeout_t timeout_sec, int keepcnt, int keepintvl )
{
	return UTCPCore::setKeepAliveParams(Poco::Net::RawSocket::sockfd() , timeout_sec, keepcnt, keepintvl);
}
// -------------------------------------------------------------------------
bool UTCPSocket::setNoDelay(bool enable)
{
	Poco::Net::RawSocket::setNoDelay(enable);
	return ( Poco::Net::RawSocket::getNoDelay() == enable );
}
// -------------------------------------------------------------------------
int UTCPSocket::getSocket()
{
	return Poco::Net::RawSocket::sockfd();
}
// -------------------------------------------------------------------------
void UTCPSocket::init( bool throwflag )
{
//	setError(throwflag);
	Poco::Net::RawSocket::setKeepAlive(true);
	Poco::Net::RawSocket::setLinger(true,1);
	setKeepAliveParams();
}
// -------------------------------------------------------------------------
ssize_t UTCPSocket::writeData(const void* buf, size_t len, timeout_t t)
{
	return Poco::Net::RawSocket::sendBytes(buf, len);
}
// -------------------------------------------------------------------------
ssize_t UTCPSocket::readData(void* buf, size_t len, char separator, timeout_t t)
{
	return Poco::Net::RawSocket::receiveBytes(buf,len);
}
// -------------------------------------------------------------------------
