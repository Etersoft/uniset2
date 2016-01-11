#include "UTCPCore.h"
// glibc..
#include <netinet/tcp.h>
// -------------------------------------------------------------------------
using namespace std;
// -------------------------------------------------------------------------
bool UTCPCore::setKeepAliveParams( int fd, timeout_t timeout_sec, int keepcnt, int keepintvl )
{
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
