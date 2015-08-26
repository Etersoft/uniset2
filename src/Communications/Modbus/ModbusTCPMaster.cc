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
	tcp(nullptr),
	nTransaction(0),
	iaddr(""),
	force_disconnect(true)
{
	setCRCNoCheckit(true);
	/*
	    dlog->addLevel(Debug::INFO);
	    dlog->addLevel(Debug::WARN);
	    dlog->addLevel(Debug::CRIT);
	*/
}

// -------------------------------------------------------------------------
ModbusTCPMaster::~ModbusTCPMaster()
{
	if( isConnection() )
		disconnect();

	tcp.reset();
}
// -------------------------------------------------------------------------
int ModbusTCPMaster::getNextData( unsigned char* buf, int len )
{
	return ModbusTCPCore::getNextData(tcp.get(), qrecv, buf, len);
}
// -------------------------------------------------------------------------
void ModbusTCPMaster::setChannelTimeout( timeout_t msec )
{
	if( tcp )
	{
		tcp->setTimeout(msec);
		tcp->setKeepAliveParams((msec > 1000 ? msec / 1000 : 1));
	}
}
// -------------------------------------------------------------------------
mbErrCode ModbusTCPMaster::sendData( unsigned char* buf, int len )
{
	return ModbusTCPCore::sendData(tcp.get(), buf, len);
}
// -------------------------------------------------------------------------
mbErrCode ModbusTCPMaster::query( ModbusAddr addr, ModbusMessage& msg,
								  ModbusMessage& reply, timeout_t timeout )
{
	try
	{
		if( iaddr.empty() )
		{
			if( dlog->is_warn() )
				dlog->warn() << iaddr << "(ModbusTCPMaster::query): unknown ip address for server..." << endl;

			return erTimeOut; // erHardwareError
		}

		if( !isConnection() )
		{
			if( dlog->is_info() )
				dlog->info() << iaddr << "(ModbusTCPMaster::query): no connection.. reconnnect..." << endl;

			reconnect();
		}

		if( !isConnection() )
		{
			if( dlog->is_warn() )
				dlog->warn() << iaddr << "(ModbusTCPMaster::query): not connected to server..." << endl;

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
		if( dlog->is_info() )
		{
			dlog->info() << iaddr << "(ModbusTCPMaster::query): send tcp header(" << sizeof(mh) << "): ";
			mbPrintMessage( dlog->info(), (ModbusByte*)(&mh), sizeof(mh));
			dlog->info() << endl;
		}

		for( unsigned int i = 0; i < 2; i++ )
		{
			(*tcp) << mh;

			// send PDU
			mbErrCode res = send(msg);

			if( res != erNoError )
				return res;

			if( tcp->isPending(ost::Socket::pendingOutput, timeout) )
				break;

			if( dlog->is_info() )
				dlog->info() << "(ModbusTCPMaster::query): no write pending.. reconnnect.." << endl;

			reconnect();

			if( !isConnection() )
			{
				if( dlog->is_warn() )
					dlog->warn() << "(ModbusTCPMaster::query): not connected to server..." << endl;

				return erTimeOut;
			}

			cleanInputStream();

			if( dlog->is_info() )
				dlog->info() << "(ModbusTCPMaster::query): no write pending.. reconnnect OK" << endl;
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

		if( tcp->isPending(ost::Socket::pendingInput, timeout) )
		{
			/*
			            unsigned char rbuf[100];
			            memset(rbuf,0,sizeof(rbuf));
			            int ret = getNextData(rbuf,sizeof(rbuf));
			            cerr << "ret=" << ret << " recv: ";
			            for( unsigned int i=0; i<sizeof(rbuf); i++ )
			                cerr << hex << " 0x" <<  (int)rbuf[i];
			            cerr << endl;
			*/
			ModbusTCP::MBAPHeader rmh;
			int ret = getNextData((unsigned char*)(&rmh), sizeof(rmh));

			if( dlog->is_info() )
			{
				dlog->info() << "(ModbusTCPMaster::query): recv tcp header(" << ret << "): ";
				mbPrintMessage( dlog->info(), (ModbusByte*)(&rmh), sizeof(rmh));
				dlog->info(false) << endl;
			}

			if( ret < (int)sizeof(rmh) )
			{
				ost::tpport_t port;

				if( dlog->is_warn() )
					dlog->warn() << "(ModbusTCPMaster::query): ret=" << (int)ret
								 << " < rmh=" << (int)sizeof(rmh)
								 << " errnum: " << tcp->getErrorNumber()
								 << " perr: " << tcp->getPeer(&port)
								 << " err: " << string(tcp->getErrorString())
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

			timeout = ptTimeout.getLeft(timeout);

			if( timeout <= 0 )
			{

				if( dlog->is_warn() )
					dlog->warn() << "(ModbusTCPMaster::query): processing reply timeout.." << endl;

				return erTimeOut; // return erHardwareError;
			}

			mbErrCode res = recv(addr, msg.func, reply, timeout);

			if( force_disconnect )
			{
				if( dlog->is_info() )
					dlog->info() << "(query): force disconnect.." << endl;

				disconnect();
			}

			return res;
		}

		if( dlog->is_info() )
			dlog->info() << "(query): input pending timeout=" << timeout << endl;

		if( force_disconnect )
		{
			if( dlog->is_info() )
				dlog->info() << "(query): force disconnect.." << endl;

			//            cleanInputStream();
			disconnect();
		}

		return erTimeOut;
	}
	catch( ModbusRTU::mbException& ex )
	{
		if( dlog->is_warn() )
			dlog->warn() << "(query): " << ex << endl;
	}
	catch( SystemError& err )
	{
		if( dlog->is_warn() )
			dlog->warn() << "(query): " << err << endl;
	}
	catch( const Exception& ex )
	{
		if( dlog->is_warn() )
			dlog->warn() << "(query): " << ex << endl;
	}
	catch( const ost::SockException& e )
	{
		if( dlog->is_warn() )
			dlog->warn() << "(query): tcp error: " << e.getString() << endl;

		return erTimeOut;
	}
	catch( const std::exception& e )
	{
		if( dlog->is_warn() )
			dlog->crit() << "(query): " << e.what() << std::endl;

		return erTimeOut;
	}

	return erTimeOut; // erHardwareError
}
// -------------------------------------------------------------------------
void ModbusTCPMaster::cleanInputStream()
{
	unsigned char buf[100];
	int ret = 0;

	do
	{
		ret = getNextData(buf, sizeof(buf));
	}
	while( ret > 0);
}
// -------------------------------------------------------------------------
bool ModbusTCPMaster::checkConnection( const std::string& ip, int port, int timeout_msec )
{
	try
	{
		ostringstream s;
		s << ip << ":" << port;

		// Проверяем просто попыткой создать соединение..
		UTCPStream t;
		t.create(ip, port, true, timeout_msec);
		t.setKeepAliveParams( (timeout_msec > 1000 ? timeout_msec / 1000 : 1), 1, 1);
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
	if( dlog->is_info() )
		dlog->info() << "(ModbusTCPMaster): reconnect " << iaddr << ":" << port << endl;

	if( tcp )
	{
		tcp->disconnect();
		tcp.reset();
	}

	try
	{
		tcp = make_shared<UTCPStream>();
		tcp->create(iaddr, port, true, 500);
		tcp->setTimeout(replyTimeOut_ms);
		tcp->setKeepAliveParams((replyTimeOut_ms > 1000 ? replyTimeOut_ms / 1000 : 1));
	}
	catch( const std::exception& e )
	{
		if( dlog->debugging(Debug::CRIT) )
		{
			ostringstream s;
			s << "(ModbusTCPMaster): connection " << s.str() << " error: " << e.what();
			dlog->crit() << s.str() << std::endl;
		}
	}
	catch( ... )
	{
		if( dlog->debugging(Debug::CRIT) )
		{
			ostringstream s;
			s << "(ModbusTCPMaster): connection " << s.str() << " error: catch ...";
			dlog->crit() << s.str() << std::endl;
		}
	}
}
// -------------------------------------------------------------------------
void ModbusTCPMaster::connect( const std::string& addr, int _port )
{
	ost::InetAddress ia(addr.c_str());
	connect(ia, _port);
}
// -------------------------------------------------------------------------
void ModbusTCPMaster::connect( ost::InetAddress addr, int _port )
{
	if( tcp )
	{
		disconnect();
		tcp.reset();
	}

	//    if( !tcp )
	//    {

	ostringstream s;
	s << addr;
	iaddr = s.str();
	port = _port;

	if( dlog->is_info() )
		dlog->info() << "(ModbusTCPMaster): connect to " << iaddr << ":" << port << endl;

	ost::Thread::setException(ost::Thread::throwException);

	try
	{
		tcp = make_shared<UTCPStream>();
		tcp->create(iaddr, port, true, 500);
		tcp->setTimeout(replyTimeOut_ms);
		tcp->setKeepAliveParams((replyTimeOut_ms > 1000 ? replyTimeOut_ms / 1000 : 1));
	}
	catch( const std::exception& e )
	{
		if( dlog->debugging(Debug::CRIT) )
		{
			ostringstream s;
			s << "(ModbusTCPMaster): connection " << s.str() << " error: " << e.what();
			dlog->crit() << s.str() << std::endl;
		}
	}
	catch( ... )
	{
		if( dlog->debugging(Debug::CRIT) )
		{
			ostringstream s;
			s << "(ModbusTCPMaster): connection " << s.str() << " error: catch ...";
			dlog->crit() << s.str() << std::endl;
		}
	}

	//    }
}
// -------------------------------------------------------------------------
void ModbusTCPMaster::disconnect()
{
	if( dlog->is_info() )
		dlog->info() << iaddr << "(ModbusTCPMaster): disconnect (" << iaddr << ":" << port << ")." << endl;

	if( !tcp )
		return;

	tcp->disconnect();
	tcp.reset();
}
// -------------------------------------------------------------------------
bool ModbusTCPMaster::isConnection()
{
	return tcp && tcp->isConnected();
}
// -------------------------------------------------------------------------
