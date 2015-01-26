#ifndef LogReader_H_
#define LogReader_H_
// -------------------------------------------------------------------------
#include <string>
#include <queue>
#include <cc++/socket.h>
#include "UTCPStream.h"
#include "DebugStream.h"
#include "LogServerTypes.h"
// -------------------------------------------------------------------------
class LogReader
{
    public:

        LogReader();
        ~LogReader();

        void readlogs( const std::string& addr, ost::tpport_t port, 
                       LogServerTypes::Command c = LogServerTypes::cmdNOP, 
                       int data = 0, 
                       const std::string& logname="", 
                       bool verbose = false );

        void readlogs( const std::string& addr, ost::tpport_t port, LogServerTypes::lsMessage& m, bool verbose = false );

        bool isConnection();

        inline void setReadCount( unsigned int n ){ readcount = n; }

        inline void setCommandOnlyMode( bool s ){ cmdonly = s; }

        inline void setinTimeout( timeout_t msec ){ inTimeout = msec; }
        inline void setoutTimeout( timeout_t msec ){ outTimeout = msec; }
        inline void setReconnectDelay( timeout_t msec ){ reconDelay = msec; }

        DebugStream::StreamEvent_Signal signal_stream_event();

        void setLogLevel( Debug::type );

    protected:

        void connect( const std::string& addr, ost::tpport_t port, timeout_t tout=TIMEOUT_INF );
        void connect( ost::InetAddress addr, ost::tpport_t port, timeout_t tout=TIMEOUT_INF );
        void disconnect();
        void logOnEvent( const std::string& s );

        timeout_t inTimeout;
        timeout_t outTimeout;
        timeout_t reconDelay;

    private:
        UTCPStream* tcp;
        std::string iaddr;
        ost::tpport_t port;
        bool cmdonly;
        unsigned int readcount; // количество циклов чтения

        DebugStream rlog;
        DebugStream log; // рабочий лог в который выводиться полученная информация..

        DebugStream::StreamEvent_Signal m_logsig;
};
// -------------------------------------------------------------------------
#endif // LogReader_H_
// -------------------------------------------------------------------------
