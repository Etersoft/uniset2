#include <sstream>
#include "Exceptions.h"
#include "Extensions.h"
#include "UNetReceiver.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// -----------------------------------------------------------------------------
bool UNetReceiver::PacketCompare::operator()(const UniSetUDP::UDPMessage& lhs,
											const UniSetUDP::UDPMessage& rhs) const
{
//	if( lhs.msg.header.num == rhs.msg.header.num )
//		return (lhs.msg < rhs.msg);

	return lhs.msg.header.num > rhs.msg.header.num;
}
// ------------------------------------------------------------------------------------------
UNetReceiver::UNetReceiver( const std::string s_host, const ost::tpport_t port, SMInterface* smi ):
shm(smi),
recvpause(10),
updatepause(100),
recvTimeout(5000),
udp(0),
activated(false),
r_thr(0),
u_thr(0),
minBufSize(30),
maxProcessingCount(100),
icache(200),
cache_init_ok(false)
{
	{
		ostringstream s;
		s << "(" << s_host << ":" << port << ")";
		myname = s.str();
	}

	try
	{
//		ost::IPV4Cidr ci(s_host.c_str());
//		addr = ci.getBroadcast();
//		cerr << "****************** addr: " << addr << endl;
		addr = s_host.c_str();
		udp = new ost::UDPDuplex(addr,port);
	}
	catch( ost::SockException& e )
	{
		ostringstream s;
		s << e.getString() << ": " << e.getSystemErrorString();
		dlog[Debug::CRIT] << myname << "(init): " << s.str() << std::endl;

		throw SystemError(s.str());
	}

	r_thr = new ThreadCreator<UNetReceiver>(this, &UNetReceiver::receive);
	u_thr = new ThreadCreator<UNetReceiver>(this, &UNetReceiver::update);


	ptRecvTimeout.setTiming(recvTimeout);
}
// -----------------------------------------------------------------------------
UNetReceiver::~UNetReceiver()
{
	delete r_thr;
	delete u_thr;
	delete udp;
}
// -----------------------------------------------------------------------------
void UNetReceiver::setReceiveTimeout( int msec )
{
	recvTimeout = msec;
	ptRecvTimeout.setTiming(msec);
}
// -----------------------------------------------------------------------------
void UNetReceiver::setReceivePause( int msec )
{
	recvpause = msec;
}
// -----------------------------------------------------------------------------
void UNetReceiver::setUpdatePause( int msec )
{
	updatepause = msec;
}
// -----------------------------------------------------------------------------
void UNetReceiver::setMinBudSize( int set )
{
	minBufSize = set;
}
// -----------------------------------------------------------------------------
void UNetReceiver::setMaxProcessingCount( int set )
{
	maxProcessingCount = set;
}
// -----------------------------------------------------------------------------
void UNetReceiver::start()
{
	if( !activated )
	{
		activated = true;
		u_thr->start();
		r_thr->start();
	}
}
// -----------------------------------------------------------------------------
void UNetReceiver::update()
{
	cerr << "******************* udpate start" << endl;
	while(activated)
	{
		try
		{
			real_update();
		}
		catch( UniSetTypes::Exception& ex)
		{
			dlog[Debug::CRIT] << myname << "(update): " << ex << std::endl;
		}
		catch(...)
		{
			dlog[Debug::CRIT] << myname << "(update): catch ..." << std::endl;
		}

		msleep(updatepause);
	}
}
// -----------------------------------------------------------------------------
void UNetReceiver::real_update()
{
	UniSetUDP::UDPMessage p;
	bool buf_ok = false;
	{
		uniset_mutex_lock l(packMutex);
		if( qpack.size() <= minBufSize )
			return;

		buf_ok = true;
	}

	int k = maxProcessingCount;
	while( buf_ok && k>0 )
	{
		{
			uniset_mutex_lock l(packMutex);
			p = qpack.top();
			qpack.pop();
		}

		if( labs(p.msg.header.num - pnum) > 1 )
		{
			dlog[Debug::CRIT] << "************ FAILED! ORDER PACKETS! recv.num=" << pack.msg.header.num
				  << " num=" << pnum << endl;
		}

		pnum = p.msg.header.num;
		k--;

		{
			uniset_mutex_lock l(packMutex);
			buf_ok =  ( qpack.size() > minBufSize );
		}

		initCache(p, !cache_init_ok);

		for( int i=0; i<p.msg.header.dcount; i++ )
		{
			try
			{
				UniSetUDP::UDPData& d = p.msg.dat[i];
				ItemInfo& ii(icache[i]);
				if( ii.id != d.id )
				{
					dlog[Debug::WARN] << myname << "(update): reinit cache for sid=" << d.id << endl;
					ii.id = d.id;
					shm->initAIterator(ii.ait);
					shm->initDIterator(ii.dit);
				}

				if( ii.iotype == UniversalIO::DigitalInput )
					shm->localSaveState(ii.dit,d.id,d.val,shm->ID());
				else if( ii.iotype == UniversalIO::AnalogInput )
					shm->localSaveValue(ii.ait,d.id,d.val,shm->ID());
				else if( ii.iotype == UniversalIO::AnalogOutput )
					shm->localSetValue(ii.ait,d.id,d.val,shm->ID());
 				else if( ii.iotype == UniversalIO::DigitalOutput )
					shm->localSetState(ii.dit,d.id,d.val,shm->ID());
				else
				  dlog[Debug::CRIT] << myname << "(update): Unknown iotype for sid=" << d.id << endl;
			}
			catch( UniSetTypes::Exception& ex)
			{
				dlog[Debug::CRIT] << myname << "(update): " << ex << std::endl;
			}
			catch(...)
			{
				dlog[Debug::CRIT] << myname << "(update): catch ..." << std::endl;
			}
		}
	}
}

