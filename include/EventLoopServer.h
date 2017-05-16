// -------------------------------------------------------------------------
#ifndef EventLoopServer_H_
#define EventLoopServer_H_
// -------------------------------------------------------------------------
#include <ev++.h>
#include <atomic>
#include <thread>
#include <future>
// -------------------------------------------------------------------------
namespace uniset
{
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

			bool evIsActive() const noexcept;

		protected:
			// действия при завершении
			// завершение своих ev::xxx.stop()
			virtual void evfinish() {}

			// подготовка перед запуском loop
			// запусу своих ev::xxx.start()
			virtual void evprepare() {}

			// Управление потоком событий
			bool evrun( bool thread = true );
			void evstop();

			ev::dynamic_loop loop;

		private:

			void onStop() noexcept;
			void defaultLoop( std::promise<bool>& prepareOK ) noexcept;

			std::atomic_bool cancelled = { false };
			std::atomic_bool isrunning = { false };

			ev::async evterm;
			std::shared_ptr<std::thread> thr;
	};
	// -------------------------------------------------------------------------
} // end of uniset namespace
// -------------------------------------------------------------------------
#endif // EventLoopServer_H_
// -------------------------------------------------------------------------
