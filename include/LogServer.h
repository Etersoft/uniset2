// -------------------------------------------------------------------------
#ifndef LogServer_H_
#define LogServer_H_
// -------------------------------------------------------------------------
#include <list>
#include <string>
#include <memory>
#include <cc++/socket.h>
#include "Mutex.h"
#include "UniXML.h"
#include "DebugStream.h"
#include "ThreadCreator.h"
class LogSession;
class LogAgregator;
class NullLogSession;
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
    auto log1 = make_shared<DebugStream>();
    log1->setLogName("log1");

    auto log2 = make_shared<DebugStream>();
    log2->setLogName("log2");

    LogAgregator la;
    la.add(log1);
    la.add(log2);

    LogServer logsrv(la);
    ...
    logsrv.run(host,port,create_thread);
    ...
\endcode

\warning Логи отдаются "клиентам" только целоиком строкой. Т.е. по сети информация передаваться не будет пока не будет записан "endl".
    Это сделано для "оптимизации передачи" (чтобы не передавать каждый байт)
*/
// -------------------------------------------------------------------------
class LogServer
{
	public:

		LogServer( std::shared_ptr<DebugStream> log );
		LogServer( std::shared_ptr<LogAgregator> log );
		~LogServer();

		inline void setSessionTimeout( timeout_t msec )
		{
			sessTimeout = msec;
		}
		inline void setCmdTimeout( timeout_t msec )
		{
			cmdTimeout = msec;
		}
		inline void setOutTimeout( timeout_t msec )
		{
			outTimeout = msec;
		}
		inline void setSessionLog( Debug::type t )
		{
			sessLogLevel = t;
		}
		inline void setMaxSessionCount( int num )
		{
			sessMaxCount = num;
		}

		void run( const std::string& addr, ost::tpport_t port, bool thread = true );

		inline bool isRunning()
		{
			return (thr && thr->isRunning());
		}

		void init( const std::string& prefix, xmlNode* cnode = 0 );

		static std::string help_print( const std::string& prefix );

	protected:
		LogServer();

		void work();
		void sessionFinished( std::shared_ptr<LogSession> s );

	private:
		typedef std::list< std::shared_ptr<LogSession> > SessionList;
		SessionList slist;
		UniSetTypes::uniset_rwmutex mutSList;

		timeout_t timeout;
		timeout_t sessTimeout;
		timeout_t cmdTimeout;
		timeout_t outTimeout;
		Debug::type sessLogLevel;
		int sessMaxCount;

		std::atomic_bool cancelled;
		DebugStream mylog;
		std::shared_ptr< ThreadCreator<LogServer> > thr;

		std::shared_ptr<ost::TCPSocket> tcp;
		std::shared_ptr<DebugStream> elog;
		std::shared_ptr<NullLogSession> nullsess;
};
// -------------------------------------------------------------------------
#endif // LogServer_H_
// -------------------------------------------------------------------------
