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
tcp(0),
iaddr("")
{
}

// -------------------------------------------------------------------------
LogReader::~LogReader()
{
    if( isConnection() )
	{
        (*tcp) << endl;
        disconnect();
	}
}
// -------------------------------------------------------------------------
void LogReader::connect( const std::string& addr, ost::tpport_t _port, timeout_t msec )
{
    ost::InetAddress ia(addr.c_str());
    connect(ia,_port,msec);
}
// -------------------------------------------------------------------------
void LogReader::connect( ost::InetAddress addr, ost::tpport_t _port, timeout_t msec )
{
    if( tcp )
    {
        (*tcp) << endl;
        disconnect();
        delete tcp;
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
            tcp = new UTCPStream();
            tcp->create(iaddr,port,true,500);
            tcp->setTimeout(msec);
            tcp->setKeepAlive(true);
        }
        catch( std::exception& e )
        {
            if( rlog.debugging(Debug::CRIT) )
            {
                ostringstream s;
                s << "(LogReader): connection " << s.str() << " error: " << e.what();
                rlog.crit() << s.str() << std::endl;
            }

            delete tcp;
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
    delete tcp;
    tcp = 0;
}
// -------------------------------------------------------------------------
bool LogReader::isConnection()
{
    return tcp && tcp->isConnected();
}
// -------------------------------------------------------------------------
void LogReader::readlogs( const std::string& _addr, ost::tpport_t _port, bool verbose )
{
	timeout_t inTimeout = 10000;
	timeout_t reconDelay = 5000;
	char buf[100001];

	if( verbose )
		rlog.addLevel(Debug::ANY);

	while( true )
	{
		if( !isConnection() )
			connect(_addr,_port,reconDelay);

		if( !isConnection() )
		{
			rlog.warn() << "**** connection timeout.." << endl;
			msleep(reconDelay);
			continue;
		}
	
		while( tcp->isPending(ost::Socket::pendingInput,inTimeout) )
		{
			int n = tcp->peek( buf,sizeof(buf)-1 );
			if( n > 0 )
			{
				tcp->read(buf,n);
				buf[n] = '\0';
				cout << buf;
			}
			else 
				break;
		}
	
		rlog.warn() << "...connection timeout..." << endl;
		disconnect();
	}

    if( isConnection() )
	{
		(*tcp) << endl;
		disconnect();
	}
}
// -------------------------------------------------------------------------
