// -------------------------------------------------------------------------
#ifndef CommonEventLoop_H_
#define CommonEventLoop_H_
// -------------------------------------------------------------------------
#include <ev++.h>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <list>
// -------------------------------------------------------------------------
class EvWatcher
{
	public:
		EvWatcher() {}
		virtual ~EvWatcher() {}

		// подготовка перед запуском loop:
		// запуск своих ev::xxx.start()
		virtual void evprepare( const ev::loop_ref& ) {}

		// действия при завершении:
		// вызов своих ev::xxx.stop()
		virtual void evfinish( const ev::loop_ref& ) {}

		virtual std::string wname()
		{
			return "";
		}
};
// -------------------------------------------------------------------------
/*!
 * \brief The CommonEventLoop class
 * Реализация общего eventloop для всех использующих libev.
 * Каждый класс который хочет подключиться к основному loop, должен наследоваться от класса Watcher
 * и при необходимости переопределить функции evprepare и evfinish
 *
 * Т.к. evprepare необходимо вызывать из потока в котором крутится event loop (иначе libev не работает),
 * а функция run() в общем случае вызывается "откуда угодно" и может быть вызвана в том числе уже после
 * запуска event loop, то задействован механизм асинхронного уведомления (см. evprep, onPrepapre) и ожидания
 * на condition_variable, когда произойдёт инициализация (см. реализацию evrun()).
 */
class CommonEventLoop
{
	public:

		CommonEventLoop();
		~CommonEventLoop();

		bool evIsActive() const;

		/*! \return TRUE - если всё удалось. return актуален только для случая когда thread = true */
		bool evrun( EvWatcher* w, bool thread = true );

		/*! \return TRUE - если это был последний EvWatcher и loop остановлен */
		bool evstop( EvWatcher* w );

		inline const ev::loop_ref evloop()
		{
			return loop;
		}

	protected:

	private:

		void onStop();
		void onPrepare();
		void defaultLoop();

		std::atomic_bool cancelled = { false };
		std::atomic_bool isrunning = { false };

		ev::dynamic_loop loop;
		ev::async evterm;
		std::shared_ptr<std::thread> thr;

		std::mutex              term_mutex;
		std::condition_variable term_event;
		std::atomic_bool term_notify = { false };

		std::mutex wlist_mutex;
		std::list<EvWatcher*> wlist;

		// готовящийся Watcher..он может быть только один в единицу времени
		// это гарантирует prep_mutex
		EvWatcher* wprep = { nullptr };
		ev::async evprep;
		std::condition_variable prep_event;
		std::mutex              prep_mutex;
		std::atomic_bool prep_notify = { false };
};
// -------------------------------------------------------------------------
#endif // CommonEventLoop_H_
// -------------------------------------------------------------------------
