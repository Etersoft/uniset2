// -------------------------------------------------------------------------
#ifndef LogSession_H_
#define LogSession_H_
// -------------------------------------------------------------------------
#include <string>
#include <memory>
#include <deque>
#include <cc++/socket.h>
#include <condition_variable>
#include <mutex>
#include "Mutex.h"
#include "DebugStream.h"
#include "PassiveTimer.h"
// -------------------------------------------------------------------------
/*! Реализация "сессии" для клиентов LogServer. */
class LogSession:
	public std::enable_shared_from_this<LogSession>,
	public ost::TCPSession
{
	public:

		LogSession( ost::TCPSocket& server, std::shared_ptr<DebugStream>& log, timeout_t sessTimeout = 10000, timeout_t cmdTimeout = 2000, timeout_t outTimeout = 2000, timeout_t delay = 2000 );
		virtual ~LogSession();

		typedef sigc::slot<void, std::shared_ptr<LogSession>> FinalSlot;
		void connectFinalSession( FinalSlot sl );

		inline void cancel()
		{
			cancelled = true;
		}
		inline std::string getClientAddress()
		{
			return caddr;
		}

		inline void setSessionLogLevel( Debug::type t )
		{
			slog.level(t);
		}
		inline void addSessionLogLevel( Debug::type t )
		{
			slog.addLevel(t);
		}
		inline void delSessionLogLevel( Debug::type t )
		{
			slog.delLevel(t);
		}

	protected:
		LogSession( ost::TCPSocket& server );

		virtual void run();
		virtual void final();
		void logOnEvent( const std::string& s );
		void readStream();

		timeout_t sessTimeout;
		timeout_t cmdTimeout;
		timeout_t outTimeout;
		timeout_t delayTime;

	private:
		typedef std::deque<std::string> LogBuffer;
		LogBuffer lbuf;
		std::string peername;
		std::string caddr;
		std::shared_ptr<DebugStream> log;

		//        PassiveTimer ptSessionTimeout;
		FinalSlot slFin;
		std::atomic_bool cancelled;

		DebugStream slog;
		std::ostringstream sbuf;

		std::mutex              log_mutex;
		std::condition_variable log_event;
		std::atomic_bool log_notify = ATOMIC_VAR_INIT(0);
};
// -------------------------------------------------------------------------
/*! Сессия просто заверщающаяся с указанным сообщением */
class NullLogSession:
	public ost::Thread
{
	public:

		NullLogSession( const std::string& _msg );
		~NullLogSession();

		void add( ost::TCPSocket& server );
		void setMessage( const std::string& _msg );

		inline void cancel()
		{
			cancelled = true;
		}

	protected:

		virtual void run();
		virtual void final();

	private:
		std::string msg;

		typedef std::list< std::shared_ptr<ost::TCPStream> > TCPStreamList;
		TCPStreamList slist;
		UniSetTypes::uniset_rwmutex smutex;
		std::atomic_bool cancelled;
};
// -------------------------------------------------------------------------
#endif // LogSession_H_
// -------------------------------------------------------------------------
