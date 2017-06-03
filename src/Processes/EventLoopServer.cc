#include <iostream>
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
	bool EventLoopServer::evrun( bool thread, size_t timeout_msec )
	{
		if( isrunning )
			return true;

		isrunning = true;

		if( !thread )
		{
			defaultLoop(); // <-- здесь бесконечный цикл..
			return false;
		}

		if( !thr )
			thr = make_shared<std::thread>( [&] { defaultLoop(); } );

		bool ret = waitDefaultLoopRunning(timeout_msec);

		// если запуститься не удалось
		if( !ret && thr )
		{
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
		cancelled = true;
		evterm.send();

		if( thr )
		{
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

		looprunOK_state = false;
		looprunOK_event.notify_all();

		isrunning = false;
	}
	// -------------------------------------------------------------------------
	bool EventLoopServer::waitDefaultLoopRunning( size_t waitTimeout_msec )
	{
		std::unique_lock<std::mutex> lock(looprunOK_mutex);
		looprunOK_event.wait_for(lock, std::chrono::milliseconds(waitTimeout_msec), [&]()
		{
			return (looprunOK_state == true);
		} );

		return looprunOK_state;
	}
	// -------------------------------------------------------------------------
	void EventLoopServer::onLoopOK( ev::timer& t, int revents ) noexcept
	{
		if( EV_ERROR & revents )
			return;

		looprunOK_state = true;
		looprunOK_event.notify_all();
		t.stop();
	}
	// -------------------------------------------------------------------------
} // end of uniset namespace
