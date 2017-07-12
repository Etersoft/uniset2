/*
 * Copyright (c) 2015 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Lesser Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
// -------------------------------------------------------------------------
#include <string.h>
#include <errno.h>
#include <Poco/Net/NetException.h>
#include <iostream>
#include <sstream>
#include <regex>
#include "PassiveTimer.h"
#include "Exceptions.h"
#include "LogReader.h"
#include "UniSetTypes.h"
// -------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// -------------------------------------------------------------------------
LogReader::LogReader():
	inTimeout(10000),
	outTimeout(6000),
	reconDelay(5000),
	iaddr(""),
	cmdonly(false),
	readcount(0)
{
	outlog = std::make_shared<DebugStream>();

	outlog->level(Debug::ANY);
	outlog->signal_stream_event().connect( sigc::mem_fun(this, &LogReader::logOnEvent) );
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
	outlog->level(t);
}
// -------------------------------------------------------------------------
std::shared_ptr<DebugStream> LogReader::log()
{
	return outlog;
}
// -------------------------------------------------------------------------
DebugStream::StreamEvent_Signal LogReader::signal_stream_event()
{
	return m_logsig;
}
// -------------------------------------------------------------------------
void LogReader::connect( const std::string& _addr, int _port, timeout_t msec )
{
	if( tcp )
	{
		disconnect();
		tcp = nullptr;
	}

	iaddr = _addr;
	port = _port;

	if( rlog.is_info() )
		rlog.info() << "(LogReader): connect to " << iaddr << ":" << port << endl;

	try
	{
		tcp = make_shared<UTCPStream>();
		tcp->create(iaddr, port, msec );
		tcp->setReceiveTimeout( UniSetTimer::millisecToPoco(inTimeout) );
		tcp->setSendTimeout( UniSetTimer::millisecToPoco(outTimeout) );
		tcp->setKeepAlive(true);
		tcp->setBlocking(true);
		return;
	}
	catch( const Poco::TimeoutException& e )
	{
		if( rlog.debugging(Debug::CRIT) )
		{
			ostringstream s;
			s << "(LogReader): connection " << s.str() << " timeout..";
			rlog.crit() << s.str() << std::endl;
		}
	}
	catch( const Poco::Net::NetException& e )
	{
		if( rlog.debugging(Debug::CRIT) )
		{
			ostringstream s;
			s << "(LogReader): connection " << s.str() << " error: " << e.what();
			rlog.crit() << s.str() << std::endl;
		}
	}
	catch( const std::exception& e )
	{
		if( rlog.debugging(Debug::CRIT) )
		{
			ostringstream s;
			s << "(LogReader): connection " << s.str() << " error: " << e.what();
			rlog.crit() << s.str() << std::endl;
		}
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

	tcp = nullptr;
}
// -------------------------------------------------------------------------
void LogReader::disconnect()
{
	if( !tcp )
		return;

	if( rlog.is_info() )
		rlog.info() << iaddr << "(LogReader): disconnect." << endl;

	try
	{
		tcp->disconnect();
	}
	catch( const Poco::Net::NetException& e )
	{
		cerr << "(LogReader): disconnect error:  " << e.displayText() << endl;
	}

	tcp = nullptr;
}
// -------------------------------------------------------------------------
bool LogReader::isConnection() const
{
	return (tcp && tcp->isConnected() );
}
// -------------------------------------------------------------------------
void LogReader::setReadCount(unsigned int n)
{
	readcount = n;
}
// -------------------------------------------------------------------------
void LogReader::setCommandOnlyMode(bool s)
{
	cmdonly = s;
}
// -------------------------------------------------------------------------
void LogReader::setinTimeout(timeout_t msec)
{
	inTimeout = msec;
}
// -------------------------------------------------------------------------
void LogReader::setoutTimeout(timeout_t msec)
{
	outTimeout = msec;
}
// -------------------------------------------------------------------------
void LogReader::setReconnectDelay(timeout_t msec)
{
	reconDelay = msec;
}
// -------------------------------------------------------------------------
void LogReader::setTextFilter(const string& f)
{
	textfilter  = f;
}
// -------------------------------------------------------------------------
void LogReader::sendCommand(const std::string& _addr, int _port, std::vector<Command>& vcmd, bool cmd_only, bool verbose )
{
	if( vcmd.empty() )
		return;

	char buf[100001];

	if( verbose )
		rlog.addLevel(Debug::ANY);

	if( outTimeout == 0 )
		outTimeout = UniSetTimer::WaitUpTime;

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

		size_t n = 2; // две попытки на посылку

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
			catch( const Poco::Net::NetException& e )
			{
				cerr << "(LogReader): send error: " << e.displayText() << " (" << _addr << ")" << endl;
			}
			catch( const std::exception& ex )
			{
				cerr << "(LogReader): send error: " << ex.what() << endl;
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
		size_t a = 2;

		while( a > 0 && tcp->poll(UniSetTimer::millisecToPoco(reply_timeout), Poco::Net::Socket::SELECT_READ) )
		{
			int n = tcp->available();

			if( n > 0 )
			{
				tcp->receiveBytes(buf, n);
				buf[n] = '\0';

				outlog->any(false) << buf;
			}

			a--;
		}

		// rlog.warn() << "(LogReader): ...wait reply timeout..." << endl;
	}
	catch( const Poco::Net::NetException& e )
	{
		cerr << "(LogReader): read error: " << e.displayText() << " (" << _addr << ")" << endl;
	}
	catch( const std::exception& ex )
	{
		cerr << "(LogReader): read error: " << ex.what() << endl;
	}

	if( cmdonly && isConnection() )
		disconnect();
}
// -------------------------------------------------------------------------
void LogReader::readlogs( const std::string& _addr, int _port, LogServerTypes::Command cmd, const std::string logfilter, bool verbose )
{
	char buf[100001];

	if( verbose )
		rlog.addLevel(Debug::ANY);

	if( inTimeout == 0 )
		inTimeout = UniSetTimer::WaitUpTime;

	if( outTimeout == 0 )
		outTimeout = UniSetTimer::WaitUpTime;

	unsigned int rcount = 1;

	if( readcount > 0 )
		rcount = readcount;

	LogServerTypes::lsMessage msg;
	msg.cmd = cmd;
	msg.data = 0;
	msg.setLogName(logfilter);

	bool send_ok = cmd == LogServerTypes::cmdNOP ? true : false;

	std::regex rule(textfilter);

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

			// Если мы работаем с текстовым фильтром
			// то надо читать построчно..
			ostringstream line;

			while( tcp->poll(UniSetTimer::millisecToPoco(inTimeout), Poco::Net::Socket::SELECT_READ) )
			{
				ssize_t n = tcp->available();

				if( n > 0 )
				{
					tcp->receiveBytes(buf, n);
					buf[n] = '\0';

					if( textfilter.empty() )
					{
						outlog->any(false) << buf;
					}
					else
					{
						// Всё это пока не оптимально,
						// но мы ведь и не спешим..
						for( ssize_t i=0; i<n; i++ )
						{
							// пока не встретили конец строки
							// наполняем line..
							if( buf[i] != '\n' )
							{
								line << buf[i];
								continue;
							}

							line << endl;

							std::string s(line.str());

							if( std::regex_search(s, rule) )
								outlog->any(false) << s;

							line.str("");
						}
					}
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
				rlog.warn() << "(LogReader): ...read timeout..." << endl;

			disconnect();
		}
		catch( const Poco::Net::NetException& e )
		{
			cerr << "(LogReader): " << e.displayText() << " (" << _addr << ")" << endl;
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
	if( !tcp || !tcp->isConnected() )
	{
		cerr << "(LogReader::sendCommand): tcp=NULL! no connection?!" << endl;
		return;
	}

	try
	{
		if( tcp->poll(UniSetTimer::millisecToPoco(outTimeout), Poco::Net::Socket::SELECT_WRITE) )
		{
			rlog.info() << "(LogReader): ** send command: cmd='" << msg.cmd << "' logname='" << msg.logname << "' data='" << msg.data << "'" << endl;
			tcp->sendBytes((unsigned char*)(&msg), sizeof(msg));
		}
		else
			rlog.warn() << "(LogReader): **** SEND COMMAND ('" << msg.cmd << "' FAILED!" << endl;
	}
	catch( const Poco::Net::NetException& e )
	{
		cerr << "(LogReader): send error:  " << e.displayText() << endl; // " (" << _addr << ")" << endl;
	}
	catch( const std::exception& ex )
	{
		cerr << "(LogReader): send error: " << ex.what() << endl;
	}
}
// -------------------------------------------------------------------------
