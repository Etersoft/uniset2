// -------------------------------------------------------------------------
#ifndef LogSession_H_
#define LogSession_H_
// -------------------------------------------------------------------------
#include <string>
#include <deque>
#include <cc++/socket.h>
#include "Mutex.h"
#include "DebugStream.h"
// -------------------------------------------------------------------------
class LogSession:
    public ost::TCPSession
{
    public:

        LogSession( ost::TCPSocket &server, timeout_t timeout );
        virtual ~LogSession();

        typedef sigc::slot<void, LogSession*> FinalSlot;
        void connectFinalSession( FinalSlot sl );

        inline std::string getClientAddress(){ return caddr; }

    protected:
        virtual void run();
        virtual void final();

    private:
        typedef std::deque<std::string> LogBuffer;
        LogBuffer lbuf;
        std::string peername;
        std::string caddr;

        timeout_t timeout;

        FinalSlot slFin;
        std::atomic_bool cancelled;
        UniSetTypes::uniset_rwmutex mLBuf;

        DebugStream slog;
};
// -------------------------------------------------------------------------
#endif // LogSession_H_
// -------------------------------------------------------------------------
