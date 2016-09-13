/*
 * Copyright (c) 2015 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Lesser Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
// -------------------------------------------------------------------------
#include <string.h>
#include <errno.h>
#include <iostream>
#include <sstream>
#include <Poco/Net/NetException.h>
#include "Exceptions.h"
#include "modbus/ModbusTCPMaster.h"
#include "modbus/ModbusTCPCore.h"
// -------------------------------------------------------------------------
using namespace std;
using namespace ModbusRTU;
using namespace UniSetTypes;
using namespace Poco;
// -------------------------------------------------------------------------
ModbusTCPMaster::ModbusTCPMaster():
	tcp(nullptr),
	nTransaction(0),
	iaddr(""),
	force_disconnect(true)
{
	setCRCNoCheckit(true);

	//	 dlog->level(Debug::ANY);
}

// -------------------------------------------------------------------------
ModbusTCPMaster::~ModbusTCPMaster()
{
	if( isConnection() )
		disconnect();
}
// -------------------------------------------------------------------------
size_t ModbusTCPMaster::getNextData( unsigned char* buf, size_t len )
{
	return ModbusTCPCore::getNextData(tcp.get(), qrecv, buf, len );
}
// -------------------------------------------------------------------------
void ModbusTCPMaster::setChannelTimeout( timeout_t msec )
{
	if( !tcp )
		return;

	Poco::Timespan tm = UniSetTimer::millisecToPoco(msec);
	Poco::Timespan old = tcp->getReceiveTimeout();;

	if( old.microseconds() == tm.microseconds() )
		return;

	tcp->setReceiveTimeout(tm);

	int oldKeepAlive = keepAliveTimeout;
	keepAliveTimeout = (msec > 1000 ? msec / 1000 : 1);

	// т.к. каждый раз не вызывать дорогой системный вызов
	// смотрим меняется ли значение
	if( oldKeepAlive != keepAliveTimeout )
		tcp->setKeepAliveParams(keepAliveTimeout);
}
// -------------------------------------------------------------------------
mbErrCode ModbusTCPMaster::sendData( unsigned char* buf, size_t len )
{
	return ModbusTCPCore::sendData(tcp.get(), buf, len);
}
// -------------------------------------------------------------------------
mbErrCode ModbusTCPMaster::query( ModbusAddr addr, ModbusMessage& msg,
								  ModbusMessage& reply, timeout_t timeout_msec )
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

		assert(timeout_msec);
		ptTimeout.setTiming(timeout_msec);

		tcp->setReceiveTimeout( UniSetTimer::millisecToPoco(timeout_msec) );
		msg.makeHead(++nTransaction, crcNoCheckit);

		for( size_t i = 0; i < 2; i++ )
		{
			if( tcp->poll(UniSetTimer::millisecToPoco(timeout_msec), Poco::Net::Socket::SELECT_WRITE) )
			{
				mbErrCode res = send(msg);

				if( res != erNoError )
					return res;

				break;
			}

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

		if( timeout_msec != UniSetTimer::WaitUpTime )
		{
			timeout_msec = ptTimeout.getLeft(timeout_msec);

			if( timeout_msec == 0 )
				return erTimeOut;

			ptTimeout.setTiming(timeout_msec);
		}

		// чистим очередь
		//        cleanInputStream();
		while( !qrecv.empty() )
			qrecv.pop();

		//tcp->sync();

		if( tcp->poll(UniSetTimer::millisecToPoco(timeout_msec), Poco::Net::Socket::SELECT_READ ) )
		{
			size_t ret = 0;

			while( !ptTimeout.checkTime() )
			{
				ret = getNextData((unsigned char*)(&reply.aduhead), sizeof(reply.aduhead));

				if( ret == sizeof(reply.aduhead) )
					break;
			}

			if( ret > 0 && dlog->is_info() )
			{
				dlog->info() << "(ModbusTCPMaster::query): recv tcp header(" << ret << "): ";
				mbPrintMessage( dlog->info(false), (ModbusByte*)(&reply.aduhead), sizeof(reply.aduhead));
				dlog->info(false) << endl;
			}

			if( ret < sizeof(reply.aduhead) )
			{
				if( dlog->is_warn() )
				{
					try
					{
						Poco::Net::SocketAddress  iaddr = tcp->peerAddress();

						dlog->warn() << "(ModbusTCPMaster::query): ret=" << ret
									 << " < rmh=" << sizeof(reply.aduhead)
									 << " perr: " << iaddr.host().toString() << ":" << iaddr.port()
									 << endl;
					}
					catch( const Poco::Net::NetException& ex )
					{
						if( dlog->is_warn() )
							dlog->warn() << "(query): tcp error: " << ex.displayText() << endl;
					}
				}

				cleanInputStream();
				tcp->forceDisconnect();
				return erTimeOut; // return erHardwareError;
			}

			reply.swapHead();

			if( dlog->is_level9() )
				dlog->level9() << "(ModbusTCPMaster::query): ADU len=" << reply.aduLen()
							   << endl;

			if( reply.tID() != msg.tID() )
			{
				if( dlog->is_warn() )
					dlog->warn() << "(ModbusTCPMaster::query): tID=" << reply.tID()
								 << " != " << msg.tID()
								 << " (len=" << reply.len() << ")"
								 << endl;

				cleanInputStream();
				return  erBadReplyNodeAddress;
			}

			if( reply.pID() != 0 )
			{
				cleanInputStream();
				return  erBadReplyNodeAddress;
			}

			//
			timeout_msec = ptTimeout.getLeft(timeout_msec);

			if( timeout_msec <= 0 )
			{

				if( dlog->is_warn() )
					dlog->warn() << "(ModbusTCPMaster::query): processing reply timeout.." << endl;

				return erTimeOut; // return erHardwareError;
			}

			//msg.aduhead = reply.aduhead;
			mbErrCode res = recv(addr, msg.func(), reply, timeout_msec);

			if( force_disconnect )
			{
				if( dlog->is_info() )
					dlog->info() << "(query): force disconnect.." << endl;

				// при штатном обмене..лучше дождаться конца "посылки"..
				// поэтому применяем disconnect(), а не forceDisconnect()
				// (с учётом выставленной опции setLinger(true))
				tcp->close();
			}

			return res;
		}

		if( dlog->is_info() )
			dlog->info() << "(query): input pending timeout=" << timeout_msec << endl;

		if( force_disconnect )
		{
			if( dlog->is_info() )
				dlog->info() << "(query): force disconnect.." << endl;

			//            cleanInputStream();
			tcp->forceDisconnect();
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
	catch( const UniSetTypes::CommFailed& ex )
	{
		if( dlog->is_crit() )
			dlog->crit() << "(query): " << ex << endl;

		if( tcp )
			tcp->forceDisconnect();
	}
	catch( const UniSetTypes::Exception& ex )
	{
		if( dlog->is_warn() )
			dlog->warn() << "(query): " << ex << endl;
	}
	catch( const Poco::Net::NetException& e )
	{
		if( dlog->is_warn() )
			dlog->warn() << "(query): tcp error: " << e.displayText() << endl;
	}
	catch( const std::exception& e )
	{
		if( dlog->is_warn() )
			dlog->crit() << "(query): " << e.what() << std::endl;
	}

	return erTimeOut; // erHardwareError
}
// -------------------------------------------------------------------------
void ModbusTCPMaster::cleanInputStream()
{
	unsigned char buf[100];
	int ret = 0;

	try
	{
		do
		{
			ret = getNextData(buf, sizeof(buf));
		}
		while( ret > 0);
	}
	catch( ... ) {}
}
// -------------------------------------------------------------------------
void ModbusTCPMaster::setReadTimeout( timeout_t msec )
{
	readTimeout = msec;
}
// -------------------------------------------------------------------------
timeout_t ModbusTCPMaster::getReadTimeout() const
{
	return readTimeout;
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
		t.create(ip, port, timeout_msec);
		t.setKeepAliveParams( (timeout_msec > 1000 ? timeout_msec / 1000 : 1), 1, 1);
		t.setNoDelay(true);
		//t.shutdown();
		t.close();
		return true;
	}
	catch(...)
	{
	}

	return false;
}
// -------------------------------------------------------------------------
bool ModbusTCPMaster::reconnect()
{
	if( dlog->is_info() )
		dlog->info() << "(ModbusTCPMaster): reconnect " << iaddr << ":" << port << endl;

	if( tcp )
	{
		tcp->forceDisconnect();
		tcp = nullptr;
	}

	return connect(iaddr, port, true);
}
// -------------------------------------------------------------------------
bool ModbusTCPMaster::connect( const std::string& addr, int _port, bool closeOldConnection ) noexcept
{
	try
	{
		Net::SocketAddress sa(addr, _port);
		return connect(sa, _port, closeOldConnection);
	}
	catch( const std::exception& e )
	{
		if( dlog->debugging(Debug::CRIT) )
		{
			ostringstream s;
			s << "(ModbusTCPMaster): connect " << iaddr << ":" << port << " error: " << e.what();
			dlog->crit() << iaddr << std::endl;
		}
	}

	if( closeOldConnection && tcp )
	{
		forceDisconnect();
		tcp = nullptr;
	}

	return false;
}
// -------------------------------------------------------------------------
bool ModbusTCPMaster::connect( const Poco::Net::SocketAddress& addr, int _port, bool closeOldConnection ) noexcept
{
	if( tcp )
	{
		if( !closeOldConnection )
			return false;

		//disconnect();
		forceDisconnect();
		tcp = nullptr;
	}

	iaddr = addr.host().toString();
	port = _port;

	if( dlog->is_info() )
		dlog->info() << "(ModbusTCPMaster): connect to " << iaddr << ":" << port << endl;

	try
	{
		tcp = make_shared<UTCPStream>();
		tcp->create(iaddr, port, 500);
		//tcp->connect(addr,500);
		tcp->setReceiveTimeout(UniSetTimer::millisecToPoco(replyTimeOut_ms));
		tcp->setKeepAlive(true); // tcp->setKeepAliveParams((replyTimeOut_ms > 1000 ? replyTimeOut_ms / 1000 : 1));
		tcp->setNoDelay(true);
		return true;
	}
	catch( Poco::Net::NetException& ex)
	{
		if( dlog->debugging(Debug::CRIT) )
		{
			ostringstream s;
			s << "(ModbusTCPMaster): create connection " << iaddr << ":" << port << " error: " << ex.displayText();
			dlog->crit() << iaddr << std::endl;
		}
	}
	catch( const std::exception& e )
	{
		if( dlog->debugging(Debug::CRIT) )
		{
			ostringstream s;
			s << "(ModbusTCPMaster): connection " << iaddr << ":" << port << " error: " << e.what();
			dlog->crit() << iaddr << std::endl;
		}
	}
	catch( ... )
	{
		if( dlog->debugging(Debug::CRIT) )
		{
			ostringstream s;
			s << "(ModbusTCPMaster): connection " << iaddr << ":" << port << " error: catch ...";
			dlog->crit() << s.str() << std::endl;
		}
	}

	tcp = nullptr;
	return false;
}
// -------------------------------------------------------------------------
void ModbusTCPMaster::disconnect()
{
	if( dlog->is_info() )
		dlog->info() << iaddr << "(ModbusTCPMaster): disconnect (" << iaddr << ":" << port << ")." << endl;

	if( !tcp )
		return;

	tcp->close();
	tcp = nullptr;
}
// -------------------------------------------------------------------------
void ModbusTCPMaster::forceDisconnect()
{
	if( dlog->is_info() )
		dlog->info() << iaddr << "(ModbusTCPMaster): FORCE disconnect (" << iaddr << ":" << port << ")." << endl;

	if( !tcp )
		return;

	tcp->forceDisconnect();
	tcp = nullptr;
}
// -------------------------------------------------------------------------
bool ModbusTCPMaster::isConnection() const
{
	return tcp && tcp->isConnected();
}
// -------------------------------------------------------------------------
