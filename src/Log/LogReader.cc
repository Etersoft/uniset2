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
tcp(0),
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
void LogReader::readlogs( const std::string& _addr, ost::tpport_t _port, LogServerTypes::Command cmd, int data, const std::string& logname, bool verbose )
{
    LogServerTypes::lsMessage msg;
    msg.cmd = cmd;
    msg.data = data;
    msg.setLogName(logname);
    readlogs(_addr,_port,msg,verbose );
}
// -------------------------------------------------------------------------
void LogReader::readlogs( const std::string& _addr, ost::tpport_t _port, LogServerTypes::lsMessage& msg, bool verbose )
{
    char buf[100001];

    if( verbose )
        rlog.addLevel(Debug::ANY);

    bool send_ok = false;

    if( inTimeout == 0 )
        inTimeout = TIMEOUT_INF;

    if( outTimeout == 0 )
        outTimeout = TIMEOUT_INF;

    unsigned int n = 1;
    if( readcount > 0 )
        n = readcount;

    while( n > 0 )
    {
        if( !isConnection() )
            connect(_addr,_port,reconDelay);

        if( !isConnection() )
        {
            rlog.warn() << "(LogReader): **** connection timeout.." << endl;

            if( readcount > 0 )
                n--;

            if( n<0 )
                break;

            msleep(reconDelay);
            continue;
        }

        if( !send_ok && msg.cmd != LogServerTypes::cmdNOP )
        {
            if( tcp->isPending(ost::Socket::pendingOutput,outTimeout) )
            {
                rlog.info() << "(LogReader): ** send command: logname='" << msg.logname << "' cmd='" << msg.cmd << "' data='" << msg.data << "'" << endl;

//               LogServerTypes::lsMessage msg;
//               msg.cmd = cmd;
//               msg.data = data;
                for( size_t i=0; i<sizeof(msg); i++ )
                    (*tcp) << ((unsigned char*)(&msg))[i];

                tcp->sync();
                send_ok = true;
            }
            else
                rlog.warn() << "(LogReader): **** SEND COMMAND ('" << msg.cmd << "' FAILED!" << endl;

            if( cmdonly )
            {
                disconnect();
                return;
            }
        }

        while( !cmdonly && tcp->isPending(ost::Socket::pendingInput,inTimeout) )
        {
            int n = tcp->peek( buf,sizeof(buf)-1 );
            if( n > 0 )
            {
                tcp->read(buf,n);
                buf[n] = '\0';
                log << buf;
            }
            else
                break;
        }

        rlog.warn() << "(LogReader): ...connection timeout..." << endl;
        send_ok = false; // ??!! делать ли?
        disconnect();

        if( readcount > 0 )
            n--;
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
