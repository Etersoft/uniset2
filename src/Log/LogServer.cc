#include <sstream>
#include "LogServer.h"
#include "UniSetTypes.h"
#include "Exceptions.h"
#include "LogSession.h"
#include "LogAgregator.h"
#include "Configuration.h"
// -------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
// -------------------------------------------------------------------------
LogServer::~LogServer()
{
	if( nullsess )
		nullsess->cancel();
	{
		// uniset_rwmutex_wrlock l(mutSList);
		for( const auto& i : slist )
		{
			if( i->isRunning() )
				i->cancel();
		}
	}

	cancelled = true;

	if( tcp && !slist.empty() )
		tcp->reject();

	if( thr )
		thr->stop();

	delete tcp;
	tcp = 0;
}
// -------------------------------------------------------------------------
LogServer::LogServer( std::shared_ptr<LogAgregator> log ):
	LogServer(static_pointer_cast<DebugStream>(log))
{

}
// -------------------------------------------------------------------------
LogServer::LogServer( std::shared_ptr<DebugStream> log ):
	timeout(TIMEOUT_INF),
	sessTimeout(3600000),
	cmdTimeout(2000),
	outTimeout(2000),
	sessLogLevel(Debug::NONE),
	sessMaxCount(10),
	cancelled(false),
	thr(0),
	tcp(0),
	elog(log),
	oslog(0)
{
}
// -------------------------------------------------------------------------
LogServer::LogServer():
	timeout(TIMEOUT_INF),
	sessTimeout(3600000),
	cmdTimeout(2000),
	outTimeout(2000),
	sessLogLevel(Debug::NONE),
	sessMaxCount(10),
	cancelled(false),
	thr(0),
	tcp(0),
	elog(nullptr)
{
}
// -------------------------------------------------------------------------
void LogServer::run( const std::string& addr, ost::tpport_t port, bool thread )
{
	try
	{
		ost::InetAddress iaddr(addr.c_str());
		tcp = new ost::TCPSocket(iaddr, port);
#if 0

		if( !nullsess )
		{
			ostringstream err;
			err << "(LOG SERVER): Exceeded the limit on the number of sessions = " << sessMaxCount << endl;
			nullsess = make_shared<NullLogSession>(err.str());
		}

#endif
	}
	catch( ost::Socket* socket )
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
		thr = make_shared<ThreadCreator<LogServer>>(this, &LogServer::work);
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
			while( !cancelled && tcp->isPendingConnection(timeout) )
			{
				if( cancelled ) break;
				{
					uniset_rwmutex_wrlock l(mutSList);
					int sz = slist.size();

					if( sz >= sessMaxCount )
					{
						ostringstream err;
						err << "(LOG SERVER): Exceeded the limit on the number of sessions = " << sessMaxCount << endl;

						if( !nullsess )
						{
							ostringstream err;
							err << "(LOG SERVER): Exceeded the limit on the number of sessions = " << sessMaxCount << endl;
							nullsess = make_shared<NullLogSession>(err.str());
							nullsess->detach(); // start();
						}
						else
							nullsess->setMessage(err.str());

						nullsess->add(*tcp);
						continue;
					}
				}

				if( cancelled ) break;

				auto s = make_shared<LogSession>(*tcp, elog, sessTimeout, cmdTimeout, outTimeout);
				s->setSessionLogLevel(sessLogLevel);
				{
					uniset_rwmutex_wrlock l(mutSList);
					slist.push_back(s);
				}
				s->connectFinalSession( sigc::mem_fun(this, &LogServer::sessionFinished) );
				s->detach();
			}
		}
		catch( ost::Socket* socket )
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
	}

	{
		//      uniset_rwmutex_wrlock l(mutSList);
		for( const auto& i : slist )
			i->disconnect();

		if( nullsess )
			nullsess->cancel();
	}
}
// -------------------------------------------------------------------------
void LogServer::sessionFinished( std::shared_ptr<LogSession> s )
{
	uniset_rwmutex_wrlock l(mutSList);

	for( SessionList::iterator i = slist.begin(); i != slist.end(); ++i )
	{
		if( i->get() == s.get() )
		{
			slist.erase(i);
			return;
		}
	}
}
// -------------------------------------------------------------------------
void LogServer::init( const std::string& prefix, xmlNode* cnode )
{
	auto conf = uniset_conf();

	// можем на cnode==0 не проверять, т.е. UniXML::iterator корректно отрабатывает эту ситуацию
	UniXML::iterator it(cnode);

	timeout_t sessTimeout = conf->getArgPInt("--" + prefix + "-session-timeout", it.getProp("sessTimeout"), 3600000);
	timeout_t cmdTimeout = conf->getArgPInt("--" + prefix + "-cmd-timeout", it.getProp("cmdTimeout"), 2000);
	timeout_t outTimeout = conf->getArgPInt("--" + prefix + "-out-timeout", it.getProp("outTimeout"), 2000);

	setSessionTimeout(sessTimeout);
	setCmdTimeout(cmdTimeout);
	setOutTimeout(outTimeout);
}
// -----------------------------------------------------------------------------
std::string LogServer::help_print( const std::string& prefix )
{
	ostringstream h;
	h << "--" << prefix << "-session-timeout msec  - Timeout for session. Default: 10 min." << endl;
	h << "--" << prefix << "-cmd-timeout msec      - Timeout for wait command. Default: 2000 msec." << endl;
	h << "--" << prefix << "-out-timeout msec      - Timeout for send to client. Default: 2000 msec." << endl;
	return std::move( h.str() );
}
// -----------------------------------------------------------------------------
