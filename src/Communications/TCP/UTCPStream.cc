#include <iostream>
#include <string>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <cc++/socket.h>
#include "UTCPStream.h"
#include "PassiveTimer.h"
#include "UniSetTypes.h"
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
	//setCompletion(false);
}
// -------------------------------------------------------------------------
