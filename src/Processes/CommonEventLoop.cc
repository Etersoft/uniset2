#include <iostream>
#include <chrono>
#include <algorithm>
#include "unisetstd.h"
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

		evruntimer.set(loop);
		evruntimer.set<CommonEventLoop, &CommonEventLoop::onLoopOK>(this);
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
	bool CommonEventLoop::runDefaultLoop( size_t waitTimeout_msec )
	{
		std::lock_guard<std::mutex> lock(thr_mutex);

		if( thr )
			return true;

		thr = unisetstd::make_unique<std::thread>( [&] { CommonEventLoop::defaultLoop(); } );

		std::unique_lock<std::mutex> lock2(looprunOK_mutex);
		looprunOK_event.wait_for(lock2, std::chrono::milliseconds(waitTimeout_msec), [&]()
		{
			return (isrunning == true);
		} );

		return isrunning;
	}
	// ---------------------------------------------------------------------------
	bool CommonEventLoop::activateWatcher( EvWatcher* w, size_t waitTimeout_msec )
	{
		// готовим "указатель" на объект требующий активации
		std::unique_lock<std::mutex> locker(prep_mutex);
		wprep = w;

		// взводим флаг
		prep_notify = false;

		// посылаем сигнал для обработки
		if( !evprep.is_active() )
			evprep.start();

		evprep.send(); // будим default loop

		// ожидаем обработки evprepare (которая будет в defaultLoop)
		prep_event.wait_for(locker, std::chrono::milliseconds(waitTimeout_msec), [ = ]()
		{
			return ( prep_notify == true );
		} );

		// сбрасываем флаг
		prep_notify = false;

		// если wprep стал nullptr - значит evprepare отработал нормально
		bool ret = ( wprep == nullptr );
		wprep = nullptr;

		return ret;
	}
	// ---------------------------------------------------------------------------
	void CommonEventLoop::onLoopOK( ev::timer& t, int revents ) noexcept
	{
		if( EV_ERROR & revents )
			return;

		isrunning = true;
		looprunOK_event.notify_all();
		t.stop();
	}
	// ---------------------------------------------------------------------------
	bool CommonEventLoop::evrun( EvWatcher* w, size_t waitTimeout_msec )
	{
		if( w == nullptr )
			return false;

		{
			std::lock_guard<std::mutex> lck(wlist_mutex);

			if( std::find(wlist.begin(), wlist.end(), w) != wlist.end() )
			{
				cerr << "(CommonEventLoop::evrun): " << w->wname() << " ALREADY ADDED.." << endl;
				return false;
			}

			wlist.push_back(w);
		}

		bool defaultLoopOK = runDefaultLoop(waitTimeout_msec);
		bool ret = defaultLoopOK && activateWatcher(w, waitTimeout_msec);

		// или activateWatcher не удалось.. выходим..
		if( !ret )
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
	bool CommonEventLoop::async_evrun( EvWatcher* w, size_t waitTimeout_msec )
	{
		if( w == nullptr )
			return false;

		{
			std::lock_guard<std::mutex> lck(wlist_mutex);

			if( std::find(wlist.begin(), wlist.end(), w) != wlist.end() )
			{
				cerr << "(CommonEventLoop::evrun): " << w->wname() << " ALREADY ADDED.." << endl;
				return false;
			}

			wlist.push_back(w);
		}

		bool defaultLoopOK = runDefaultLoop(waitTimeout_msec);
		return defaultLoopOK && activateWatcher(w, waitTimeout_msec);
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

		std::lock_guard<std::mutex> lock(wlist_mutex);

		try
		{
			w->evfinish(loop); // для этого Watcher это уже finish..
		}
		catch( std::exception& ex )
		{
			cerr << "(CommonEventLoop::evfinish): evfinish err: " << ex.what() << endl;
		}

		wlist.erase( std::remove( wlist.begin(), wlist.end(), w ), wlist.end() );
		//	wlist.remove(w);

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
	size_t CommonEventLoop::size() const
	{
		return wlist.size();
	}
	// -------------------------------------------------------------------------
	void CommonEventLoop::onPrepare( ev::async& w, int revents ) noexcept
	{
		if( EV_ERROR & revents )
		{
			//			cerr << myname << "(CommonEventLoop::onPrepare): invalid event" << endl;
			return;
		}

		prep_notify = false;
		{
			std::lock_guard<std::mutex> lock(prep_mutex);

			if( wprep )
			{
				try
				{
					wprep->evprepare(loop);
					wprep = nullptr;
				}
				catch( std::exception& ex )
				{
					cerr << "(CommonEventLoop::onPrepare): evprepare err: " << ex.what() << endl;
				}
			}
		}

		// будим всех ожидающих..
		prep_notify = true;
		prep_event.notify_all();
	}
	// -------------------------------------------------------------------------
	void CommonEventLoop::onStop( ev::async& aw, int revents ) noexcept
	{
		if( EV_ERROR & revents )
		{
			//			cerr << myname << "(CommonEventLoop::onStop): invalid event" << endl;
			return;
		}

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
		evterm.start();
		evprep.start();

		// нам нужен "одноразовый таймер"
		// т.к. нам надо просто зафиксировать, что loop начал работать
		evruntimer.start(0);

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
		evruntimer.stop();

		term_notify = true;
		term_event.notify_all();
	}
	// -------------------------------------------------------------------------
} // end of uniset namespace
