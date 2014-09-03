#include <string.h>
#include <errno.h>
#include <iostream>
#include <sstream>
#include "Exceptions.h"
#include "LogReader.h"
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
        disconnect();
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
        }
        catch( std::exception& e )
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
//    }
}
// -------------------------------------------------------------------------
void LogReader::disconnect()
{
    if( rlog.is_info() )
        rlog.info() << iaddr << "(LogReader): disconnect." << endl;

    if( !tcp )
        return;

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
void LogReader::readlogs( const std::string& _addr, ost::tpport_t _port, timeout_t msec )
{
	if( !isConnection() )
		connect(_addr,_port,msec);

	if( !isConnection() )
		throw TimeOut();

	char buf[1000];
	while( tcp->isPending(ost::Socket::pendingInput,msec) )
    {
		int n = tcp->peek( buf,sizeof(buf) );
		if( n > 0 )
		{
			tcp->read(buf,n);
			cout << buf;
		}
#if 0
        if( tcp->gcount() > 0 )
            break;

		int n = tcp->peek( buf,sizeof(buf) );
		tcp->read(buf,sizeof(buf));
		for( int i=0; i<n; i++ )
			cout << buf[i];
#endif
	}
}
// -------------------------------------------------------------------------
