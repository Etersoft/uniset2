#include <iostream>
#include "unisetstd.h"
#include "EventLoopServer.h"
// -------------------------------------------------------------------------
using namespace std;
// -------------------------------------------------------------------------
namespace uniset
{
	// -------------------------------------------------------------------------
	EventLoopServer::EventLoopServer():
		loop(ev::AUTO)
	{
		evterm.set(loop);
		evterm.set<EventLoopServer, &EventLoopServer::onStop>(this);

		evruntimer.set(loop);
		evruntimer.set<EventLoopServer, &EventLoopServer::onLoopOK>(this);
	}
	// -------------------------------------------------------------------------
	EventLoopServer::~EventLoopServer()
	{
		if( !cancelled )
			evstop();
	}
	// ---------------------------------------------------------------------------
	bool EventLoopServer::evrun()
	{
		{
			std::lock_guard<std::timed_mutex> l(run_mutex);
			if( isactive )
				return false;

			isactive = true;
		}

		defaultLoop(); // <-- здесь бесконечный цикл..
		return false;
	}
	// ---------------------------------------------------------------------------
	bool EventLoopServer::async_evrun( size_t timeout_msec )
	{
		if( !run_mutex.try_lock_for(std::chrono::milliseconds(timeout_msec)) )
			return false;

		std::lock_guard<std::timed_mutex> l(run_mutex,std::adopt_lock);

		if( isactive )
			return true;

		isactive = true;

		if( thr )
		{
			if( thr->joinable() )
				thr->join();

			thr = nullptr;
		}

		if( !thr )
			thr = unisetstd::make_unique<std::thread>( [&] { defaultLoop(); } );

		bool ret = waitDefaultLoopRunning(timeout_msec);

		// если запуститься не удалось
		if( !ret && thr )
		{
			if( thr->joinable() )
				thr->join();

			thr = nullptr;
		}

		return ret;
	}
	// ---------------------------------------------------------------------------
	bool EventLoopServer::evIsActive() const noexcept
	{
		return isrunning;
	}
	// -------------------------------------------------------------------------
	void EventLoopServer::evstop()
	{
		{
			std::lock_guard<std::timed_mutex> l(run_mutex);

			if( thr && !isactive )
			{
				if( thr->joinable() )
					thr->join();

				thr = nullptr;
				return;
			}

			cancelled = true;
			evterm.send();
		}

		if( thr )
		{
			if( thr->joinable() )
				thr->join();
			thr = nullptr;
		}
	}
	// -------------------------------------------------------------------------
	void EventLoopServer::onStop() noexcept
	{
		try
		{
			evfinish();
		}
		catch( std::exception& ex )
		{

		}

		evterm.stop();
		loop.break_loop(ev::ALL);
	}
	// -------------------------------------------------------------------------
	void EventLoopServer::defaultLoop() noexcept
	{
		evterm.start();

		// нам нужен "одноразовый таймер"
		// т.к. нам надо просто зафиксировать, что loop начал работать
		evruntimer.start(0);

		try
		{
			evprepare();

			while( !cancelled )
			{
				try
				{
					loop.run(0);
				}
				catch( std::exception& ex )
				{
					cerr << "(EventLoopServer::defaultLoop): " << ex.what() << endl;
				}
			}
		}
		catch( std::exception& ex )
		{
			cerr << "(EventLoopServer::defaultLoop): " << ex.what() << endl;
		}
		catch( ... )
		{
			cerr << "(EventLoopServer::defaultLoop): UNKNOWN EXCEPTION.." << endl;
		}

		{
			std::lock_guard<std::timed_mutex> l(run_mutex);
			isrunning = false;
			looprunOK_event.notify_all();
			isactive = false;
		}
	}
	// -------------------------------------------------------------------------
	bool EventLoopServer::waitDefaultLoopRunning( size_t waitTimeout_msec )
	{
		std::unique_lock<std::mutex> lock(looprunOK_mutex);
		looprunOK_event.wait_for(lock, std::chrono::milliseconds(waitTimeout_msec), [&]()
		{
			return (isrunning == true);
		} );

		return isrunning;
	}
	// -------------------------------------------------------------------------
	void EventLoopServer::onLoopOK( ev::timer& t, int revents ) noexcept
	{
		if( EV_ERROR & revents )
			return;

		isrunning = true;
		looprunOK_event.notify_all();
		t.stop();
	}
	// -------------------------------------------------------------------------
} // end of uniset namespace
