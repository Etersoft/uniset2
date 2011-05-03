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
udp(0),
recvTimeout(5000),
lostTimeout(5000),
lostPackets(0),
activated(false),
r_thr(0),
u_thr(0),
pnum(0),
maxDifferens(1000),
waitClean(false),
rnum(0),
maxProcessingCount(100),
icache(200),
cache_init_ok(false)
{
	{
		ostringstream s;
		s << "(" << s_host << ":" << port << ")";
		myname = s.str();
	}

	ost::Thread::setException(ost::Thread::throwException);
	try
	{
//		ost::IPV4Cidr ci(s_host.c_str());
//		addr = ci.getBroadcast();
//		cerr << "****************** addr: " << addr << endl;
		addr = s_host.c_str();
		udp = new ost::UDPDuplex(addr,port);
	}
	catch( std::exception& e )
	{
		ostringstream s;
		s << "Could not create connection for " << s_host << ":" << port << " err: " << e.what();
		dlog[Debug::CRIT] << myname << "(init): " << s.str() << std::endl;
		throw SystemError(s.str());
	}
	catch( ... )
	{
		ostringstream s;
		s << "Could not create connection for " << s_host << ":" << port;
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
void UNetReceiver::setReceiveTimeout( timeout_t msec )
{
	recvTimeout = msec;
	ptRecvTimeout.setTiming(msec);
}
// -----------------------------------------------------------------------------
void UNetReceiver::setLostTimeout( timeout_t msec )
{
	lostTimeout = msec;
	ptLostTimeout.setTiming(msec);
}
// -----------------------------------------------------------------------------
void UNetReceiver::setReceivePause( timeout_t msec )
{
	recvpause = msec;
}
// -----------------------------------------------------------------------------
void UNetReceiver::setUpdatePause( timeout_t msec )
{
	updatepause = msec;
}
// -----------------------------------------------------------------------------
void UNetReceiver::setMaxProcessingCount( int set )
{
	maxProcessingCount = set;
}
// -----------------------------------------------------------------------------
void UNetReceiver::setMaxDifferens( unsigned long set )
{
	maxDifferens = set;
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
	// обрабатываем пока, очередь либо не опустеет
	// либо обнаружится "дырка" в последовательности
	// но при этом обрабатываем не больше maxProcessingCount
	// за один раз..
	int k = maxProcessingCount;
	while( k>0 )
	{
		{ // lock qpack
			uniset_mutex_lock l(packMutex);
			if( qpack.empty() )
				return;

			p = qpack.top();
			unsigned long sub = labs(p.msg.header.num - pnum);
			if( pnum > 0 )
			{
				// если sub > maxDifferens
				// значит это просто "разрыв"
				// и нам ждать lostTimeout не надо
				// сразу начинаем обрабатывать новые пакеты
				// а если > 1 && < maxDifferens
				// значит это временная "дырка"
				// и надо подождать lostTimeout
				// чтобы констатировать потерю пакета..
				if( sub > 1 && sub < maxDifferens )
				{
					if( !ptLostTimeout.checkTime() )
						return;

					lostPackets++;
				}
				else if( p.msg.header.num == pnum )
				{
				   /* а что делать если идут повторные пакеты ?!
					* для надёжности лучше обрабатывать..
					* для "оптимизации".. лучше игнорировать
					*/
					qpack.pop(); // пока выбрали вариант "оптимизации"
					continue;
				}
			}

			ptLostTimeout.reset();

			// удаляем из очереди, только если
			// всё в порядке с последовательностью..
			qpack.pop();
			pnum = p.msg.header.num;
		} // unlock qpack

		k--;

//		cerr << myname << "(update): " << p.msg.header << endl;

		initCache(p, !cache_init_ok);

		for( size_t i=0; i<p.msg.header.dcount; i++ )
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
void UNetReceiver::stop()
{
	activated = false;
//	msleep(10);
//	u_thr->stop();
//	r_thr->stop();
}
// -----------------------------------------------------------------------------
void UNetReceiver::receive()
{
	dlog[Debug::INFO] << myname << ": ******************* receive start" << endl;
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
			dlog[Debug::WARN] << myname << "(receive): " << e.getString() << endl;
		}
		catch( UniSetTypes::Exception& ex)
		{
			dlog[Debug::WARN] << myname << "(receive): " << ex << std::endl;
		}
		catch(...)
		{
			dlog[Debug::WARN] << myname << "(receive): catch ..." << std::endl;
		}

		msleep(recvpause);
	}

	dlog[Debug::INFO] << myname << ": ************* receive FINISH **********" << endl;
}
// -----------------------------------------------------------------------------
bool UNetReceiver::recv()
{
	if( !udp->isInputReady(recvTimeout) )
		return false;

	size_t ret = udp->UDPReceive::receive(&(pack.msg),sizeof(pack.msg));
	if( ret < sizeof(UniSetUDP::UDPHeader) )
	{
		dlog[Debug::CRIT] << myname << "(receive): FAILED header ret=" << ret << " sizeof=" << sizeof(UniSetUDP::UDPHeader) << endl;
		return false;
	}

	size_t sz = pack.msg.header.dcount * sizeof(UniSetUDP::UDPData) + sizeof(UniSetUDP::UDPHeader);
	if( ret < sz )
	{
		dlog[Debug::CRIT] << myname << "(receive): FAILED data ret=" << ret << " sizeof=" << sz
			  << " packnum=" << pack.msg.header.num << endl;
		return false;
	}


	if( rnum>0 && labs(pack.msg.header.num - rnum) > maxDifferens )
	{
		/* А что делать если мы уже ждём и ещё не "разгребли предыдущее".. а тут уже повторный "разрыв"
		 * Можно откинуть всё.. что сложили во временную очередь и заново "копить" (но тогда теряем информацию)
		 * А можно породолжать складывать во временную, но тогда есть риск "никогда" не разгрести временную
		 * очередь, при "частых обрывах". Потому-что update будет на каждом разрыве ждать ещё lostTimeout..
		 */
		// Пока выбираю.. чистить qtmp. Это будет соотвествовать логике работы с картами у которых ограничен буфер приёма.
		// Обычно "кольцевой". Т.е. если не успели обработать и "вынуть" из буфера информацию.. он будет переписан новыми данными
		if( waitClean )
		{
			dlog[Debug::CRIT] << myname << "(receive): reset qtmp.." << endl;
			while( !qtmp.empty() )
				qtmp.pop();
		}

		waitClean = true;
	}

	rnum = pack.msg.header.num;

#if 0
	cerr << myname << "(receive): recv DATA OK. ret=" << ret << " sizeof=" << sz
		  << " header: " << pack.msg.header
		  << " waitClean=" << waitClean
		  << endl;
	for( size_t i=0; i<pack.msg.header.dcount; i++ )
	{
		UniSetUDP::UDPData& d = pack.msg.dat[i];
		cerr << "****** save id=" << d.id << " val=" << d.val << endl;
	}
#endif

	{	// lock qpack
		uniset_mutex_lock l(packMutex,500);
		if( !waitClean )
		{
			qpack.push(pack);
			return true;
		}

		if( !qpack.empty() )
		{
//			cerr << myname << "(receive): copy to qtmp..."
//							  << " header: " << pack.msg.header
//							  << endl;
			qtmp.push(pack);
		}
		else
		{
//		  	cerr << myname << "(receive): copy from qtmp..." << endl;
			// очередь освободилась..
			// то копируем в неё всё что набралось...
			while( !qtmp.empty() )
			{
				qpack.push(qtmp.top());
				qtmp.pop();
			}

			// не забываем и текущий поместить в очередь..
			qpack.push(pack);
			waitClean = false;
		}
	}	// unlock qpack

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
	 for( size_t i=0; i<icache.size(); i++ )
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
