#include <iostream>
#include <sstream>
#include "Exceptions.h"
#include "modbus/ModbusTCPServer.h"
#include "modbus/ModbusTCPCore.h"
// -------------------------------------------------------------------------
using namespace std;
using namespace ModbusRTU;
using namespace UniSetTypes;
// -------------------------------------------------------------------------
ModbusTCPServer::ModbusTCPServer( ost::InetAddress &ia, int port ):
	TCPSocket(ia,port),
	iaddr(ia),
	ignoreAddr(false)
{
	setCRCNoCheckit(true);
}

// -------------------------------------------------------------------------
ModbusTCPServer::~ModbusTCPServer()
{
	terminate();
}
// -------------------------------------------------------------------------
mbErrCode ModbusTCPServer::receive( ModbusRTU::ModbusAddr addr, timeout_t timeout )
{
	PassiveTimer ptTimeout(timeout);
	ModbusMessage buf;
	mbErrCode res = erTimeOut;
	
//	Thread::setException(Thread::throwException);
//#warning "Why timeout can be 0 there?"
	assert(timeout);
	
	ptTimeout.reset();
	try 
	{
		if( isPendingConnection(timeout) )
		{
			tcp.connect(*this);

			if( timeout != UniSetTimer::WaitUpTime )
			{
				timeout = ptTimeout.getLeft(timeout);
				if( timeout == 0 )
				{
					tcp.disconnect();
					return erTimeOut;
				}

				ptTimeout.setTiming(timeout);
			}

			if( tcp.isPending(Socket::pendingInput, timeout) )
			{
				memset(&curQueryHeader,0,sizeof(curQueryHeader));
				res = tcp_processing(tcp,curQueryHeader);
				if( res!=erNoError )
				{
					tcp.disconnect();
					return res;
				}

				if( timeout != UniSetTimer::WaitUpTime )
				{
					timeout = ptTimeout.getLeft(timeout);
					if( timeout == 0 )
					{
						tcp.disconnect();
						return erTimeOut;
					}
				}

				if( !qrecv.empty() )
				{
					// check addr
				 	unsigned char _addr = qrecv.front();
					
					// для режима игнорирования RTU-адреса
					// просто подменяем его на то который пришёл
					// чтобы проверка всегда была успешной...
					if( ignoreAddr ) 
						addr = _addr;
					else if( _addr != addr )
					{
						tmProcessing.setTiming(replyTimeout_ms);
						ErrorRetMessage em( buf.addr, buf.func, erBadReplyNodeAddress ); 
						buf = em.transport_msg();
						send(buf);
						printProcessingTime();
						tcp.disconnect();
						return res;
					}
    			}
    			
				res = recv( addr, buf, timeout );

				if( res!=erNoError ) // && res!=erBadReplyNodeAddress )
				{
					if( res < erInternalErrorCode )
					{
						ErrorRetMessage em( buf.addr, buf.func, res ); 
						buf = em.transport_msg();
						send(buf);
						printProcessingTime();
					}
					else if( aftersend_msec >= 0 )                                                                                                                                                       
				        msleep(aftersend_msec);  

					tcp.disconnect();
					return res;
				}
					
				if( timeout != UniSetTimer::WaitUpTime )
				{
					timeout = ptTimeout.getLeft(timeout);
					if( timeout == 0 )
					{
						tcp.disconnect();
						return erTimeOut;
					}
				}
				
				if( res!=erNoError )
				{
					tcp.disconnect();
					return res;
				}

				// processing message...
				res = processing(buf);
			}

//			tcp << "exiting now" << endl;
			tcp.disconnect();
		}
	}
	catch( ost::SockException& e )
	{
		cout << e.getString() << ": " << e.getSystemErrorString() << endl;
		return erInternalErrorCode;
	}
	
	return res;
}

// --------------------------------------------------------------------------------
void ModbusTCPServer::setChannelTimeout( timeout_t msec )
{
	tcp.setTimeout(msec);
}
// --------------------------------------------------------------------------------
mbErrCode ModbusTCPServer::sendData( unsigned char* buf, int len )
{
	return ModbusTCPCore::sendData(buf,len,&tcp);
}
// -------------------------------------------------------------------------
int ModbusTCPServer::getNextData( unsigned char* buf, int len )
{
	return ModbusTCPCore::getNextData(buf,len,qrecv,&tcp);
}
// --------------------------------------------------------------------------------

mbErrCode ModbusTCPServer::tcp_processing( ost::TCPStream& tcp, ModbusTCP::MBAPHeader& mhead )
{
	if( !tcp.isConnected() )
		return erTimeOut;

	// чистим очередь
	while( !qrecv.empty() )
		qrecv.pop();

	unsigned int len = getNextData((unsigned char*)(&mhead),sizeof(mhead));
	if( len < sizeof(mhead) )
		return erInvalidFormat;

	mhead.swapdata();

	if( dlog.debugging(Debug::INFO) )
	{
		dlog[Debug::INFO] << "(ModbusTCPServer::tcp_processing): recv tcp header(" << len << "): ";
		mbPrintMessage( dlog, (ModbusByte*)(&mhead), sizeof(mhead));
		dlog(Debug::INFO) << endl;
	}

	// check header
	if( mhead.pID != 0 )
		return erUnExpectedPacketType; // erTimeOut;

	len = ModbusTCPCore::readNextData(&tcp,qrecv,mhead.len);

	if( len<mhead.len )
	{
		if( dlog.debugging(Debug::INFO) )
			dlog[Debug::INFO] << "(ModbusTCPServer::tcp_processing): len(" << (int)len 
					<< ") < mhead.len(" << (int)mhead.len << ")" << endl;
		
		return erInvalidFormat;
	}

	return erNoError;
}
// -------------------------------------------------------------------------
mbErrCode ModbusTCPServer::pre_send_request( ModbusMessage& request )
{
	if( !tcp.isConnected() )
		return erTimeOut;
	
	curQueryHeader.len = request.len + szModbusHeader;

	if( crcNoCheckit )
		curQueryHeader.len -= szCRC;

	curQueryHeader.swapdata();
	if( dlog.debugging(Debug::INFO) )
	{
		dlog[Debug::INFO] << "(ModbusTCPServer::pre_send_request): send tcp header: ";
		mbPrintMessage( dlog, (ModbusByte*)(&curQueryHeader), sizeof(curQueryHeader));
		dlog(Debug::INFO) << endl;
	}

	tcp << curQueryHeader;
	curQueryHeader.swapdata();
	
	return erNoError;
}
// -------------------------------------------------------------------------
void ModbusTCPServer::cleanInputStream()                                                                                                                                            
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
void ModbusTCPServer::terminate()
{
	if( dlog.debugging(Debug::INFO) )
		dlog[Debug::INFO] << "(ModbusTCPServer): terminate..." << endl;
	
	if( tcp && tcp.isConnected() )
		tcp.disconnect();
}
// -------------------------------------------------------------------------
