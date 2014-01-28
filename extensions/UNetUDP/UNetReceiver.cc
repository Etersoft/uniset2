#include <sstream>
#include <iomanip>
#include "Exceptions.h"
#include "Extensions.h"
#include "UNetReceiver.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// -----------------------------------------------------------------------------
/*
bool UNetReceiver::PacketCompare::operator()(const UniSetUDP::UDPMessage& lhs,
											const UniSetUDP::UDPMessage& rhs) const
{
//	if( lhs.num == rhs.num )
//		return (lhs < rhs);

	return lhs.num > rhs.num;
}
*/
// ------------------------------------------------------------------------------------------
UNetReceiver::UNetReceiver( const std::string& s_host, const ost::tpport_t port, SMInterface* smi ):
shm(smi),
recvpause(10),
updatepause(100),
udp(0),
recvTimeout(5000),
prepareTime(2000),
lostTimeout(5000),
lostPackets(0),
sidRespond(UniSetTypes::DefaultObjectId),
respondInvert(false),
sidLostPackets(UniSetTypes::DefaultObjectId),
activated(false),
r_thr(0),
u_thr(0),
pnum(0),
maxDifferens(1000),
waitClean(false),
rnum(0),
maxProcessingCount(100),
lockUpdate(false),
d_icache(UniSetUDP::MaxDCount),
a_icache(UniSetUDP::MaxACount),
d_cache_init_ok(false),
a_cache_init_ok(false)
{
	{
		ostringstream s;
		s << "R(" << setw(15) << s_host << ":" << setw(4) << port << ")";
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
		s << myname << ": " << e.what();
		dlog[Debug::CRIT] << s.str() << std::endl;
		throw SystemError(s.str());
	}
	catch( ... )
	{
		ostringstream s;
		s << myname << ": catch...";
		dlog[Debug::CRIT] << s.str() << std::endl;
		throw SystemError(s.str());
	}

	r_thr = new ThreadCreator<UNetReceiver>(this, &UNetReceiver::receive);
	u_thr = new ThreadCreator<UNetReceiver>(this, &UNetReceiver::update);

	ptRecvTimeout.setTiming(recvTimeout);
	ptPrepare.setTiming(prepareTime);
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
void UNetReceiver::setPrepareTime( timeout_t msec )
{
	prepareTime = msec;
	ptPrepare.setTiming(msec);
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
void UNetReceiver::setRespondID( UniSetTypes::ObjectId id, bool invert )
{
	sidRespond = id;
	respondInvert = invert;
	shm->initDIterator(ditRespond);
}
// -----------------------------------------------------------------------------
void UNetReceiver::setLostPacketsID( UniSetTypes::ObjectId id )
{
	sidLostPackets = id;
	shm->initAIterator(aitLostPackets);
}
// -----------------------------------------------------------------------------
void UNetReceiver::setLockUpdate( bool st )
{ 
	uniset_mutex_lock l(lockMutex,200);
	lockUpdate = st;
	if( !st )
	  ptPrepare.reset();
}
// -----------------------------------------------------------------------------
void UNetReceiver::resetTimeout()
{ 
	uniset_mutex_lock l(tmMutex,200);
	ptRecvTimeout.reset();
	trTimeout.change(false);
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
	cerr << "******************* update start" << endl;
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

		if( sidRespond!=DefaultObjectId )
		{
			try
			{
				bool r = respondInvert ? !isRecvOK() : isRecvOK();
				shm->localSaveState(ditRespond,sidRespond,r,shm->ID());
			}
			catch(Exception& ex)
			{
				dlog[Debug::CRIT] << myname << "(step): (respond) " << ex << std::endl;
			}
		}

		if( sidLostPackets!=DefaultObjectId )
		{
			try
			{
				shm->localSaveValue(aitLostPackets,sidLostPackets,getLostPacketsNum(),shm->ID());
			}
			catch(Exception& ex)
			{
				dlog[Debug::CRIT] << myname << "(step): (lostPackets) " << ex << std::endl;
			}
		}

		msleep(updatepause);
	}
}
// -----------------------------------------------------------------------------
void UNetReceiver::real_update()
{
	UniSetUDP::UDPMessage p;
	// обрабатываем, пока очередь либо не опустеет,
	// либо обнаружится "дырка" в последовательности,
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
			unsigned long sub = labs(p.num - pnum);
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
				else if( p.num == pnum )
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
			pnum = p.num;
		} // unlock qpack

		k--;

//		cerr << myname << "(update): " << p.msg.header << endl;

		initDCache(p, !d_cache_init_ok);
		initACache(p, !a_cache_init_ok);

		// Обработка дискретных
		size_t nbit = 0;
		for( size_t i=0; i<p.dcount; i++, nbit++ )
		{
			try
			{

				long id = p.dID(i);
				bool val = p.dValue(i);

				ItemInfo& ii(d_icache[i]);
				if( ii.id != id )
				{
					dlog[Debug::WARN] << myname << "(update): reinit cache for sid=" << id << endl;
					ii.id = id;
					shm->initAIterator(ii.ait);
					shm->initDIterator(ii.dit);
				}

				// обновление данных в SM (блокировано)
				{
					uniset_mutex_lock l(lockMutex,100);
					if( lockUpdate )
						continue;
				}

				if( ii.iotype == UniversalIO::DigitalInput )
					shm->localSaveState(ii.dit,id,val,shm->ID());
				else if( ii.iotype == UniversalIO::AnalogInput )
					shm->localSaveValue(ii.ait,id,val,shm->ID());
				else if( ii.iotype == UniversalIO::AnalogOutput )
					shm->localSetValue(ii.ait,id,val,shm->ID());
				else if( ii.iotype == UniversalIO::DigitalOutput )
					shm->localSetState(ii.dit,id,val,shm->ID());
				else
					dlog[Debug::CRIT] << myname << "(update): Unknown iotype for sid=" << id << endl;
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

		// Обработка аналоговых
		for( size_t i=0; i<p.acount; i++ )
		{
			try
			{
				UniSetUDP::UDPAData& d = p.a_dat[i];
				ItemInfo& ii(a_icache[i]);
				if( ii.id != d.id )
				{
					dlog[Debug::WARN] << myname << "(update): reinit cache for sid=" << d.id << endl;
					ii.id = d.id;
					shm->initAIterator(ii.ait);
					shm->initDIterator(ii.dit);
				}

				// обновление данных в SM (блокировано)
				{
					uniset_mutex_lock l(lockMutex,100);
					if( lockUpdate )
						continue;
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
	bool tout = false;
	while( activated )
	{
		try
		{
			if( recv() )
			{
				uniset_mutex_lock l(tmMutex,100);
				ptRecvTimeout.reset();
			}
		}
		catch( UniSetTypes::Exception& ex)
		{
			dlog[Debug::WARN] << myname << "(receive): " << ex << std::endl;
		}
		catch( std::exception& e )
		{
			dlog[Debug::WARN] << myname << "(receive): " << e.what()<< std::endl;
		}
		catch(...)
		{
			dlog[Debug::WARN] << myname << "(receive): catch ..." << std::endl;
		}

		// делаем через промежуточную переменную
		// чтобы поскорее освободить mutex
		{
			uniset_mutex_lock l(tmMutex,100);
			tout = ptRecvTimeout.checkTime();
		}

		// только если "режим подготовки закончился, то можем генерировать "события"
		if( ptPrepare.checkTime() && trTimeout.change(tout) )
		{
			if( tout )
				slEvent(this,evTimeout);
			else
				slEvent(this,evOK);
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

	size_t ret = udp->UDPReceive::receive((char*)(r_buf.data),sizeof(r_buf.data));

	size_t sz = UniSetUDP::UDPMessage::getMessage(pack,r_buf);
	if( sz == 0 )
	{
		dlog[Debug::CRIT] << myname << "(receive): FAILED RECEIVE DATA ret=" << ret << endl;
		return false;
	}

	if( rnum>0 && labs(pack.num - rnum) > maxDifferens )
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

	rnum = pack.num;

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
		uniset_mutex_lock l(packMutex,2000);
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
	for( ItemVec::iterator it=d_icache.begin(); it!=d_icache.end(); ++it )
	{
		shm->initAIterator(it->ait);
		shm->initDIterator(it->dit);
	}
	for( ItemVec::iterator it=a_icache.begin(); it!=a_icache.end(); ++it )
	{
		shm->initAIterator(it->ait);
		shm->initDIterator(it->dit);
	}
}
// -----------------------------------------------------------------------------
void UNetReceiver::initDCache( UniSetUDP::UDPMessage& pack, bool force )
{
	 if( !force && pack.dcount == d_icache.size() )
		  return;

	 dlog[Debug::INFO] << myname << ": init icache.." << endl;
	 d_cache_init_ok = true;

	 d_icache.resize(pack.dcount);
	 for( size_t i=0; i<d_icache.size(); i++ )
	 {
		  ItemInfo& d(d_icache[i]);

		  if( d.id != pack.d_id[i] )
		  {
				d.id = pack.d_id[i];
				d.iotype = conf->getIOType(d.id);
				shm->initAIterator(d.ait);
				shm->initDIterator(d.dit);
		  }
	 }
}
// -----------------------------------------------------------------------------
void UNetReceiver::initACache( UniSetUDP::UDPMessage& pack, bool force )
{
	 if( !force && pack.acount == a_icache.size() )
		  return;

	 dlog[Debug::INFO] << myname << ": init icache.." << endl;
	 a_cache_init_ok = true;

	 a_icache.resize(pack.acount);
	 for( size_t i=0; i<a_icache.size(); i++ )
	 {
		  ItemInfo& d(a_icache[i]);
		  if( d.id != pack.a_dat[i].id )
		  {
				d.id = pack.a_dat[i].id;
				d.iotype = conf->getIOType(d.id);
				shm->initAIterator(d.ait);
				shm->initDIterator(d.dit);
		  }
	 }
}
// -----------------------------------------------------------------------------
void UNetReceiver::connectEvent( UNetReceiver::EventSlot sl )
{
	slEvent = sl;
}
// -----------------------------------------------------------------------------
