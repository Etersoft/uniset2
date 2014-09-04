#include <iostream>
#include <string>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <cc++/socket.h>
#include "LogSession.h"
#include "UniSetTypes.h"
#include "LogServerTypes.h"
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
LogSession::LogSession( ost::TCPSocket &server, DebugStream* _log, timeout_t msec ):
TCPSession(server),
peername(""),
caddr(""),
log(_log),
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

    string oldLogFile( log->getLogFile() );

    // Команды могут посылаться только в начале сессии..
    if( isPending(Socket::pendingInput, inTimeout) )
    {
		LogServerTypes::lsMessage msg;
		// проверяем канал..(если данных нет, значит "клиент отвалился"...
		if( peek( (void*)(&msg),sizeof(msg)) > 0 )
		{
			ssize_t ret = readData( &msg,sizeof(msg) );

			if( ret!=sizeof(msg) || msg.magic!=LogServerTypes::MAGICNUM )
				slog.warn() << peername << "(run): BAD MESSAGE..." << endl;
			else
			{
				slog.info() << peername << "(run): receive command: '" << msg.cmd << "'" << endl;

				// Обработка команд..
				// \warning Работа с логом ведётся без mutex-а, хотя он разделяется отдельными потоками 
				switch( msg.cmd )
				{
					case LogServerTypes::cmdSetLevel:
						log->level( (Debug::type)msg.data );
					break;
					case LogServerTypes::cmdAddLevel:
						log->addLevel((Debug::type)msg.data );
					break;
					case LogServerTypes::cmdDelLevel:
						log->delLevel( (Debug::type)msg.data );
					break;

					case LogServerTypes::cmdRotate:
					{
						string lfile( log->getLogFile() );
						if( !lfile.empty() )
							log->logFile(lfile);
					}
					break;

					case LogServerTypes::cmdOffLogFile:
					{
						string lfile( log->getLogFile() );
						if( !lfile.empty() )
							log->logFile("");
					}
					break;

					case LogServerTypes::cmdOnLogFile:
					{
						if( !oldLogFile.empty() && oldLogFile != log->getLogFile() )
							log->logFile(oldLogFile);
					}
					break;
					
					default:
						slog.warn() << peername << "(run): Unknown command '" << msg.cmd << "'" << endl;
					break;
				}
			}
		}
	}

    cancelled = false;
    while( !cancelled && !ptSessionTimeout.checkTime() )
    {
		// проверка только ради проверки "целостности" соединения
	    if( isPending(Socket::pendingInput, 10) )
    	{
			char buf[10];
			// проверяем канал..(если данных нет, значит "клиент отвалился"...
			if( peek(buf,sizeof(buf)) <=0 )
				break;
		}

        if( isPending(Socket::pendingOutput, outTimeout) )
        {
			// slog.warn() << peername << "(run): send.." << endl;
			ptSessionTimeout.reset();
			uniset_rwmutex_wrlock l(mLBuf);

			if( !lbuf.empty() )
			{
				slog.info() << peername << "(run): send messages.." << endl;
				while( !lbuf.empty() )
				{
					*tcp() << lbuf.front();
					lbuf.pop_front();
				}
				tcp()->sync();
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
