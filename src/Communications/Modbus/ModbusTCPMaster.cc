#include <string.h>
#include <errno.h>
#include <iostream>
#include <sstream>
#include "Exceptions.h"
#include "modbus/ModbusTCPMaster.h"
#include "modbus/ModbusTCPCore.h"
// -------------------------------------------------------------------------
using namespace std;
using namespace ModbusRTU;
using namespace UniSetTypes;
// -------------------------------------------------------------------------
ModbusTCPMaster::ModbusTCPMaster():
tcp(0),
nTransaction(0),
iaddr(""),
force_disconnect(true)
{
    setCRCNoCheckit(true);
/*
    dlog.addLevel(Debug::INFO);
    dlog.addLevel(Debug::WARN);
    dlog.addLevel(Debug::CRIT);
*/
}

// -------------------------------------------------------------------------
ModbusTCPMaster::~ModbusTCPMaster()
{
    if( isConnection() )
        disconnect();
}
// -------------------------------------------------------------------------
int ModbusTCPMaster::getNextData( unsigned char* buf, int len )
{
    return ModbusTCPCore::getNextData(buf,len,qrecv,tcp);
}
// -------------------------------------------------------------------------
void ModbusTCPMaster::setChannelTimeout( timeout_t msec )
{
    if( tcp )
        tcp->setTimeout(msec);
}
// -------------------------------------------------------------------------
mbErrCode ModbusTCPMaster::sendData( unsigned char* buf, int len )
{
    return ModbusTCPCore::sendData(buf,len,tcp);
}
// -------------------------------------------------------------------------
mbErrCode ModbusTCPMaster::query( ModbusAddr addr, ModbusMessage& msg, 
                             ModbusMessage& reply, timeout_t timeout )
{
      try
    {
        if( iaddr.empty() )
        {
            if( dlog.is_warn() )
                dlog.warn() << iaddr << "(ModbusTCPMaster::query): unknown ip address for server..." << endl;
            return erTimeOut; // erHardwareError
        }

        if( !isConnection() )
        {
                if( dlog.is_info() )
                    dlog.info() << iaddr << "(ModbusTCPMaster::query): no connection.. reconnnect..." << endl;
                reconnect();
        }

        if( !isConnection() )
        {
            if( dlog.is_warn() )
                dlog.warn() << iaddr << "(ModbusTCPMaster::query): not connected to server..." << endl;
                return erTimeOut;
        }

        assert(timeout);
        ptTimeout.setTiming(timeout);

        tcp->setTimeout(timeout);

//        ost::Thread::setException(ost::Thread::throwException);
//        ost::tpport_t port;
//        cerr << "****** peer: " << tcp->getPeer(&port) << " err: " << tcp->getErrorNumber() << endl;

        if( nTransaction >= numeric_limits<ModbusRTU::ModbusData>::max() )
            nTransaction = 0;
        
        ModbusTCP::MBAPHeader mh;
        mh.tID = ++nTransaction;
        mh.pID = 0;
        mh.len = msg.len + szModbusHeader;
//        mh.uID = addr;
        if( crcNoCheckit )
            mh.len -= szCRC;

        mh.swapdata();
        // send TCP header
        if( dlog.is_info() )
        {
            dlog.info() << iaddr << "(ModbusTCPMaster::query): send tcp header(" << sizeof(mh) <<"): ";
            mbPrintMessage( dlog, (ModbusByte*)(&mh), sizeof(mh));
            dlog(Debug::INFO) << endl;
        }

        for( int i=0; i<2; i++ )
        {
            (*tcp) << mh;

            // send PDU
            mbErrCode res = send(msg);
            if( res!=erNoError )
                return res;

            if( tcp->isPending(ost::Socket::pendingOutput,timeout) )
                break;

            if( dlog.is_info() )
                dlog.info() << "(ModbusTCPMaster::query): no write pending.. reconnnect.." << endl;

            reconnect();
            if( !isConnection() )
            {
                if( dlog.is_warn() )
                    dlog.warn() << "(ModbusTCPMaster::query): not connected to server..." << endl;
                return erTimeOut;
            }
            cleanInputStream();
        
            if( dlog.is_info() )
                dlog.info() << "(ModbusTCPMaster::query): no write pending.. reconnnect OK" << endl;
        }

        mh.swapdata();

        if( timeout != UniSetTimer::WaitUpTime )
        {
            timeout = ptTimeout.getLeft(timeout);
            if( timeout == 0 )
                return erTimeOut;

            ptTimeout.setTiming(timeout);
        }

        // чистим очередь
//        cleanInputStream();
        while( !qrecv.empty() )
            qrecv.pop();

        tcp->sync();
        if( tcp->isPending(ost::Socket::pendingInput,timeout) ) 
        {
/*            
            unsigned char rbuf[100];
            memset(rbuf,0,sizeof(rbuf));
            int ret = getNextData(rbuf,sizeof(rbuf));
            cerr << "ret=" << ret << " recv: ";
            for( int i=0; i<sizeof(rbuf); i++ )
                cerr << hex << " 0x" <<  (int)rbuf[i];
            cerr << endl;
*/
            ModbusTCP::MBAPHeader rmh;
            int ret = getNextData((unsigned char*)(&rmh),sizeof(rmh));
            if( dlog.is_info() )
            {
                dlog.info() << "(ModbusTCPMaster::query): recv tcp header(" << ret << "): ";
                mbPrintMessage( dlog, (ModbusByte*)(&rmh), sizeof(rmh));
                dlog.info(false) << endl;
            }

            if( ret < (int)sizeof(rmh) )
            {
                ost::tpport_t port;
                if( dlog.is_info() )
                    dlog.info() << "(ModbusTCPMaster::query): ret=" << (int)ret
                            << " < rmh=" << (int)sizeof(rmh)
                            << " err: " << tcp->getErrorNumber()
                            << " perr: " << tcp->getPeer(&port)
                            << endl;
                            
                disconnect();
                return erTimeOut; // return erHardwareError;
            }

            rmh.swapdata();
            
            if( rmh.tID != mh.tID )
            {
                cleanInputStream();
                return  erBadReplyNodeAddress;
            }
            if( rmh.pID != 0 )
            {
                cleanInputStream();
                return  erBadReplyNodeAddress;
            }
            //

            // timeout = ptTimeout.getLeft(timeout);
            // в tcp ответе задержек уже не должно быть..
            mbErrCode res = recv(addr,msg.func,reply,1); //timeout);
            
            if( force_disconnect )
            {
                if( dlog.is_info() )
                    dlog.info() << "(query): force disconnect.." << endl;
            
                disconnect();
            }

            return res;
        }

        if( dlog.is_info() )
            dlog.info() << "(query): input pending timeout " << endl;

        if( force_disconnect )
        {
            if( dlog.is_info() )
                dlog.info() << "(query): force disconnect.." << endl;
            
//            cleanInputStream();
            disconnect();
        }

        return erTimeOut;
    }
    catch( ModbusRTU::mbException& ex )
    {
        if( dlog.is_warn() )
            dlog.warn() << "(query): " << ex << endl;
    }
    catch( SystemError& err )
    {
        if( dlog.is_warn() )
            dlog.warn() << "(query): " << err << endl;
    }
    catch( Exception& ex )
    {
        if( dlog.is_warn() )
            dlog.warn() << "(query): " << ex << endl;
    }
    catch( ost::SockException& e ) 
    {
        if( dlog.is_warn() )
            dlog.warn() << "(query): tcp error: " << e.getString() << endl;
        return erTimeOut;
    }
    catch( std::exception& e )
    {
        if( dlog.is_warn() )
            dlog.crit() << "(query): " << e.what() << std::endl;
        return erTimeOut;
    }
    catch(...)
    {
        if( dlog.is_warn() )
            dlog.warn() << "(query): cath..." << endl;
    }    

    return erTimeOut; // erHardwareError
}
// -------------------------------------------------------------------------
void ModbusTCPMaster::cleanInputStream()
{
    unsigned char buf[100];
    int ret=0;
    do
    {
        ret = getNextData(buf,sizeof(buf));
    }
    while( ret > 0);
}
// -------------------------------------------------------------------------
bool ModbusTCPMaster::checkConnection( const std::string ip, int port, int timeout_msec )
{
    try
    {
        ostringstream s;
        s << ip << ":" << port;

        // Проверяем просто попыткой создать соединение..

//        ost::Thread::setException(ost::Thread::throwException);
          //     TCPStream (const char *name, Family family=IPV4, unsigned mss=536, bool throwflag=false, timeout_t timer=0)
        ost::TCPStream t(s.str().c_str(),ost::Socket::IPV4,536,true,timeout_msec);
        t.disconnect();
        return true;
    }
    catch(...)
    {
    }

    return false;
}
// -------------------------------------------------------------------------
void ModbusTCPMaster::reconnect()
{
    if( dlog.is_info() )
        dlog.info() << "(ModbusTCPMaster): reconnect " << iaddr << endl;

    if( tcp )
    {
//        cerr << "tcp diconnect..." << endl;
        tcp->disconnect();
        delete tcp;
        tcp = 0;
    }

    // ost::Thread::setException(ost::Thread::throwException);

    try
    {
          //     TCPStream (const char *name, Family family=IPV4, unsigned mss=536, bool throwflag=false, timeout_t timer=0)
        tcp = new ost::TCPStream(iaddr.c_str(),ost::Socket::IPV4,536,true,500);
        tcp->setTimeout(replyTimeOut_ms);
    }
    catch( std::exception& e )
    {
        if( dlog.debugging(Debug::CRIT) )
        {
            ostringstream s;
            s << "(ModbusTCPMaster): connection " << s.str() << " error: " << e.what();
            dlog.crit() << s.str() << std::endl;
        }
    }
    catch( ... )
    {
        if( dlog.debugging(Debug::CRIT) )
        {
            ostringstream s;
            s << "(ModbusTCPMaster): connection " << s.str() << " error: catch ...";
            dlog.crit() << s.str() << std::endl;
        }
    }
}
// -------------------------------------------------------------------------
void ModbusTCPMaster::connect( const std::string addr, int port )
{
    ost::InetAddress ia(addr.c_str());
    connect(ia,port);
}
// -------------------------------------------------------------------------
void ModbusTCPMaster::connect( ost::InetAddress addr, int port )
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
        s << addr << ":" << port;
        
        if( dlog.is_info() )
            dlog.info() << "(ModbusTCPMaster): connect to " << s.str() << endl;
        
        iaddr = s.str();
        
        ost::Thread::setException(ost::Thread::throwException);
        try
        {        
            tcp = new ost::TCPStream(iaddr.c_str(),ost::Socket::IPV4,536,true,500);
            tcp->setTimeout(replyTimeOut_ms);
        }
        catch( std::exception& e )
        {
            if( dlog.debugging(Debug::CRIT) )
            {
                ostringstream s;
                s << "(ModbusTCPMaster): connection " << s.str() << " error: " << e.what();
                dlog.crit() << s.str() << std::endl;
            }
        }
        catch( ... )
        {
            if( dlog.debugging(Debug::CRIT) )
            {
                ostringstream s;
                s << "(ModbusTCPMaster): connection " << s.str() << " error: catch ...";
                dlog.crit() << s.str() << std::endl;
            }
        }
//    }
}
// -------------------------------------------------------------------------
void ModbusTCPMaster::disconnect()
{
    if( dlog.is_info() )
        dlog.info() << iaddr << "(ModbusTCPMaster): disconnect." << endl;

    if( !tcp )
        return;
    
    tcp->disconnect();
    delete tcp;
    tcp = 0;
}
// -------------------------------------------------------------------------
bool ModbusTCPMaster::isConnection()
{
    return tcp && tcp->isConnected();
}
// -------------------------------------------------------------------------
