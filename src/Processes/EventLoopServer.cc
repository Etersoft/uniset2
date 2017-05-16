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
	}
	// -------------------------------------------------------------------------
	EventLoopServer::~EventLoopServer()
	{
		if( !cancelled )
			evstop();
	}
	// ---------------------------------------------------------------------------
	bool EventLoopServer::evrun( bool thread )
	{
		if( isrunning )
			return true;

		isrunning = true;

		std::promise<bool> p;

		auto prepareOK = p.get_future();

		if( !thread )
		{
			defaultLoop(p);
			return prepareOK.get();
		}
		else if( !thr )
			thr = make_shared<std::thread>( [ &p, this ] { defaultLoop(std::ref(p)); } );

		bool ret = prepareOK.get();
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
	void EventLoopServer::defaultLoop( std::promise<bool>& prepareOK ) noexcept
	{
		evterm.start();
		try
		{
			evprepare();
			prepareOK.set_value(true);

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
			prepareOK.set_value(false);
		}

		isrunning = false;
	}
	// -------------------------------------------------------------------------
} // end of uniset namespace
