// -------------------------------------------------------------------------
#ifndef CommonEventLoop_H_
#define CommonEventLoop_H_
// -------------------------------------------------------------------------
#include <ev++.h>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <queue>
#include <future>
// -------------------------------------------------------------------------
namespace uniset
{


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

			virtual std::string wname() const noexcept
			{
				return "";
			}
	};
	// -------------------------------------------------------------------------
	/*!
	 * \brief The CommonEventLoop class
	 * Реализация механизма "один eventloop, много подписчиков" (libev).
	 * Создаётся один CommonEventLoop который обслуживает множество EvWatcher-ов.
	 * Каждый класс который хочет подключиться к основному loop, должен наследоваться от класса Watcher
	 * и при необходимости переопределить функции evprepare и evfinish.
	 * EvWatcher добавляется(запускается) evrun(..) и останавливается функцией evstop(..).
	 * При этом фактически eventloop запускается при первом вызове evrun(), а останавливается при
	 * отключении последнего EvWatcher.
	 *
	 * Некоторые детали:
	 * Т.к. evprepare необходимо вызывать из потока в котором крутится event loop (иначе libev не работает),
	 * а функция run() в общем случае вызывается "откуда угодно" и может быть вызвана в том числе уже после
	 * запуска event loop, то задействован механизм асинхронного уведомления (см. evprep, onPrepapre) и ожидания
	 * на condition_variable, когда произойдёт инициализация (см. реализацию evrun()).
	 */
	class CommonEventLoop
	{
		public:

			CommonEventLoop() noexcept;
			~CommonEventLoop();

			bool evIsActive() const noexcept;

			/*! Синхронный запуск. Функция возвращает управление (false), только если запуск не удался,
			 * либо был остановлен вызовом evstop();
			 * \param prepareTimeout_msec - сколько ждать активации, либо функция вернёт false.
			 */
			bool evrun( EvWatcher* w, size_t prepareTimeout_msec = 60000);

			/*! Асинхронный запуск (запуск в отдельном потоке)
			 * \return TRUE - если всё удалось.
			 * \param prepareTimeout_msec - сколько ждать активации, либо функция вернёт false.
			 */
			bool async_evrun( EvWatcher* w, size_t prepareTimeout_msec = 60000 );

			/*! \return TRUE - если это был последний EvWatcher и loop остановлен */
			bool evstop( EvWatcher* w );

			inline const ev::loop_ref evloop() noexcept
			{
				return loop;
			}

			// количество зарегистрированных wather-ов
			size_t size() const;

		protected:

		private:

			void onStop( ev::async& w, int revents ) noexcept;
			void onPrepare( ev::async& w, int revents ) noexcept;
			void defaultLoop() noexcept;
			bool runDefaultLoop( size_t waitTimeout_msec );
			bool activateWatcher( EvWatcher* w, size_t waitTimeout_msec );
			void onLoopOK( ev::timer& t, int revents ) noexcept;

			std::atomic_bool cancelled = { false };
			std::atomic_bool isrunning = { false };

			ev::dynamic_loop loop;
			ev::async evterm;
			std::unique_ptr<std::thread> thr;
			std::mutex              thr_mutex;

			std::mutex              term_mutex;
			std::condition_variable term_event;
			std::atomic_bool term_notify = { false };

			std::mutex wlist_mutex;
			std::vector<EvWatcher*> wlist;

			// готовящийся Watcher..он может быть только один в единицу времени
			// это гарантирует prep_mutex
			EvWatcher* wprep = { nullptr };
			ev::async evprep;
			std::condition_variable prep_event;
			std::mutex              prep_mutex;
			std::atomic_bool prep_notify = { false };


			std::mutex              looprunOK_mutex;
			std::condition_variable looprunOK_event;
			ev::timer evruntimer;
	};
	// -------------------------------------------------------------------------
} // end of uniset namespace
// -------------------------------------------------------------------------
#endif // CommonEventLoop_H_
// -------------------------------------------------------------------------
