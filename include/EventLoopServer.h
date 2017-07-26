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

			/*! асинхронный запуск (создаётся отдельный поток)
			 * \return true - если всё хорошо
			 */
			bool async_evrun( size_t waitRunningTimeout_msec = 60000 );

			void evstop(); /*!< остановить раннее запущенный поток (async_run) */

			/*! синхронный запуск
			 * функция вернёт управление, только в случае неудачного запуска
			 * либо если evrun уже был вызван
			 */
			bool evrun();

			ev::dynamic_loop loop;

		private:

			void onStop() noexcept;
			void defaultLoop() noexcept;
			bool waitDefaultLoopRunning( size_t waitTimeout_msec );
			void onLoopOK( ev::timer& t, int revents ) noexcept;

			std::atomic_bool cancelled = { false };
			std::atomic_bool isactive = { false };
			std::timed_mutex run_mutex;

			ev::async evterm;
			std::shared_ptr<std::thread> thr;

			std::mutex              looprunOK_mutex;
			std::condition_variable looprunOK_event;
			std::atomic_bool isrunning = { false };
			ev::timer evruntimer;
	};
	// -------------------------------------------------------------------------
} // end of uniset namespace
// -------------------------------------------------------------------------
#endif // EventLoopServer_H_
// -------------------------------------------------------------------------
