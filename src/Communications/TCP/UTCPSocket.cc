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
int UTCPSocket::getSocket()
{
	return Poco::Net::ServerSocket::sockfd();
}
// -------------------------------------------------------------------------
void UTCPSocket::init()
{
	Poco::Net::ServerSocket::setKeepAlive(true);
	Poco::Net::ServerSocket::setLinger(true,1);
	setKeepAliveParams();
}
// -------------------------------------------------------------------------
