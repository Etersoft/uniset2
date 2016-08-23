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
	Poco::Net::ServerSocket::close();
}
// -------------------------------------------------------------------------
UTCPSocket::UTCPSocket()
{

}
// -------------------------------------------------------------------------
UTCPSocket::UTCPSocket( int sock ):
	Poco::Net::ServerSocket(sock)
{
/*
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);

	so = accept(sock, (struct sockaddr*)&client_addr, &client_len);

	if( so < 0 )
	{
		endServerServerSocket();
		error(errConnectRejected);
		return;
	}

	ServerServerSocket::state = CONNECTED;
*/
	init();
}
// -------------------------------------------------------------------------
UTCPSocket::UTCPSocket( const string& host, int port ):
	Poco::Net::ServerSocket(Poco::Net::SocketAddress(host,port),true)
{
	init();
}
// -------------------------------------------------------------------------
bool UTCPSocket::setKeepAliveParams(timeout_t timeout_sec, int keepcnt, int keepintvl )
{
	return UTCPCore::setKeepAliveParams(Poco::Net::ServerSocket::sockfd() , timeout_sec, keepcnt, keepintvl);
}
// -------------------------------------------------------------------------
bool UTCPSocket::setNoDelay(bool enable)
{
	Poco::Net::ServerSocket::setNoDelay(enable);
	return ( Poco::Net::ServerSocket::getNoDelay() == enable );
}
// -------------------------------------------------------------------------
int UTCPSocket::getSocket()
{
	return Poco::Net::ServerSocket::sockfd();
}
// -------------------------------------------------------------------------
void UTCPSocket::init( bool throwflag )
{
//	setError(throwflag);
	Poco::Net::ServerSocket::setKeepAlive(true);
	Poco::Net::ServerSocket::setLinger(true,1);
	setKeepAliveParams();
}
// -------------------------------------------------------------------------
