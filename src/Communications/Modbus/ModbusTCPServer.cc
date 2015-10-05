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
ModbusTCPServer::ModbusTCPServer( ost::InetAddress& ia, int port ):
	TCPSocket(ia, port),
	iaddr(ia),
	ignoreAddr(false),
	maxSessions(10),
	sessCount(0),
	sessTimeout(10000),
	cancelled(false)
{
	setCRCNoCheckit(true);
}

// -------------------------------------------------------------------------
ModbusTCPServer::~ModbusTCPServer()
{
	terminate();
}
// -------------------------------------------------------------------------
std::unordered_set<ModbusAddr> ModbusTCPServer::addr2vaddr(ModbusAddr& mbaddr)
{
	std::unordered_set<ModbusRTU::ModbusAddr> v;
	v.emplace(mbaddr);
	return std::move(v);
}
// -------------------------------------------------------------------------
void ModbusTCPServer::setMaxSessions( unsigned int num )
{
	if( num < sessCount )
	{
		uniset_mutex_lock l(sMutex);

		int k = sessCount - num;
		int d = 0;

		for( SessionList::reverse_iterator i = slist.rbegin(); d < k && i != slist.rend(); ++i, d++ )
			delete *i;

		sessCount = num;
	}

	maxSessions = num;
}
// -------------------------------------------------------------------------
unsigned ModbusTCPServer::getCountSessions()
{
	return sessCount;
}
// -------------------------------------------------------------------------
void ModbusTCPServer::setSessionTimeout( timeout_t msec )
{
	sessTimeout = msec;
}
// -------------------------------------------------------------------------
bool ModbusTCPServer::waitQuery(const std::unordered_set<ModbusAddr>& vmbaddr, timeout_t msec )
{
	if( msec == 0 )
		msec = UniSetTimer::WaitUpTime;

	if( sessCount >= maxSessions )
		return false;

	if( cancelled )
		return false;

	try
	{
		if( isPendingConnection(msec) )
		{
			if( cancelled )
				return false;

			ModbusTCPSession* s = new ModbusTCPSession(*this, vmbaddr, sessTimeout);

			s->connectReadCoil( sigc::mem_fun(this, &ModbusTCPServer::readCoilStatus) );
			s->connectReadInputStatus( sigc::mem_fun(this, &ModbusTCPServer::readInputStatus) );
			s->connectReadOutput( sigc::mem_fun(this, &ModbusTCPServer::readOutputRegisters) );
			s->connectReadInput( sigc::mem_fun(this, &ModbusTCPServer::readInputRegisters) );
			s->connectForceSingleCoil( sigc::mem_fun(this, &ModbusTCPServer::forceSingleCoil) );
			s->connectForceCoils( sigc::mem_fun(this, &ModbusTCPServer::forceMultipleCoils) );
			s->connectWriteOutput( sigc::mem_fun(this, &ModbusTCPServer::writeOutputRegisters) );
			s->connectWriteSingleOutput( sigc::mem_fun(this, &ModbusTCPServer::writeOutputSingleRegister) );
			s->connectMEIRDI( sigc::mem_fun(this, &ModbusTCPServer::read4314) );
			s->connectSetDateTime( sigc::mem_fun(this, &ModbusTCPServer::setDateTime) );
			s->connectDiagnostics( sigc::mem_fun(this, &ModbusTCPServer::diagnostics) );
			s->connectFileTransfer( sigc::mem_fun(this, &ModbusTCPServer::fileTransfer) );
			s->connectJournalCommand( sigc::mem_fun(this, &ModbusTCPServer::journalCommand) );
			s->connectRemoteService( sigc::mem_fun(this, &ModbusTCPServer::remoteService) );
			s->connectFileTransfer( sigc::mem_fun(this, &ModbusTCPServer::fileTransfer) );

			s->setAfterSendPause(aftersend_msec);
			s->setReplyTimeout(replyTimeout_ms);
			s->setRecvTimeout(recvTimeOut_ms);
			s->setSleepPause(sleepPause_usec);
			s->setCleanBeforeSend(cleanBeforeSend);

			s->setLog(dlog);
			s->connectFinalSession( sigc::mem_fun(this, &ModbusTCPServer::sessionFinished) );

			{
				uniset_mutex_lock l(sMutex);
				slist.push_back(s);
				sessCount++;
			}

			s->detach();
			return true;
		}
	}
	catch( ost::Exception& e )
	{
		if( dlog->is_warn() )
			dlog->warn() << "(ModbusTCPServer): " << e.what() << endl;
	}

	return false;
}
// -------------------------------------------------------------------------
mbErrCode ModbusTCPServer::receive(const std::unordered_set<ModbusAddr>& vmbaddr, timeout_t timeout )
{
	PassiveTimer ptTimeout(timeout);
	ModbusMessage buf;
	mbErrCode res = erTimeOut;

	//  Thread::setException(Thread::throwException);
	//  #warning "Why timeout can be 0 there?"
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
				memset(&curQueryHeader, 0, sizeof(curQueryHeader));
				res = tcp_processing(tcp, curQueryHeader);

				if( res != erNoError )
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

				if( qrecv.empty() )
				{
					tcp.disconnect();
					return erTimeOut;
				}

				unsigned char q_addr = qrecv.front();

				res = recv( vmbaddr, buf, timeout );

				if( res != erNoError ) // && res!=erBadReplyNodeAddress )
				{
					if( res < erInternalErrorCode )
					{
						ErrorRetMessage em( q_addr, buf.func, res );
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

				if( res != erNoError )
				{
					tcp.disconnect();
					return res;
				}

				// processing message...
				res = processing(buf);
			}

			//            tcp << "exiting now" << endl;
			tcp.disconnect();
		}
	}
	catch( const ost::Exception& e )
	{
		if( dlog->is_crit() )
			dlog->crit() << "(ModbusTCPServer): " << e.what() << endl;
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
	return ModbusTCPCore::sendData(&tcp, buf, len);
}
// -------------------------------------------------------------------------
int ModbusTCPServer::getNextData( unsigned char* buf, int len )
{
	return ModbusTCPCore::getNextData(&tcp, qrecv, buf, len);
}
// --------------------------------------------------------------------------------

