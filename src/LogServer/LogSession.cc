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
LogSession::LogSession( ost::TCPSocket &server, DebugStream* log, timeout_t msec ):
TCPSession(server),
peername(""),
caddr(""),
timeout(msec),
cancelled(false)
{
	// slog.addLevel(Debug::ANY);
	log->signal_stream_event().connect( sigc::mem_fun(this, &LogSession::logOnEvent) );
}
// -------------------------------------------------------------------------
void  LogSession::logOnEvent( const std::string& s )
{
	uniset_rwmutex_wrlock l(mLBuf);
	lbuf.push_back(s);
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

    ptSessionTimeout.setTiming(10000);
    timeout_t inTimeout = 2000;
    timeout_t outTimeout = 2000;

    cancelled = false;
    while( !cancelled && !ptSessionTimeout.checkTime() )
    {
        if( isPending(Socket::pendingInput, inTimeout) )
        {
			char buf[100];
			// проверяем канал..(если данных нет, значит "клиент отвалился"...
			if( peek(buf,sizeof(buf)) <=0 )
				break;

			ptSessionTimeout.reset();
			slog.warn() << peername << "(run): receive command.." << endl;
			// Обработка команд..
		}

        if( isPending(Socket::pendingOutput, outTimeout) )
        {
			// slog.warn() << peername << "(run): send.." << endl;
			ptSessionTimeout.reset();
			uniset_rwmutex_wrlock l(mLBuf);

			if( !lbuf.empty() )
				slog.info() << peername << "(run): send messages.." << endl;

			while( !lbuf.empty() )
			{
				*tcp() << lbuf.front();
				lbuf.pop_front();
			}
		}
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
     *tcp() << endl;
     slFin(this);
     delete this;
}
// -------------------------------------------------------------------------
void LogSession::connectFinalSession( FinalSlot sl )
{
    slFin = sl;
}
// ---------------------------------------------------------------------
