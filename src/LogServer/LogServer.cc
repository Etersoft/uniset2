#include <sstream>
#include "LogServer.h"
#include "UniSetTypes.h"
#include "Exceptions.h"
#include "LogSession.h"
// -------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
// -------------------------------------------------------------------------
LogServer::~LogServer()
{
	cancelled = true;

	{
		uniset_rwmutex_wrlock l(mutSList);
		for( SessionList::iterator i=slist.begin(); i!=slist.end(); ++i )
			delete (*i);
	}

	if( thr )
	{
		thr->stop();
		delete thr;
	}
}
// -------------------------------------------------------------------------
LogServer::LogServer():
timeout(600000),
session_timeout(3600000),
cancelled(false),
thr(0),
tcp(0)
{
}
// -------------------------------------------------------------------------
void LogServer::run( const std::string& addr, ost::tpport_t port, timeout_t msec, bool thread )
{
	try
	{
		ost::InetAddress iaddr(addr.c_str());
		tcp = new ost::TCPSocket(iaddr,port);
	}
	catch( ost::Socket *socket )
	{
		ost::tpport_t port;
		int errnum = socket->getErrorNumber();
		ost::InetAddress saddr = (ost::InetAddress)socket->getPeer(&port);
		ostringstream err;

		err << "socket error " << saddr.getHostname() << ":" << port << " = " << errnum;

		if( errnum == ost::Socket::errBindingFailed )
			err << "bind failed; port busy" << endl;
		else
			err << "client socket failed" << endl;

		throw SystemError( err.str() );
	}

	if( !thread )
		work();
	else
	{
        thr = new ThreadCreator<LogServer>(this, &LogServer::work);
        thr->start();
	}
}
// -------------------------------------------------------------------------
void LogServer::work()
{
	cancelled = false;
	while( !cancelled )
	{
		try
		{
			while( tcp->isPendingConnection(timeout) )
			{
				LogSession* s = new LogSession(*tcp, session_timeout);
				{
					uniset_rwmutex_wrlock l(mutSList);
					slist.push_back(s);
				}
				s->connectFinalSession( sigc::mem_fun(this, &LogServer::sessionFinished) );
				s->detach();
			}
		}
		catch( ost::Socket *socket )
		{
			ost::tpport_t port;
			int errnum = socket->getErrorNumber();
			ost::InetAddress saddr = (ost::InetAddress)socket->getPeer(&port);
			cerr << "socket error " << saddr.getHostname() << ":" << port << " = " << errnum << endl; 
			if( errnum == ost::Socket::errBindingFailed )
			{
				cerr << "bind failed; port busy" << endl;
				// ::exit(-1);
			}
			else
				cerr << "client socket failed" << endl;
		}
	
		cout << "timeout after 30 seconds inactivity, exiting" << endl;
	}

	{
		uniset_rwmutex_wrlock l(mutSList);
		for( SessionList::iterator i=slist.begin(); i!=slist.end(); ++i )
			(*i)->disconnect();
	}
}
// -------------------------------------------------------------------------
void LogServer::sessionFinished( LogSession* s )
{
	uniset_rwmutex_wrlock l(mutSList);
	for( SessionList::iterator i=slist.begin(); i!=slist.end(); ++i )
	{
		if( (*i) == s )
		{
			slist.erase(i);
			return;
		}
	}
}
// -------------------------------------------------------------------------
