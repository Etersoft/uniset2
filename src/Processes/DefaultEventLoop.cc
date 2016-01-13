#include <iostream>
#include <chrono>
#include "DefaultEventLoop.h"
// -------------------------------------------------------------------------
using namespace std;
// -------------------------------------------------------------------------
std::shared_ptr<DefaultEventLoop> DefaultEventLoop::_inst;
// ---------------------------------------------------------------------------
std::shared_ptr<DefaultEventLoop> DefaultEventLoop::inst()
{
	if( _inst == nullptr )
		_inst = std::shared_ptr<DefaultEventLoop>( new DefaultEventLoop() );

	return _inst;
}

// ---------------------------------------------------------------------------
DefaultEventLoop::~DefaultEventLoop()
{
	if( !cancelled )
	{
		cerr << " ~DefaultEventLoop(): not canceled.." << endl;
		finish();
	}
}
// ---------------------------------------------------------------------------
void DefaultEventLoop::run( EventWatcher* s, bool thread )
{
	if( cancelled )
		return;

	{
		std::unique_lock<std::mutex> lk(m_run_mutex);

		if( !thr )
			thr = make_shared<std::thread>( [ = ] { defaultLoop(); } );
	}

	{
		std::unique_lock<std::mutex> lk(m_slist_mutex);
		slist.push_back(s);
	}

	if( !thread )
	{
		std::unique_lock<std::mutex> lk(m_mutex);

		while( !m_notify )
			m_event.wait(lk);

		if( thr->joinable() )
			thr->join();
	}
}
// -------------------------------------------------------------------------
bool DefaultEventLoop::isActive()
{
	return !cancelled;
}
// -------------------------------------------------------------------------
void DefaultEventLoop::terminate( EventWatcher* s )
{
	cerr << "(DefaultEventLoop::defaultLoop): terminate.." << endl;
	std::unique_lock<std::mutex> lk(m_slist_mutex);

	for( auto i = slist.begin(); i != slist.end(); i++ )
	{
		if( (*i) == s )
		{
			slist.erase(i);
			break;
		}
	}

	if( slist.empty() && !cancelled && evloop )
		finish();
}
// -------------------------------------------------------------------------
void DefaultEventLoop::finish()
{
	cerr << "(DefaultEventLoop::fini): TERMINATE EVENT LOOP.." << endl;
	cancelled = true;

	if( !evloop )
		return;

	cerr << "(DefaultEventLoop::fini): BREAK EVENT LOOP.." << endl;
	evloop->break_loop(ev::ALL);

	std::unique_lock<std::mutex> lk(m_mutex);
	m_event.wait_for(lk, std::chrono::seconds(1), [ = ]()
	{
		return (m_notify == true);
	} );

	if( !m_notify )
	{
		// а как прервать EVENT LOOP?!!
		cerr << "(DefaultEventLoop::fini): MAIN LOOP NOT BREAKED! kill(SIGTERM)!.." << endl;
		raise(SIGTERM);
	}

	if( thr && thr->joinable() )
	{
		cerr << "(DefaultEventLoop::fini): join.." << endl;
		thr->join();
	}

	cerr << "(DefaultEventLoop::fini): exit..." << endl;
	_inst.reset();
}
// -------------------------------------------------------------------------
DefaultEventLoop::DefaultEventLoop()
{

}
// -------------------------------------------------------------------------
void DefaultEventLoop::defaultLoop()
{
	cerr << "(DefaultEventLoop::defaultLoop): run main loop.." << endl;

	// скобки специально чтобы evloop пораньше вышел из "зоны" видимости
	{
		evloop = make_shared<ev::default_loop>();

		while( !cancelled )
		{
			try
			{
				evloop->run(0);
			}
			catch( std::exception& ex )
			{
				cerr << "(DefaultEventLoop::defaultLoop): " << ex.what() << endl;
			}
		}

		cerr << "(DefaultEventLoop::defaultLoop): MAIN EVENT LOOP EXIT ****************" << endl;
	}

	{
		std::unique_lock<std::mutex> lk(m_mutex);
		m_notify = true;
	}

	cancelled = true;
	m_event.notify_all();
}
// -------------------------------------------------------------------------
