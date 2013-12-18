#include "modbus/ModbusTCPCore.h"
// -------------------------------------------------------------------------
using namespace std;
using namespace ModbusRTU;
// -------------------------------------------------------------------------
int ModbusTCPCore::readNextData( ost::TCPStream* tcp, 
                                    std::queue<unsigned char>& qrecv, int max )
{
    if( !tcp || !tcp->isConnected() )
        return 0;

    int i=0;
    for( ; i<max; i++ )
    {
        char c;
        tcp->read(&c,sizeof(c));
        if( tcp->gcount() <= 0 )
            break;

        qrecv.push( (unsigned char)(c) );
    }

    return i;
}
// -------------------------------------------------------------------------
int ModbusTCPCore::getNextData( unsigned char* buf, int len, 
                                std::queue<unsigned char>& qrecv,
                                ost::TCPStream* tcp )
{
    if( !tcp || !tcp->isConnected() )
        return 0;

    if( qrecv.empty() )
    {
        if( len <= 0 )
            len = 7;

        int ret = ModbusTCPCore::readNextData(tcp,qrecv,len);

        if( ret <= 0 )
            return 0;
    }

    int i=0;
    for( ; i<len && !qrecv.empty(); i++ )
    {
        buf[i] = qrecv.front();
        qrecv.pop();
    }

    return i;
}
// -------------------------------------------------------------------------
mbErrCode ModbusTCPCore::sendData( unsigned char* buf, int len, ost::TCPStream* tcp )
{
    if( !tcp || !tcp->isConnected() )
        return erTimeOut;

    try
    {
        for( int i=0; i<len; i++ )
            (*tcp) << buf[i];

        return erNoError;
    }
    catch( ost::SockException& e )
    {
//        cerr << "(send): " << e.getString() << ": " << e.getSystemErrorString() << endl;
    }
    catch(...)
    {
//        cerr << "(send): cath..." << endl;
    }

    return erHardwareError;
}
// -------------------------------------------------------------------------
