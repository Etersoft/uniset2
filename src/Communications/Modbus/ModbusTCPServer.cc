/*! $Id: ModbusTCPServer.cc,v 1.2 2008/11/23 22:16:04 vpashka Exp $ */
// -------------------------------------------------------------------------
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
	iaddr(ia)
{
	setCRCNoCheckit(true);
}

// -------------------------------------------------------------------------
ModbusTCPServer::~ModbusTCPServer()
{
}
// -------------------------------------------------------------------------
mbErrCode ModbusTCPServer::receive( ModbusRTU::ModbusAddr addr, int timeout )
{
	PassiveTimer ptTimeout(timeout);
	ModbusMessage buf;
	mbErrCode res = erTimeOut;
	
//	Thread::setException(Thread::throwException);
	if( timeout<=0 || timeout == UniSetTimer::WaitUpTime )
		timeout = TIMEOUT_INF;
	
	ptTimeout.reset();
	try 
	{
		if( isPendingConnection(timeout) ) 
		{
			tcp.connect(*this);

			if( timeout != TIMEOUT_INF )
			{
				timeout -= ptTimeout.getCurrent();
				if( timeout <=0 )
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

				if( timeout != TIMEOUT_INF )
				{
					timeout -= ptTimeout.getCurrent();
					if( timeout <=0 )
					{
						tcp.disconnect();
						return erTimeOut;
					}
				}

				int msec = (timeout != TIMEOUT_INF) ? timeout : UniSetTimer::WaitUpTime;
				do
				{
					// buf.addr = curQueryHeader.uID;
					// res = recv_pdu(buf,mec);
					res = recv( addr, buf, msec );

					if( res!=erNoError && res!=erBadReplyNodeAddress )
					{
						if( res < erInternalErrorCode )
						{
							ErrorRetMessage em( buf.addr, buf.func, res ); 
							buf = em.transport_msg();
							send(buf);
							printProcessingTime();
						}

						usleep(10000);
						return res;
					}
					
					if( timeout != TIMEOUT_INF )
					{
						timeout -= ptTimeout.getCurrent();
						if( timeout <=0 )
						{
							tcp.disconnect();
							return erTimeOut;
						}
					}
				}
				while( res == erBadReplyNodeAddress );
				
				if( res!=erNoError )
					return res;

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
void ModbusTCPServer::setChannelTimeout( int msec )
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
		return erInvalidFormat;

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