mbErrCode ModbusTCPServer::tcp_processing( ost::TCPStream& tcp, ModbusTCP::MBAPHeader& mhead )
{
	if( !tcp.isConnected() )
		return erTimeOut;

	// чистим очередь
	while( !qrecv.empty() )
		qrecv.pop();

	unsigned int len = getNextData((unsigned char*)(&mhead), sizeof(mhead));

	if( len < sizeof(mhead) )
		return erInvalidFormat;

	mhead.swapdata();

	if( dlog->is_info() )
	{
		dlog->info() << "(ModbusTCPServer::tcp_processing): recv tcp header(" << len << "): ";
		mbPrintMessage( dlog->info(), (ModbusByte*)(&mhead), sizeof(mhead));
		dlog->info() << endl;
	}

	// check header
	if( mhead.pID != 0 )
		return erUnExpectedPacketType; // erTimeOut;

	len = ModbusTCPCore::readNextData(&tcp, qrecv, mhead.len);

	if( len < mhead.len )
	{
		if( dlog->is_info() )
			dlog->info() << "(ModbusTCPServer::tcp_processing): len(" << (int)len
						 << ") < mhead.len(" << (int)mhead.len << ")" << endl;

		return erInvalidFormat;
	}

	return erNoError;
}
// -------------------------------------------------------------------------
ModbusRTU::mbErrCode ModbusTCPServer::post_send_request( ModbusRTU::ModbusMessage& request )
{
	tcp << endl;
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

	if( dlog->is_info() )
	{
		dlog->info() << "(ModbusTCPServer::pre_send_request): send tcp header: ";
		mbPrintMessage( dlog->info(), (ModbusByte*)(&curQueryHeader), sizeof(curQueryHeader));
		dlog->info() << endl;
	}

	tcp << curQueryHeader;
	curQueryHeader.swapdata();

	return erNoError;
}
// -------------------------------------------------------------------------
void ModbusTCPServer::cleanInputStream()
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
void ModbusTCPServer::terminate()
{
	cancelled = true;

	if( dlog->is_info() )
		dlog->info() << "(ModbusTCPServer): terminate..." << endl;

	if( tcp && tcp.isConnected() )
		tcp.disconnect();

	uniset_mutex_lock l(sMutex);

	for( const auto& s : slist )
	{
		try
		{
			s->terminate();
		}
		catch(...) {}
	}
}
// -------------------------------------------------------------------------
void ModbusTCPServer::sessionFinished( ModbusTCPSession* s )
{
	uniset_mutex_lock l(sMutex);

	for( auto i = slist.begin(); i != slist.end(); ++i )
	{
		if( (*i) == s )
		{
			slist.erase(i);
			sessCount--;
			return;
		}
	}
}
// -------------------------------------------------------------------------
void ModbusTCPServer::getSessions( Sessions& lst )
{
	uniset_mutex_lock l(sMutex);

	for( const auto& i : slist )
	{
		SessionInfo inf( i->getClientAddress(), i->getAskCount() );
		lst.push_back(inf);
	}
}
// -------------------------------------------------------------------------
