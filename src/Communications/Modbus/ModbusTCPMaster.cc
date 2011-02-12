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
				dlog[Debug::WARN] << "(query): unknown ip address for server..." << endl;
				return erHardwareError;
		}

		if( !isConnection() )
		{
				if( dlog.debugging(Debug::INFO) )
						dlog[Debug::INFO] << "(ModbusTCPMaster): no connection.. reconnnect..." << endl;
				reconnect();
		}

		if( !isConnection() )
		{
				dlog[Debug::WARN] << "(query): not connected to server..." << endl;
				return erTimeOut;
		}

		assert(timeout);
		ptTimeout.setTiming(timeout);

		tcp->setTimeout(timeout);

//		ost::Thread::setException(ost::Thread::throwException);
//		ost::tpport_t port;
//		cerr << "****** peer: " << tcp->getPeer(&port) << " err: " << tcp->getErrorNumber() << endl;

		if( nTransaction >= numeric_limits<ModbusRTU::ModbusData>::max() )
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

		for( int i=0; i<2; i++ )
		{
			(*tcp) << mh;

			// send PDU
			mbErrCode res = send(msg);
			if( res!=erNoError )
				return res;

			if( tcp->isPending(ost::Socket::pendingOutput,timeout) )
				break;

			if( dlog.debugging(Debug::INFO) )
				dlog[Debug::INFO] << "(ModbusTCPMaster::query): no write pending.. reconnnect.." << endl;

			reconnect();
			if( !isConnection() )
			{
				dlog[Debug::WARN] << "(ModbusTCPMaster::query): not connected to server..." << endl;
				return erTimeOut;
			}
			cleanInputStream();
		
			if( dlog.debugging(Debug::INFO) )
				dlog[Debug::INFO] << "(ModbusTCPMaster::query): no write pending.. reconnnect OK" << endl;
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
//		cleanInputStream();
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
			if( dlog.debugging(Debug::INFO) )
			{
				dlog[Debug::INFO] << "(ModbusTCPMaster::query): recv tcp header(" << ret << "): ";
				mbPrintMessage( dlog, (ModbusByte*)(&rmh), sizeof(rmh));
				dlog(Debug::INFO) << endl;
			}

			if( ret < (int)sizeof(rmh) )
			{
				ost::tpport_t port;
				if( dlog.debugging(Debug::INFO) )
					dlog[Debug::INFO] << "(ModbusTCPMaster::query): ret=" << (int)ret
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

//			return recv(addr,msg.func,reply,timeout);
			mbErrCode res = recv(addr,msg.func,reply,timeout);
			
			if( force_disconnect )
			{
				if( dlog.debugging(Debug::INFO) )
					dlog[Debug::INFO] << "(query): force disconnect.." << endl;
			
				disconnect();
			}

			return res;
		}

		if( dlog.debugging(Debug::INFO) )
			dlog[Debug::INFO] << "(query): input pending timeout " << endl;

		if( force_disconnect )
		{
			if( dlog.debugging(Debug::INFO) )
				dlog[Debug::INFO] << "(query): force disconnect.." << endl;
			
//			cleanInputStream();
			disconnect();
		}

		return erTimeOut;
	}
	catch( ModbusRTU::mbException& ex )
	{
		dlog[Debug::WARN]  << "(query): " << ex << endl;
	}
	catch( SystemError& err )
	{
		dlog[Debug::WARN] << "(query): " << err << endl;
	}
	catch( Exception& ex )
	{
		dlog[Debug::WARN] << "(query): " << ex << endl;
	}
	catch( ost::SockException& e ) 
	{
		dlog[Debug::WARN] << "(query): tcp error: " << e.getString() << endl;
		return erTimeOut;
	}
	catch(...)
	{
		dlog[Debug::WARN] << "(query): cath..." << endl;
	}	

	return erHardwareError;
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
void ModbusTCPMaster::reconnect()
{
	if( dlog.debugging(Debug::INFO) )
		dlog[Debug::INFO] << "(ModbusTCPMaster): reconnect " << iaddr << endl;

	if( tcp )
	{
//		cerr << "tcp diconnect..." << endl;
		tcp->disconnect();
		delete tcp;
		tcp = 0;
	}

	ost::Thread::setException(ost::Thread::throwException);

	try
	{
  		// 	TCPStream (const char *name, Family family=IPV4, unsigned mss=536, bool throwflag=false, timeout_t timer=0)
		tcp = new ost::TCPStream(iaddr.c_str(),ost::Socket::IPV4,536,true,500);
		tcp->setTimeout(replyTimeOut_ms);
	}
	catch(ost::Socket *socket)
	{
		  ost::tpport_t port;
		  int err = socket->getErrorNumber();
		  ost::InetAddress saddr = (ost::InetAddress)socket->getPeer(&port);
		  dlog[Debug::CRIT] << "tcp error " << saddr.getHostname() << ":" << port << " = " << err << endl;
	}
	catch( ost::SockException& e)
	{
		dlog[Debug::CRIT] << "tcp error: " << e.getString() << endl;
	}
	catch(...)
	{
		dlog[Debug::CRIT] << "create TCPStream[" << iaddr << "] error..." << endl;
	}
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

//	if( !tcp )
//	{
		ostringstream s;
		s << addr << ":" << port;
		
		if( dlog.debugging(Debug::INFO) )
			dlog[Debug::INFO] << "(ModbusTCPMaster): connect to " << s.str() << endl;
		
		iaddr = s.str();
		try
		{		
			tcp = new ost::TCPStream(iaddr.c_str());
			tcp->setTimeout(replyTimeOut_ms);
		}
		catch(ost::Socket *socket)
		{
			  ost::tpport_t port;
			  int err = socket->getErrorNumber();
			  ost::InetAddress saddr = (ost::InetAddress)socket->getPeer(&port);
			  dlog[Debug::CRIT] << ": tcp error " << saddr.getHostname() << ":" << port << " = " << err << endl;
		}
		catch( ost::SockException& e)
		{
			  dlog[Debug::CRIT] << "tcp error: " << e.getString() << endl;
		}
		catch(...)
		{
			dlog[Debug::CRIT] << "create TCPStream[" << iaddr << "] error..." << endl;
		}			
//	}
}
// -------------------------------------------------------------------------
void ModbusTCPMaster::disconnect()
{
	if( dlog.debugging(Debug::INFO) )
		dlog[Debug::INFO] << "(ModbusTCPMaster): disconnect." << endl;

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
