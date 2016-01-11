#include "USocket.h"
#include "UTCPCore.h"
// -------------------------------------------------------------------------
using namespace std;
// -------------------------------------------------------------------------
USocket::~USocket()
{
	endSocket();
}
// -------------------------------------------------------------------------
USocket::USocket( int sock ):
	Socket(accept(sock, NULL, NULL))
{
	init();
}
// -------------------------------------------------------------------------
bool USocket::setKeepAliveParams( timeout_t timeout_sec, int keepcnt, int keepintvl )
{
	return UTCPCore::setKeepAliveParams(so, timeout_sec, keepcnt, keepintvl);
}
// -------------------------------------------------------------------------
int USocket::getSocket()
{
	return so;
}
// -------------------------------------------------------------------------
void USocket::init( bool throwflag )
{
	setError(throwflag);
	setKeepAlive(true);
	setLinger(true);
	setKeepAliveParams();
}
// -------------------------------------------------------------------------
