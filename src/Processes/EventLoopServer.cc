#include <iostream>
#include "EventLoopServer.h"
// -------------------------------------------------------------------------
using namespace std;
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
void EventLoopServer::evrun( bool thread )
{
	if( isrunning )
		return;

	isrunning = true;

	if( !thread )
	{
		defaultLoop();
		return;
	}
	else if( !thr )
		thr = make_shared<std::thread>( [ = ] { defaultLoop(); } );
}
// ---------------------------------------------------------------------------
bool EventLoopServer::evIsActive()
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
void EventLoopServer::onStop()
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
void EventLoopServer::defaultLoop()
{
	evterm.start();
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

	cancelled = true;
	isrunning = false;
}
// -------------------------------------------------------------------------
