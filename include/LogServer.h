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
        LogServer( std::ostream& os );
        ~LogServer();

        inline void setSessionTimeout( timeout_t msec ){ sessTimeout = msec; }
        inline void setCmdTimeout( timeout_t msec ){ cmdTimeout = msec; }
        inline void setOutTimeout( timeout_t msec ){ outTimeout = msec; }

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
        timeout_t sessTimeout;
        timeout_t cmdTimeout;
        timeout_t outTimeout;

        std::atomic_bool cancelled;
        DebugStream mylog;
        ThreadCreator<LogServer>* thr;

        ost::TCPSocket* tcp;
        DebugStream* elog;
        std::ostream* oslog;
};
// -------------------------------------------------------------------------
#endif // LogServer_H_
// -------------------------------------------------------------------------
