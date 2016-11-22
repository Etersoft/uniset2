#include <iostream>
#include <chrono>
#include "CommonEventLoop.h"
// -------------------------------------------------------------------------
using namespace std;
// -------------------------------------------------------------------------
namespace uniset
{
// -------------------------------------------------------------------------
CommonEventLoop::CommonEventLoop() noexcept
{
	evterm.set(loop);
	evterm.set<CommonEventLoop, &CommonEventLoop::onStop>(this);

	evprep.set(loop);
	evprep.set<CommonEventLoop, &CommonEventLoop::onPrepare>(this);
}
// -------------------------------------------------------------------------
CommonEventLoop::~CommonEventLoop()
{
	if( !cancelled )
	{
		cancelled = true;
		evterm.send();

		if( thr )
		{
			thr->join();
			thr = nullptr;
		}
	}
}
// ---------------------------------------------------------------------------
bool CommonEventLoop::evrun( EvWatcher* w, bool thread, size_t waitTimeout_msec )
{
	if( !w )
		return false;

	bool ret = false;
	{
		{
			std::lock_guard<std::mutex> lck(wlist_mutex);
			for( auto& e: wlist )
			{
				if( e == w )
				{
					cerr << "(CommonEventLoop::evrun): " << w->wname() << " ALREADY ADDED.." << endl;
					return false;
				}
			}
			wlist.push_back(w);
		}

		{
			std::lock_guard<std::mutex> lck(thr_mutex);
			if( !thr )
			{
				thr = make_shared<std::thread>( [ = ] { CommonEventLoop::defaultLoop(); } );

				std::unique_lock<std::mutex> locker(prep_mutex);
				// ожидаем запуска loop
				// иначе evprep.send() улетит в никуда
				prep_event.wait_for(locker,std::chrono::milliseconds(waitTimeout_msec), [=]()
				{
					return (isrunning == false);
				} );

				if( !isrunning )
				{
					cerr << "(CommonEventLoop::evrun): " << w->wname() << " evloop NOT RUN!.." << endl;
					return false;
				}
			}
		}

		// готовим "указатель" на объект требующий активации
		std::unique_lock<std::mutex> locker(prep_mutex);
		wprep = w;

		// взводим флаг
		prep_notify = false;

		// посылаем сигнал для обработки
		evprep.send(); // будим default loop

		// ожидаем обработки evprepare (которая будет в defaultLoop)
		prep_event.wait_for(locker,std::chrono::milliseconds(waitTimeout_msec), [=]()
		{
			return (prep_notify == true);
		} );

		// сбрасываем флаг
		prep_notify = false;

		// если wprep стал nullptr - значит evprepare отработал нормально
		ret = ( wprep == nullptr );
	}

	// если ждать завершения не надо (thread=true)
	// или evprepare не удалось.. выходим..
	if( thread || !ret )
		return ret;

	// ожидаем завершения основного потока..
	std::unique_lock<std::mutex> locker(term_mutex);

	while( !term_notify )
		term_event.wait(locker);

	if( thr && thr->joinable() )
		thr->join();

	return true;
}
// ---------------------------------------------------------------------------
bool CommonEventLoop::evIsActive() const noexcept
{
	return isrunning;
}
// -------------------------------------------------------------------------
bool CommonEventLoop::evstop( EvWatcher* w )
{
	if( !w )
		return false;

	std::lock_guard<std::mutex> l(wlist_mutex);

	try
	{
		w->evfinish(loop); // для этого Watcher это уже finish..
	}
	catch( std::exception& ex )
	{
		cerr << "(CommonEventLoop::evfinish): evfinish err: " << ex.what() << endl;
	}

	wlist.remove(w);

	if( !wlist.empty() )
		return false;

	if( isrunning || !cancelled )
	{
		cancelled = true;
		evterm.send();

		if( thr )
		{
			if( thr->joinable() )
				thr->join();

			thr = nullptr;  // после этого можно уже запускать заново поток..
			cancelled = false;
		}
	}

	return true;
}
// -------------------------------------------------------------------------
void CommonEventLoop::onPrepare() noexcept
{
	prep_notify = false;
	{
		std::lock_guard<std::mutex> lock(prep_mutex);
		if( wprep )
		{
			try
			{
				wprep->evprepare(loop);
			}
			catch( std::exception& ex )
			{
				cerr << "(CommonEventLoop::onPrepare): evprepare err: " << ex.what() << endl;
			}

			wprep = nullptr;
		}
	}

	// будим всех ожидающих..
	prep_notify = true;
	prep_event.notify_all();
}
// -------------------------------------------------------------------------
void CommonEventLoop::onStop() noexcept
{
	// здесь список не защищаем wlist_mutex
	// потому-что onStop будет вызываться
	// из evstop, где он уже будет под "блокировкой"
	// т.е. чтобы не получить deadlock
	for( const auto& w : wlist )
	{
		try
		{
			w->evfinish(loop);
		}
		catch( std::exception& ex )
		{
			cerr << "(CommonEventLoop::onStop): evfinish err: " << ex.what() << endl;
		}
	}

	evterm.stop();
	evprep.stop();
	loop.break_loop(ev::ALL);
}
// -------------------------------------------------------------------------

void CommonEventLoop::defaultLoop() noexcept
{
	isrunning = true;

	evterm.start();
	evprep.start();

	while( !cancelled )
	{
		try
		{
			loop.run(0);
		}
		catch( std::exception& ex )
		{
			cerr << "(CommonEventLoop::defaultLoop): " << ex.what() << endl;
		}
	}

	cancelled = true;
	isrunning = false;

	evterm.stop();
	evprep.stop();

	term_notify = true;
	term_event.notify_all();
}
// -------------------------------------------------------------------------
} // end of uniset namespace
