#include "USocket.h"
#include "UTCPCore.h"
// -------------------------------------------------------------------------
using namespace std;
// -------------------------------------------------------------------------
namespace uniset
{
	// -------------------------------------------------------------------------
	USocket::~USocket()
	{
		try
		{
			close();
		}
		catch(...) {}
	}
	// -------------------------------------------------------------------------
	USocket::USocket( int sock )
	//	Socket(sock)
	{
		init();
	}
	// -------------------------------------------------------------------------
	bool USocket::setKeepAliveParams( timeout_t timeout_sec, int keepcnt, int keepintvl )
	{
		return UTCPCore::setKeepAliveParams(getSocket(), timeout_sec, keepcnt, keepintvl);
	}
	// -------------------------------------------------------------------------
	int USocket::getSocket()
	{
		return Socket::sockfd();
	}
	// -------------------------------------------------------------------------
	void USocket::init( bool throwflag )
	{
		//setError(throwflag);
		setKeepAlive(true);
		Socket::setLinger(true, 1);
		//setLinger(true);
		setKeepAliveParams();
	}
	// -------------------------------------------------------------------------
} // end of namespace uniset
