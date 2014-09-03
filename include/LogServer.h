// -------------------------------------------------------------------------
#ifndef LogServer_H_
#define LogServer_H_
// -------------------------------------------------------------------------
#include <list>
#include <string>
#include <cc++/socket.h>
#include "Mutex.h"
#include "DebugStream.h"
#include "ThreadCreator.h"
class LogSession;
// -------------------------------------------------------------------------
class LogServer
{
    public:

        LogServer( DebugStream& log );
        ~LogServer();

        void run( const std::string& addr, ost::tpport_t port, bool thread=true );

    protected:
         LogServer();

         void work();
         void sessionFinished( LogSession* s );

    private:
        typedef std::list<LogSession*> SessionList;
        SessionList slist;
        UniSetTypes::uniset_rwmutex mutSList;

        timeout_t timeout;
        timeout_t session_timeout;
        std::atomic_bool cancelled;
        DebugStream mylog;
        ThreadCreator<LogServer>* thr;

        ost::TCPSocket* tcp;
        DebugStream* elog;
};
// -------------------------------------------------------------------------
#endif // LogServer_H_
// -------------------------------------------------------------------------
