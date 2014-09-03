#include <iostream>
#include <string>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <cc++/socket.h>
#include "LogSession.h"
#include "UniSetTypes.h"
// -------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
// -------------------------------------------------------------------------
LogSession::~LogSession()
{
    cancelled = true;
    if( isRunning() )
	{
        disconnect();
        ost::Thread::join();
	}
}
// -------------------------------------------------------------------------
LogSession::LogSession( ost::TCPSocket &server, timeout_t msec ):
TCPSession(server),
peername(""),
caddr(""),
timeout(msec),
cancelled(false)
{
}
// -------------------------------------------------------------------------
void LogSession::run()
{
    if( cancelled )
        return;

    {
        ost::tpport_t p;
        ost::InetAddress iaddr = getIPV4Peer(&p);

        // resolve..
        caddr = string( iaddr.getHostname() );

        ostringstream s;
        s << iaddr << ":" << p;
        peername = s.str();
    }

    if( slog.debugging(Debug::INFO) )
        slog[Debug::INFO] << peername << "(run): run thread of sessions.." << endl;

    cancelled = false;
    while( !cancelled && isPending(Socket::pendingOutput, timeout) )
    {
//        char rbuf[100];
//       int ret = readData(&rbuf,sizeof(rbuf));
         *tcp() << "test log... test log" << endl;
         sleep(8000);
    }

    if( slog.debugging(Debug::INFO) )
        slog[Debug::INFO] << peername << "(run): stop thread of sessions..disconnect.." << endl;

    disconnect();

    if( slog.debugging(Debug::INFO) )
        slog[Debug::INFO] << peername << "(run): thread stopping..." << endl;
}
// -------------------------------------------------------------------------
void LogSession::final()
{
     slFin(this);
     delete this;
}
// -------------------------------------------------------------------------
void LogSession::connectFinalSession( FinalSlot sl )
{
    slFin = sl;
}
// ---------------------------------------------------------------------