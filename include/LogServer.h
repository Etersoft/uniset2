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
/*! \page pgLogServer Лог сервер
	Лог сервер предназначен для возможности удалённого чтения логов (DebugStream).
Ему указывается host и port для прослушивания запросов, которые можно делать при помощи
LogReader. Читающих клиентов может быть скольугодно много, на каждого создаётся своя "сессия"(LogSession).
При этом через лог сервер имеется возможность управлять включением или отключением определённых уровней логов,
записью, отключением записи или ротацией файла с логами.  DebugStream за которым ведётся "слежение"
задаётся в конструкторе для LogServer. 
\code
   DebugStream mylog;
   LogServer logsrv(mylog);
   ...
   logsrv.run(host,port,create_thread);
   ...
\endcode

При этом если необходимо управлять или читать сразу несколько логов можно воспользоваться специальным классом LogAgregator.
\code
    DebugStream log1;
    log1.setLogName("log1");

    DebugStream log2;
    log2.setLogName("log2");

    LogAgregator la;
    la.add(log1);
    la.add(log2);

    LogServer logsrv(la);
    ...
    logsrv.run(host,port,create_thread);
    ...
\endcode
*/
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
