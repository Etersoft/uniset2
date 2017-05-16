#include <iostream>
#include <chrono>
#include <algorithm>
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
	bool CommonEventLoop::runDefaultLoop( size_t waitTimeout_msec )
	{
		std::lock_guard<std::mutex> lock(thr_mutex);
		if( thr )
			return true;

		bool defaultLoopOK = true;

		std::promise<bool> pRun;
		auto runOK = pRun.get_future();

		thr = make_shared<std::thread>( [ &pRun, this ] { CommonEventLoop::defaultLoop(pRun); } );

		// ожидание старта потока
		while( true )
		{
			auto status = runOK.wait_for(std::chrono::milliseconds(waitTimeout_msec));
			if( status == future_status::timeout )
			{
				defaultLoopOK = false;
				break;
			}

			if( status == future_status::ready )
			{
				defaultLoopOK = runOK.get();
				break;
			}
		}

		return defaultLoopOK;
	}
	// ---------------------------------------------------------------------------
	bool CommonEventLoop::activateWatcher( EvWatcher* w, size_t waitTimeout_msec )
	{
		std::promise<bool> p;
		WatcherInfo winfo(w,p);
		auto result = p.get_future();

		{
			std::unique_lock<std::mutex> l(wact_mutex);
			wactlist.push(winfo);
		}

		bool ret = true;

		// посылаем сигнал для обработки
		evprep.send(); // будим default loop

		// ждём инициализации
		while( true )
		{
			auto status = result.wait_for(std::chrono::milliseconds(waitTimeout_msec));
			if( status == future_status::timeout )
			{
				ret = false;
				break;
			}

			if( status == future_status::ready )
			{
				ret = result.get();
				break;
			}
		}

		return ret;
	}
	// ---------------------------------------------------------------------------
	bool CommonEventLoop::evrun( EvWatcher* w, bool thread, size_t waitTimeout_msec )
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

		// если ждать завершения не надо (thread=true)
		// или activateWatcher не удалось.. выходим..
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

		{
			std::lock_guard<std::mutex> lock(wact_mutex);

			while( !wactlist.empty() )
			{
				auto&& winf = wactlist.front();

				try
				{
					winf.watcher->evprepare(loop);
					winf.result.set_value(true);
				}
				catch( std::exception& ex )
				{
					cerr << "(CommonEventLoop::onPrepare): evprepare err: " << ex.what() << endl;
					winf.result.set_value(false);
				}

				wactlist.pop();
			}
		}
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

	void CommonEventLoop::defaultLoop( std::promise<bool>& runOK ) noexcept
	{
		evterm.start();
		evprep.start();

		isrunning = true;

		runOK.set_value(true);

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
