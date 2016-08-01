// -------------------------------------------------------------------------
#ifndef EventLoopServer_H_
#define EventLoopServer_H_
// -------------------------------------------------------------------------
#include <ev++.h>
#include <atomic>
#include <thread>
// -------------------------------------------------------------------------
/*!
 * \brief The EventLoopServer class
 * Реализация общей части всех процессов использующих libev.
 * Содержит свой (динамический) eventloop;
 */
class EventLoopServer
{
	public:

		EventLoopServer();
		virtual ~EventLoopServer();

		bool evIsActive() const;

	protected:
		// действия при завершении
		// завершение своих ev::xxx.stop()
		virtual void evfinish() {}

		// подготовка перед запуском loop
		// запусу своих ev::xxx.start()
		virtual void evprepare() {}

		// Управление потоком событий
		void evrun( bool thread = true );
		void evstop();

		ev::dynamic_loop loop;

	private:

		void onStop();
		void defaultLoop();

		std::atomic_bool cancelled = { false };
		std::atomic_bool isrunning = { false };

		ev::async evterm;
		std::shared_ptr<std::thread> thr;
};
// -------------------------------------------------------------------------
#endif // EventLoopServer_H_
// -------------------------------------------------------------------------
