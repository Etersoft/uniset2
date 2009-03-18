/*! $Id: ModbusTCPMaster.cc,v 1.3 2009/02/07 13:27:01 vpashka Exp $ */
// -------------------------------------------------------------------------
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
iaddr("")
{
	setCRCNoCheckit(true);
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
void ModbusTCPMaster::setChannelTimeout( int msec )
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
int ModbusTCPMaster::nTransaction = 0;

mbErrCode ModbusTCPMaster::query( ModbusAddr addr, ModbusMessage& msg, 
				 			ModbusMessage& reply, int timeout )
{

//	if( !isConnection() )
	if( iaddr.empty() )
	{
		dlog[Debug::WARN] << "(query): not connection to server..." << endl;
		return erHardwareError;

	}

	reconnect();

	if( timeout<=0 || timeout == UniSetTimer::WaitUpTime )
	{
		timeout = TIMEOUT_INF;
		ptTimeout.setTiming(UniSetTimer::WaitUpTime);
	}
	else
		ptTimeout.setTiming(timeout);
	
	ost::Thread::setException(ost::Thread::throwException);
	
	try
	{
		if( nTransaction >= numeric_limits<int>::max() )
			nTransaction = 0;
		
		ModbusTCP::MBAPHeader mh;
		mh.tID = ++nTransaction;
		mh.pID = 0;
		mh.len = msg.len + szModbusHeader;
//		mh.uID = addr;
		if( crcNoCheckit )
			mh.len -= szCRC;

		mh.swapdata();
		// send TCP header
		if( dlog.debugging(Debug::INFO) )
		{
			dlog[Debug::INFO] << "(ModbusTCPMaster::query): send tcp header(" << sizeof(mh) <<"): ";
			mbPrintMessage( dlog, (ModbusByte*)(&mh), sizeof(mh));
			dlog(Debug::INFO) << endl;
		}
		(*tcp) << mh;
		mh.swapdata();

		// send PDU
		mbErrCode res = send(msg);
		if( res!=erNoError )
			return res;
		
  		if( !tcp->isPending(ost::Socket::pendingOutput,timeout) )
  			return erTimeOut;

		if( timeout != TIMEOUT_INF )
		{
			timeout -= ptTimeout.getCurrent();
			if( timeout <=0 )
				return erTimeOut;

			ptTimeout.setTiming(timeout);
		}

		// чистим очередь
		while( !qrecv.empty() )
			qrecv.pop();

		if( tcp->isPending(ost::Socket::pendingInput,timeout) ) 
		{
			ModbusTCP::MBAPHeader rmh;
			int ret = getNextData((unsigned char*)(&rmh),sizeof(rmh));
			if( dlog.debugging(Debug::INFO) )
			{
				dlog[Debug::INFO] << "(ModbusTCPMaster::query): recv tcp header(" << ret << "): ";
				mbPrintMessage( dlog, (ModbusByte*)(&rmh), sizeof(rmh));
				dlog(Debug::INFO) << endl;
			}

			if( ret < sizeof(rmh) )
				return erHardwareError;

			rmh.swapdata();
			
			if( rmh.tID != mh.tID )
				return  erBadReplyNodeAddress;
			if( rmh.pID != 0 )
				return  erBadReplyNodeAddress;

			//
			return recv(addr,msg.func,reply,timeout);
//			msg.addr = rmh.uID;
//			return recv_pdu(msg.func,reply,timeout);
		}
		
		return erTimeOut;
	}
	catch( ModbusRTU::mbException& ex )
	{
		dlog[Debug::WARN]  << "(query): " << ex << endl;
	}
	catch(SystemError& err)
	{
		dlog[Debug::WARN] << "(query): " << err << endl;
	}
	catch(Exception& ex)
	{
		dlog[Debug::WARN] << "(query): " << ex << endl;
	}
	catch( ost::SockException& e ) 
	{
		dlog[Debug::WARN] << e.getString() << ": " << e.getSystemErrorString() << endl;
	}
	catch(...)
	{
		dlog[Debug::WARN] << "(query): cath..." << endl;
	}	

	return erHardwareError;
}
// -------------------------------------------------------------------------
void ModbusTCPMaster::reconnect()
{
	if( tcp )
	{
		tcp->disconnect();
		delete tcp;
	}
	
	tcp = new ost::TCPStream(iaddr.c_str());
}
// -------------------------------------------------------------------------
void ModbusTCPMaster::connect( ost::InetAddress addr, int port )
{
	if( !tcp )
	{
		ostringstream s;
		s << addr << ":" << port;
		
		if( dlog.debugging(Debug::INFO) )
			dlog[Debug::INFO] << "(ModbusTCPMaster): connect to " << s.str() << endl;
		
		iaddr = s.str();
		tcp = new ost::TCPStream(iaddr.c_str());
	}
}
// -------------------------------------------------------------------------
void ModbusTCPMaster::disconnect()
{
	if( !tcp )
		return;
	
//	if( dlog.debugging(Debug::INFO) )
//		dlog[Debug::INFO] << "(ModbusTCPMaster): disconnect." << endl;

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
