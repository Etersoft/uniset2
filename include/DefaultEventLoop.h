// -------------------------------------------------------------------------
#ifndef DefaultEventLoop_H_
#define DefaultEventLoop_H_
// -------------------------------------------------------------------------
#include <ev++.h>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <list>
// -------------------------------------------------------------------------
class EventWatcher
{
	public:
		EventWatcher(){}
		virtual ~EventWatcher(){}
};
// -------------------------------------------------------------------------
/*!
 * \brief The DefaultEventLoop class
 * Т.к. libev требует чтобы был только один default_loop и плохо относится к запуску default_loop в разных потоках.
 * то пришлось сделать такую обёртку - sigleton для default_loop. При этом подразумевается, что все объекты(классы),
 * которые используют ev::xxx должны наследоваться от EventWatcher и не создавать свой default_loop, а использовать этот.
 * Иначе будет случаться ошибка: libev: ev_loop recursion during release detected
 * Типичное использование:
 * \code
 * в h-файле:
 * class MyClass:
 *       public EventWatcher
 * {
 *    ...
 *
 *	  std::shared_ptr<DefaultEventLoop> evloop;
 *	  ev::io myio;
 *    ..
 * }
 *
 * в сс-файле:
 * где-то у себя, где надо запускать:
 *
 * ..
 * myio.set(...);
 * myio.start();
 * ..
 * evloop = DefaultEventLoop::inst();
 * evloop->run( this, thread );
 * \endcode
 * При этом thread определяет создавать ли отдельный поток и продолжить работу или "застрять"
 * на вызове run().
 *
 */
class DefaultEventLoop
{
	public:

		~DefaultEventLoop();

		static std::shared_ptr<DefaultEventLoop> inst();

		void run( EventWatcher* s, bool thread = true );

		bool isActive();

		void terminate( EventWatcher* s );

	protected:
		DefaultEventLoop();
		void defaultLoop();
		void finish();

	private:

		static std::shared_ptr<DefaultEventLoop> _inst;

		std::mutex              m_run_mutex;

		std::mutex              m_mutex;
		std::condition_variable m_event;
		std::atomic_bool m_notify = { false };
		std::atomic_bool cancelled = { false };

		std::shared_ptr<ev::default_loop> evloop;
		std::shared_ptr<std::thread> thr;

		std::mutex m_slist_mutex;
		std::list<EventWatcher*> slist;
};
// -------------------------------------------------------------------------
#endif // DefaultEventLoop_H_
// -------------------------------------------------------------------------