// -----------------------------------------------------------------------------
void UNetReceiver::receive()
{
	cerr << "******************* receive start" << endl;
	ptRecvTimeout.setTiming(recvTimeout);
	while( activated )
	{
		try
		{
			if( recv() )
			  ptRecvTimeout.reset();
		}
		catch( ost::SockException& e )
		{
			cerr  << e.getString() << ": " << e.getSystemErrorString() << endl;
		}
		catch( UniSetTypes::Exception& ex)
		{
			cerr << myname << "(poll): " << ex << std::endl;
		}
		catch(...)
		{
			cerr << myname << "(poll): catch ..." << std::endl;
		}	

		msleep(recvpause);
	}

	cerr << "************* execute FINISH **********" << endl;
}
// -----------------------------------------------------------------------------
bool UNetReceiver::recv()
{
	if( !udp->isInputReady(recvTimeout) )
		return false;

	ssize_t ret = udp->UDPReceive::receive(&(pack.msg),sizeof(pack.msg));
	if( ret < sizeof(UniSetUDP::UDPHeader) )
	{
		cerr << myname << "(receive): FAILED header ret=" << ret << " sizeof=" << sizeof(UniSetUDP::UDPHeader) << endl;
		return false;
	}
		
	ssize_t sz = pack.msg.header.dcount * sizeof(UniSetUDP::UDPData) + sizeof(UniSetUDP::UDPHeader);
	if( ret < sz )
	{
		cerr << myname << "(receive): FAILED data ret=" << ret << " sizeof=" << sz << endl;
		return false;
	}
		

//	cerr << myname << "(receive): recv DATA OK. ret=" << ret << " sizeof=" << sz
//		  << " header: " << pack.msg.header << endl;
/*		
	if( labs(pack.msg.header.num - pnum) > 1 )
	{
		cerr << "************ FAILED! ORDER PACKETS! recv.num=" << pack.msg.header.num
				<< " num=" << pnum << endl;
	}
		
	pnum = pack.msg.header.num;
*/
	{
		uniset_mutex_lock l(packMutex);
		qpack.push(pack);
	}

	return true;
}
// -----------------------------------------------------------------------------
void UNetReceiver::initIterators()
{
	for( ItemVec::iterator it=icache.begin(); it!=icache.end(); ++it )
	{
		shm->initAIterator(it->ait);
		shm->initDIterator(it->dit);
	}
}
// -----------------------------------------------------------------------------
void UNetReceiver::initCache( UniSetUDP::UDPMessage& pack, bool force )
{
	 if( !force && pack.msg.header.dcount == icache.size() )
		  return;

	 dlog[Debug::INFO] << myname << ": init icache.." << endl;
	 cache_init_ok = true;

	 icache.resize(pack.msg.header.dcount);
	 for( int i=0; i<icache.size(); i++ )
	 {
		  ItemInfo& d(icache[i]);
		  if( d.id != pack.msg.dat[i].id )
		  {
				d.id = pack.msg.dat[i].id;
				d.iotype = conf->getIOType(d.id);
				shm->initAIterator(d.ait);
				shm->initDIterator(d.dit);
		  }
	 }
}
// -----------------------------------------------------------------------------
