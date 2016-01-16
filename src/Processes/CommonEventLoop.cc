#include <iostream>
#include <chrono>
#include "CommonEventLoop.h"
// -------------------------------------------------------------------------
using namespace std;
// -------------------------------------------------------------------------
CommonEventLoop::CommonEventLoop()
{
	evterm.set(loop);
	evterm.set<CommonEventLoop, &CommonEventLoop::onStop>(this);
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
void CommonEventLoop::evrun( EvWatcher* w, bool thread )
{
	if( !w )
		return;

	{
		std::unique_lock<std::mutex> l(wlist_mutex);
		wlist.push_back(w);

		if( !thr )
		{
			thr = make_shared<std::thread>( [ = ] { CommonEventLoop::defaultLoop(); } );
			std::this_thread::sleep_for(std::chrono::milliseconds(30));
		}

		w->evprepare(loop);
	}

	if( !thread )
	{
		// ожидаем завершения основного потока..
		std::unique_lock<std::mutex> locker(term_mutex);
		while( !term_notify )
			term_event.wait(locker);

		if( thr && thr->joinable() )
			thr->join();
	}
}
// ---------------------------------------------------------------------------
bool CommonEventLoop::evIsActive()
{
	return isrunning;
}
// -------------------------------------------------------------------------
bool CommonEventLoop::evstop( EvWatcher* w )
{
	std::unique_lock<std::mutex> l(wlist_mutex);
	for( auto i = wlist.begin(); i!=wlist.end(); i++ )
	{
		if( (*i) == w )
		{
			wlist.erase(i);
			break;
		}
	}

	if( !wlist.empty() )
	{
		w->evfinish(loop); // для этого Watcher это уже finish..
		return false;
	}

	cancelled = true;
	evterm.send();
	if( thr )
	{
		thr->join();
		thr = nullptr;
	}

	return true;
}
// -------------------------------------------------------------------------
void CommonEventLoop::onStop()
{
	// здесь список не защищаем wlist_mutex
	// потому-что onStop будет вызываться
	// из evstop, где он уже будет под "блокировкой"
	// т.е. чтобы не получить deadlock
	for( const auto& w: wlist )
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
	loop.break_loop(ev::ALL);
}
// -------------------------------------------------------------------------

void CommonEventLoop::defaultLoop()
{
	isrunning = true;

	evterm.start();
	cerr << "************* CommonEventLoop::defaultLoop() *************" << endl;
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

	cerr << "************* CommonEventLoop::defaultLoop() EXIT *************" << endl;
	cancelled = true;
	isrunning = false;

	// будим всех ожидающих..
	term_notify = true;
	term_event.notify_all();
}
// -------------------------------------------------------------------------
