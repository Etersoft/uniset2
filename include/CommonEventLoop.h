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
	 * когда произойдёт инициализация при помощи promise/future (см. реализацию evrun()).
	 */
	class CommonEventLoop
	{
		public:

			CommonEventLoop() noexcept;
			~CommonEventLoop();

			bool evIsActive() const noexcept;

			/*! \return TRUE - если всё удалось. return актуален только для случая когда thread = true
			 * \param thread - создать отдельный (асинхронный) поток для event loop.
			 * Если thread=false  - функция не вернёт управление и будет ждать завершения работы ( см. evstop())
			 * \param waitPrepareTimeout_msec - сколько ждать активации, либо функция вернёт false.
			 * Даже если thread = false, но wather не сможет быть "активирован" функция вернёт управление
			 * с return false.
			 */
			bool evrun( EvWatcher* w, bool thread = true, size_t waitPrepareTimeout_msec = 8000);

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
			void defaultLoop( std::promise<bool>& runOK ) noexcept;
			bool runDefaultLoop( size_t waitTimeout_msec );
			bool activateWatcher( EvWatcher* w, size_t waitTimeout_msec );

			std::atomic_bool cancelled = { false };
			std::atomic_bool isrunning = { false };

			ev::dynamic_loop loop;
			ev::async evterm;
			std::shared_ptr<std::thread> thr;
			std::mutex              thr_mutex;

			std::mutex              term_mutex;
			std::condition_variable term_event;
			std::atomic_bool term_notify = { false };

			std::mutex wlist_mutex;
			std::vector<EvWatcher*> wlist;

			// очередь wather-ов для инициализации (добавления в обработку)
			struct WatcherInfo
			{
				WatcherInfo( EvWatcher* w, std::promise<bool>& p ):
					watcher(w),result(p){}

				EvWatcher* watcher;
				std::promise<bool>& result;
			};

			std::queue<WatcherInfo> wactlist;
			std::mutex wact_mutex;
			ev::async evprep;
	};
	// -------------------------------------------------------------------------
} // end of uniset namespace
// -------------------------------------------------------------------------
#endif // CommonEventLoop_H_
// -------------------------------------------------------------------------
