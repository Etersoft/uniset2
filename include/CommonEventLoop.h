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
		EvWatcher(){}
		virtual ~EvWatcher(){}

		// подготовка перед запуском loop
		// запуск своих ev::xxx.start()
		virtual void evprepare( const ev::loop_ref& ){}

		// действия при завершении
		// завершение своих ev::xxx.stop()
		virtual void evfinish( const ev::loop_ref& ){}

		virtual std::string wname(){ return ""; }
};
// -------------------------------------------------------------------------
/*!
 * \brief The CommonEventLoop class
 * Реализация общего eventloop для всех использующих libev.
 * Каждый класс который хочет подключиться к "потоку", должен наследоваться от класса Watcher
 * и при необходимости переопределить функции prepare и finish
 */
class CommonEventLoop
{
	public:

		CommonEventLoop();
		~CommonEventLoop();

		bool evIsActive();

		void evrun( EvWatcher* w, bool thread = true );

		/*! \return TRUE - если это был последний EvWatcher и loop остановлен */
		bool evstop( EvWatcher* s );

		inline const ev::loop_ref evloop(){ return loop; }

	protected:

	private:

		void onStop();
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
};
// -------------------------------------------------------------------------
#endif // CommonEventLoop_H_
// -------------------------------------------------------------------------
