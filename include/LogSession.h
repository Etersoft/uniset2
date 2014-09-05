// -------------------------------------------------------------------------
#ifndef LogSession_H_
#define LogSession_H_
// -------------------------------------------------------------------------
#include <string>
#include <deque>
#include <cc++/socket.h>
#include "Mutex.h"
#include "DebugStream.h"
#include "PassiveTimer.h"
// -------------------------------------------------------------------------
class LogSession:
    public ost::TCPSession
{
    public:

        LogSession( ost::TCPSocket &server, DebugStream* log, timeout_t sessTimeout=10000, timeout_t cmdTimeout=2000, timeout_t outTimeout=2000 );
        virtual ~LogSession();

        typedef sigc::slot<void, LogSession*> FinalSlot;
        void connectFinalSession( FinalSlot sl );

        inline std::string getClientAddress(){ return caddr; }

    protected:
        virtual void run();
        virtual void final();
        void  logOnEvent( const std::string& s );

    private:
        typedef std::deque<std::string> LogBuffer;
        LogBuffer lbuf;
        std::string peername;
        std::string caddr;
        DebugStream* log;

        timeout_t sessTimeout;
        timeout_t cmdTimeout;
        timeout_t outTimeout;
        PassiveTimer ptSessionTimeout;

        FinalSlot slFin;
        std::atomic_bool cancelled;
        UniSetTypes::uniset_rwmutex mLBuf;

        DebugStream slog;

};
// -------------------------------------------------------------------------
#endif // LogSession_H_
// -------------------------------------------------------------------------
