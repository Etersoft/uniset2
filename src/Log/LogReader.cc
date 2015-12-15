#include <string.h>
#include <errno.h>
#include <iostream>
#include <sstream>
#include "Exceptions.h"
#include "LogReader.h"
#include "UniSetTypes.h"
// -------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
// -------------------------------------------------------------------------
LogReader::LogReader():
	inTimeout(10000),
	outTimeout(6000),
	reconDelay(5000),
	iaddr(""),
	cmdonly(false),
	readcount(0)
{
	log.level(Debug::ANY);
	log.signal_stream_event().connect( sigc::mem_fun(this, &LogReader::logOnEvent) );
}

// -------------------------------------------------------------------------
LogReader::~LogReader()
{
	if( isConnection() )
		disconnect();
}
// -------------------------------------------------------------------------
void LogReader::setLogLevel( Debug::type t )
{
	log.level(t);
}
// -------------------------------------------------------------------------
DebugStream::StreamEvent_Signal LogReader::signal_stream_event()
{
	return m_logsig;
}

// -------------------------------------------------------------------------
void LogReader::connect( const std::string& addr, ost::tpport_t _port, timeout_t msec )
{
	ost::InetAddress ia(addr.c_str());
	connect(ia, _port, msec);
}
// -------------------------------------------------------------------------
void LogReader::connect( ost::InetAddress addr, ost::tpport_t _port, timeout_t msec )
{
	if( tcp )
	{
		(*tcp.get()) << endl;
		disconnect();
		tcp = 0;
	}

	//    if( !tcp )
	//    {

	ostringstream s;
	s << addr;
	iaddr = s.str();
	port = _port;

	if( rlog.is_info() )
		rlog.info() << "(LogReader): connect to " << iaddr << ":" << port << endl;

	ost::Thread::setException(ost::Thread::throwException);

	try
	{
		tcp = make_shared<UTCPStream>();
		tcp->create(iaddr, port, true, 500);
		tcp->setTimeout(msec);
		tcp->setKeepAlive(true);
	}
	catch( const std::exception& e )
	{
		if( rlog.debugging(Debug::CRIT) )
		{
			ostringstream s;
			s << "(LogReader): connection " << s.str() << " error: " << e.what();
			rlog.crit() << s.str() << std::endl;
		}

		tcp = 0;
	}
	catch( ... )
	{
		if( rlog.debugging(Debug::CRIT) )
		{
			ostringstream s;
			s << "(LogReader): connection " << s.str() << " error: catch ...";
			rlog.crit() << s.str() << std::endl;
		}
	}

	//    }
}
// -------------------------------------------------------------------------
void LogReader::disconnect()
{
	if( !tcp )
		return;

	if( rlog.is_info() )
		rlog.info() << iaddr << "(LogReader): disconnect." << endl;

	tcp->disconnect();
	tcp = 0;
}
// -------------------------------------------------------------------------
bool LogReader::isConnection()
{
	return tcp && tcp->isConnected();
}
// -------------------------------------------------------------------------
void LogReader::sendCommand( const std::string& _addr, ost::tpport_t _port, std::vector<Command>& vcmd, bool cmd_only, bool verbose )
{
	if( vcmd.empty() )
		return;

	char buf[100001];

	if( verbose )
		rlog.addLevel(Debug::ANY);

	if( outTimeout == 0 )
		outTimeout = TIMEOUT_INF;

	std::string listfilter("");

	for( const auto& c : vcmd )
	{
		if( c.cmd == LogServerTypes::cmdNOP )
			continue;

		if( c.cmd == LogServerTypes::cmdFilterMode )
		{
			if( verbose )
				cerr << "WARNING: sendCommand() ignore '" << c.cmd << "'..." << endl;

			continue;
		}

		LogServerTypes::lsMessage msg;
		msg.cmd = c.cmd;
		msg.data = c.data;
		msg.setLogName(c.logfilter);

		unsigned int n = 2; // две попытки на посылку

		while( n > 0 )
		{
			try
			{
				if( !isConnection() )
					connect(_addr, _port, reconDelay);

				if( !isConnection() )
				{
					rlog.warn() << "(LogReader): **** connection timeout.." << endl;

					if( cmdonly )
						return;

					n--;

					if( n == 0 )
						break;

					msleep(reconDelay);
					continue;
				}

				if( c.cmd == LogServerTypes::cmdList )
				{
					listfilter = c.logfilter;
					n = 0;
					continue;
				}

				sendCommand(msg, verbose);
				break;
			}
			catch( const ost::SockException& e )
			{
				cerr << "(LogReader): " << e.getString() << " (" << _addr << ")" << endl;
			}
			catch( const std::exception& ex )
			{
				cerr << "(LogReader): " << ex.what() << endl;
			}

			n--;
		}
	} // end for send all command

	if( !isConnection() )
		return;

	// после команд.. выводим список текущий..

	timeout_t reply_timeout = 2000; // TIMEOUT_INF;

	LogServerTypes::lsMessage msg;
	msg.cmd = LogServerTypes::cmdList;
	msg.data = 0;
	msg.setLogName( (listfilter.empty() ? "ALL" : listfilter) );
	sendCommand(msg, verbose);

	// теперь ждём ответ..
	try
	{
		int a = 2;

		while( a > 0 && tcp->isPending(ost::Socket::pendingInput, reply_timeout) )
		{
			int n = tcp->peek( buf, sizeof(buf) - 1 );

			if( n > 0 )
			{
				tcp->read(buf, n);
				buf[n] = '\0';

				log << buf;
			}

			a--;
		}

		// rlog.warn() << "(LogReader): ...wait reply timeout..." << endl;
	}
	catch( const ost::SockException& e )
	{
		cerr << "(LogReader): " << e.getString() << " (" << _addr << ")" << endl;
	}
	catch( const std::exception& ex )
	{
		cerr << "(LogReader): " << ex.what() << endl;
	}

	if( cmdonly && isConnection() )
		disconnect();
}
// -------------------------------------------------------------------------
void LogReader::readlogs( const std::string& _addr, ost::tpport_t _port, LogServerTypes::Command cmd, const std::string logfilter, bool verbose )
{
	char buf[100001];

	if( verbose )
		rlog.addLevel(Debug::ANY);

	if( inTimeout == 0 )
		inTimeout = TIMEOUT_INF;

	if( outTimeout == 0 )
		outTimeout = TIMEOUT_INF;

	unsigned int rcount = 1;

	if( readcount > 0 )
		rcount = readcount;

	LogServerTypes::lsMessage msg;
	msg.cmd = cmd;
	msg.data = 0;
	msg.setLogName(logfilter);

	bool send_ok = cmd == LogServerTypes::cmdNOP ? true : false;

	while( rcount > 0 )
	{
		try
		{
			if( !isConnection() )
				connect(_addr, _port, reconDelay);

			if( !isConnection() )
			{
				rlog.warn() << "(LogReader): **** connection timeout.." << endl;

				if( rcount > 0 && readcount > 0 )
					rcount--;

				if( rcount == 0 )
					break;

				msleep(reconDelay);
				continue;
			}

			if( !send_ok )
			{
				sendCommand(msg, verbose);
				send_ok = true;
			}

			while( tcp->isPending(ost::Socket::pendingInput, inTimeout) )
			{
				ssize_t n = tcp->peek( buf, sizeof(buf) - 1 );

				if( n > 0 )
				{
					tcp->read(buf, n);
					buf[n] = '\0';

					log << buf;
				}
				else if( n == 0 && readcount <= 0 )
					break;

				if( rcount > 0 && readcount > 0 )
					rcount--;

				if( readcount > 0 && rcount == 0 )
					break;
			}

			if( rcount > 0 && readcount > 0 )
				rcount--;

			if( rcount != 0 )
				rlog.warn() << "(LogReader): ...connection timeout..." << endl;

			disconnect();
		}
		catch( const ost::SockException& e )
		{
			cerr << "(LogReader): " << e.getString() << " (" << _addr << ")" << endl;
		}
		catch( const std::exception& ex )
		{
			cerr << "(LogReader): " << ex.what() << endl;
		}

		if( rcount == 0 && readcount > 0 )
			break;
	}

	if( isConnection() )
		disconnect();
}
// -------------------------------------------------------------------------
void LogReader::logOnEvent( const std::string& s )
{
	m_logsig.emit(s);
}
// -------------------------------------------------------------------------
void LogReader::sendCommand(LogServerTypes::lsMessage& msg, bool verbose )
{
	if( !tcp )
	{
		cerr << "(LogReader::sendCommand): tcp=NULL! no connection?!" << endl;
		return;
	}

	try
	{
		if( tcp->isPending(ost::Socket::pendingOutput, outTimeout) )
		{
			rlog.info() << "(LogReader): ** send command: cmd='" << msg.cmd << "' logname='" << msg.logname << "' data='" << msg.data << "'" << endl;

			for( size_t i = 0; i < sizeof(msg); i++ )
				(*tcp) << ((unsigned char*)(&msg))[i];

			tcp->sync();
		}
		else
			rlog.warn() << "(LogReader): **** SEND COMMAND ('" << msg.cmd << "' FAILED!" << endl;
	}
	catch( const ost::SockException& e )
	{
		cerr << "(LogReader): " << e.getString() << endl; // " (" << _addr << ")" << endl;
	}
	catch( const std::exception& ex )
	{
		cerr << "(LogReader): " << ex.what() << endl;
	}
}
// -------------------------------------------------------------------------
