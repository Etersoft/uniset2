#include <sstream>
#include <iomanip>
#include "Exceptions.h"
#include "Extensions.h"
#include "UNetReceiver.h"
#include "UNetLogSugar.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// -----------------------------------------------------------------------------
/*
bool UNetReceiver::PacketCompare::operator()(const UniSetUDP::UDPMessage& lhs,
                                            const UniSetUDP::UDPMessage& rhs) const
{
//    if( lhs.num == rhs.num )
//        return (lhs < rhs);

    return lhs.num > rhs.num;
}
*/
// ------------------------------------------------------------------------------------------
UNetReceiver::UNetReceiver( const std::string& s_host, const ost::tpport_t port, const std::shared_ptr<SMInterface>& smi ):
	shm(smi),
	recvpause(10),
	updatepause(100),
	recvTimeout(5000),
	prepareTime(2000),
	lostTimeout(200), /* 2*updatepause */
	lostPackets(0),
	sidRespond(UniSetTypes::DefaultObjectId),
	respondInvert(false),
	sidLostPackets(UniSetTypes::DefaultObjectId),
	activated(false),
	pnum(0),
	maxDifferens(20),
	waitClean(false),
	rnum(0),
	maxProcessingCount(100),
	lockUpdate(false),
	d_cache_init_ok(false),
	a_cache_init_ok(false)
{
	{
		ostringstream s;
		s << "R(" << setw(15) << s_host << ":" << setw(4) << port << ")";
		myname = s.str();
	}

	unetlog = make_shared<DebugStream>();
	unetlog->setLogName(myname);

	auto conf = uniset_conf();
	conf->initLogStream(unetlog, myname);

	ost::Thread::setException(ost::Thread::throwException);

	try
	{
		//        ost::IPV4Cidr ci(s_host.c_str());
		//        addr = ci.getBroadcast();
		//        cerr << "****************** addr: " << addr << endl;
		addr = s_host.c_str();
		udp = make_shared<ost::UDPDuplex>(addr, port);
	}
	catch( const std::exception& e )
	{
		ostringstream s;
		s << myname << ": " << e.what();
		unetcrit << s.str() << std::endl;
		throw SystemError(s.str());
	}
	catch( ... )
	{
		ostringstream s;
		s << myname << ": catch...";
		unetcrit << s.str() << std::endl;
		throw SystemError(s.str());
	}

	r_thr = make_shared< ThreadCreator<UNetReceiver> >(this, &UNetReceiver::receive);
	u_thr = make_shared< ThreadCreator<UNetReceiver> >(this, &UNetReceiver::update);

	ptRecvTimeout.setTiming(recvTimeout);
	ptPrepare.setTiming(prepareTime);
}
// -----------------------------------------------------------------------------
UNetReceiver::~UNetReceiver()
{
}
// -----------------------------------------------------------------------------
void UNetReceiver::setReceiveTimeout( timeout_t msec )
{
	uniset_rwmutex_wrlock l(tmMutex);
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
	shm->initIterator(itRespond);
}
// -----------------------------------------------------------------------------
void UNetReceiver::setLostPacketsID( UniSetTypes::ObjectId id )
{
	sidLostPackets = id;
	shm->initIterator(itLostPackets);
}
// -----------------------------------------------------------------------------
void UNetReceiver::setLockUpdate( bool st )
{
	uniset_rwmutex_wrlock l(lockMutex);
	lockUpdate = st;

	if( !st )
		ptPrepare.reset();
}
// -----------------------------------------------------------------------------
void UNetReceiver::resetTimeout()
{
	uniset_rwmutex_wrlock l(tmMutex);
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
	else
		forceUpdate();
}
// -----------------------------------------------------------------------------
void UNetReceiver::update()
{
	unetinfo << myname << "(update): start.." << endl;

	while(activated)
	{
		try
		{
			real_update();
		}
		catch( UniSetTypes::Exception& ex)
		{
			unetcrit << myname << "(update): " << ex << std::endl;
		}
		catch(...)
		{
			unetcrit << myname << "(update): catch ..." << std::endl;
		}

		if( sidRespond != DefaultObjectId )
		{
			try
			{
				bool r = respondInvert ? !isRecvOK() : isRecvOK();
				shm->localSetValue(itRespond, sidRespond, ( r ? 1 : 0 ), shm->ID());
			}
			catch( const Exception& ex )
			{
				unetcrit << myname << "(step): (respond) " << ex << std::endl;
			}
		}

		if( sidLostPackets != DefaultObjectId )
		{
			try
			{
				shm->localSetValue(itLostPackets, sidLostPackets, getLostPacketsNum(), shm->ID());
			}
			catch( const Exception& ex )
			{
				unetcrit << myname << "(step): (lostPackets) " << ex << std::endl;
			}
		}

		msleep(updatepause);
	}
}
// -----------------------------------------------------------------------------
void UNetReceiver::forceUpdate()
{
	uniset_rwmutex_wrlock l(packMutex);
	pnum = 0; // сбрасываем запомненый номер последнего обработанного пакета
	// и тем самым заставляем обновить данные в SM (см. real_update)
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

	while( k > 0 )
	{
		{
			// lock qpack
			uniset_rwmutex_wrlock l(packMutex);

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
					// если p.num < pnum, то это какой-то "дубль",
					// т.к мы все пакеты <= pnum уже "отработали".
					// а значит можно не ждать, а откидывать пакет и
					// дальше работать..
					if( p.num < pnum )
					{
						qpack.pop();
						continue;
					}

					if( !ptLostTimeout.checkTime() )
						return;

					unetwarn << myname << "(update): lostTimeout(" << ptLostTimeout.getInterval() << ")! pnum=" << p.num << " lost " << sub << " packets " << endl;
					lostPackets += sub;
				}
				else if( p.num == pnum )
				{
					/* а что делать если идут повторные пакеты ?!
					 * для надёжности лучше обрабатывать..
					 * для "оптимизации".. лучше игнорировать
					 */
					qpack.pop(); // пока выбрали вариант "оптимизации" (выкидываем из очереди и идём дальше)
					continue;
				}

				if( sub >= maxDifferens )
				{
					// считаем сколько пакетов потеряли.. (pnum=0 - означает что мы только что запустились...)
					if( pnum != 0 && p.num > pnum )
					{
						lostPackets += sub;
						unetwarn << myname << "(update): sub=" <<  sub << " > maxDifferenst(" << maxDifferens << ")! lost " << sub << " packets " << endl;
					}
				}
			}

			ptLostTimeout.reset();

			// удаляем из очереди, только если
			// всё в порядке с последовательностью..
			qpack.pop();
			pnum = p.num;
		} // unlock qpack

		k--;

		//        cerr << myname << "(update): " << p.msg.header << endl;

		initDCache(p, !d_cache_init_ok);
		initACache(p, !a_cache_init_ok);

		// Обработка дискретных
		CacheInfo& d_iv = d_icache_map[p.getDataID()];

		for( size_t i = 0; i < p.dcount; i++ )
		{
			try
			{
				long id = p.dID(i);
				bool val = p.dValue(i);

				CacheItem& ii(d_iv.cache[i]);

				if( ii.id != id )
				{
					unetwarn << myname << "(update): reinit cache for sid=" << id << endl;
					ii.id = id;
					shm->initIterator(ii.ioit);
				}

				// обновление данных в SM (блокировано)
				{
					uniset_rwmutex_rlock l(lockMutex);

					if( lockUpdate )
						continue;
				}

				shm->localSetValue(ii.ioit, id, val, shm->ID());
			}
			catch( const UniSetTypes::Exception& ex)
			{
				unetcrit << myname << "(update): " << ex << std::endl;
			}
			catch(...)
			{
				unetcrit << myname << "(update): catch ..." << std::endl;
			}
		}

		// Обработка аналоговых
		CacheInfo& a_iv = a_icache_map[p.getDataID()];

		for( size_t i = 0; i < p.acount; i++ )
		{
			try
			{
				UniSetUDP::UDPAData& d = p.a_dat[i];

				CacheItem& ii(a_iv.cache[i]);

				if( ii.id != d.id )
				{
					unetwarn << myname << "(update): reinit cache for sid=" << d.id << endl;
					ii.id = d.id;
					shm->initIterator(ii.ioit);
				}

				// обновление данных в SM (блокировано)
				{
					uniset_rwmutex_rlock l(lockMutex);

					if( lockUpdate )
						continue;
				}

				shm->localSetValue(ii.ioit, d.id, d.val, shm->ID());
			}
			catch( const UniSetTypes::Exception& ex)
			{
				unetcrit << myname << "(update): " << ex << std::endl;
			}
			catch(...)
			{
				unetcrit << myname << "(update): catch ..." << std::endl;
			}
		}
	}
}

// -----------------------------------------------------------------------------
void UNetReceiver::stop()
{
	activated = false;
}
// -----------------------------------------------------------------------------
void UNetReceiver::receive()
{
	unetinfo << myname << ": ******************* receive start" << endl;

	{
		uniset_rwmutex_wrlock l(tmMutex);
		ptRecvTimeout.setTiming(recvTimeout);
	}

	bool tout = false;

	while( activated )
	{
		try
		{
			if( recv() )
			{
				uniset_rwmutex_wrlock l(tmMutex);
				ptRecvTimeout.reset();
			}
		}
		catch( UniSetTypes::Exception& ex)
		{
			unetwarn << myname << "(receive): " << ex << std::endl;
		}
		catch( const std::exception& e )
		{
			unetwarn << myname << "(receive): " << e.what() << std::endl;
		}

		/*
		        catch(...)
		        {
					unetwarn << myname << "(receive): catch ..." << std::endl;
		        }
		*/
		// делаем через промежуточную переменную
		// чтобы поскорее освободить mutex
		{
			uniset_rwmutex_rlock l(tmMutex);
			tout = ptRecvTimeout.checkTime();
		}

		// только если "режим подготовки закончился, то можем генерировать "события"
		if( ptPrepare.checkTime() && trTimeout.change(tout) )
		{
			if( tout )
				slEvent(shared_from_this(), evTimeout);
			else
				slEvent(shared_from_this(), evOK);
		}

		msleep(recvpause);
	}

	unetinfo << myname << ": ************* receive FINISH **********" << endl;
}
// -----------------------------------------------------------------------------
bool UNetReceiver::recv()
{
	if( !udp->isInputReady(recvTimeout) )
		return false;

	size_t ret = udp->UDPReceive::receive((char*)(r_buf.data), sizeof(r_buf.data));

	size_t sz = UniSetUDP::UDPMessage::getMessage(pack, r_buf);

	if( sz == 0 )
	{
		unetcrit << myname << "(receive): FAILED RECEIVE DATA ret=" << ret << endl;
		return false;
	}

	if( pack.magic != UniSetUDP::UNETUDP_MAGICNUM )
	{
		// пакет не нашей "системы"
		return false;
	}

	if( rnum > 0 && labs(pack.num - rnum) > maxDifferens )
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
			unetcrit << myname << "(receive): reset qtmp.." << endl;

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

	for( size_t i = 0; i < pack.msg.header.dcount; i++ )
	{
		UniSetUDP::UDPData& d = pack.msg.dat[i];
		cerr << "****** save id=" << d.id << " val=" << d.val << endl;
	}

#endif

	{
		// lock qpack
		uniset_rwmutex_wrlock l(packMutex);

		if( !waitClean )
		{
			qpack.push(pack);
			return true;
		}

		if( !qpack.empty() )
		{
			//            cerr << myname << "(receive): copy to qtmp..."
			//                              << " header: " << pack.msg.header
			//                              << endl;
			qtmp.push(pack);
		}
		else
		{
			//              cerr << myname << "(receive): copy from qtmp..." << endl;
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
	}    // unlock qpack

	return true;
}
// -----------------------------------------------------------------------------
void UNetReceiver::initIterators()
{
	for( auto mit = d_icache_map.begin(); mit != d_icache_map.end(); ++mit )
	{
		CacheVec& d_icache(mit->second.cache);

		for( auto && it : d_icache )
			shm->initIterator(it.ioit);
	}

	for( auto mit = a_icache_map.begin(); mit != a_icache_map.end(); ++mit )
	{
		CacheVec& a_icache(mit->second.cache);

		for( auto && it : a_icache )
			shm->initIterator(it.ioit);
	}
}
// -----------------------------------------------------------------------------
void UNetReceiver::initDCache( UniSetUDP::UDPMessage& pack, bool force )
{
	CacheInfo& d_info(d_icache_map[pack.getDataID()]);

	if( !force && pack.dcount == d_info.cache.size() )
		return;

	if( d_info.cache_init_ok && pack.dcount == d_info.cache.size() )
	{
		d_cache_init_ok = true;
		auto it = d_icache_map.begin();

		for( ; it != d_icache_map.end(); ++it )
		{
			CacheInfo& d_info(it->second);
			d_cache_init_ok = d_cache_init_ok && d_info.cache_init_ok;

			if(d_cache_init_ok == false)
				break;
		}

		return;
	}

	unetinfo << myname << ": init dcache for " << pack.getDataID() << endl;

	d_info.cache_init_ok = true;
	d_info.cache.resize(pack.dcount);

	size_t sz = d_info.cache.size();
	auto conf = uniset_conf();

	for( size_t i = 0; i < sz; i++ )
	{
		CacheItem& d(d_info.cache[i]);

		if( d.id != pack.d_id[i] )
		{
			d.id = pack.d_id[i];
			d.iotype = conf->getIOType(d.id);
			shm->initIterator(d.ioit);
		}
	}
}
// -----------------------------------------------------------------------------
void UNetReceiver::initACache( UniSetUDP::UDPMessage& pack, bool force )
{
	CacheInfo& a_info(a_icache_map[pack.getDataID()]);

	if( !force && pack.acount == a_info.cache.size() )
		return;

	if( a_info.cache_init_ok && pack.acount == a_info.cache.size() )
	{
		a_cache_init_ok = true;
		auto it = a_icache_map.begin();

		for( ; it != a_icache_map.end(); ++it )
		{
			CacheInfo& a_info(it->second);
			a_cache_init_ok = a_cache_init_ok && a_info.cache_init_ok;

			if(a_cache_init_ok == false)
				break;
		}

		return;
	}

	unetinfo << myname << ": init icache for " << pack.getDataID() << endl;
	a_info.cache_init_ok = true;
	auto conf = uniset_conf();

	a_info.cache.resize(pack.acount);

	size_t sz = a_info.cache.size();

	for( size_t i = 0; i < sz; i++ )
	{
		CacheItem& d(a_info.cache[i]);

		if( d.id != pack.a_dat[i].id )
		{
			d.id = pack.a_dat[i].id;
			d.iotype = conf->getIOType(d.id);
			shm->initIterator(d.ioit);
		}
	}
}
// -----------------------------------------------------------------------------
void UNetReceiver::connectEvent( UNetReceiver::EventSlot sl )
{
	slEvent = sl;
}
// -----------------------------------------------------------------------------
const std::string UNetReceiver::getShortInfo() const
{
	// warning: будет вызываться из другого потока
	// (считаем что чтение безопасно)

	ostringstream s;

	s << setw(15) << std::right << getAddress() << ":" << std::left << setw(6) << getPort()
	  << "[ " << setw(7) << ( isLockUpdate() ? "PASSIVE" : "ACTIVE" ) << " ]"
	  << "    recvOK=" << isRecvOK()
	  << " receivepack=" << rnum
	  << " lostPackets=" << setw(6) << getLostPacketsNum()
	  << endl
	  << "\t["
	  << " recvTimeout=" << setw(6) << recvTimeout
	  << " prepareTime=" << setw(6) << prepareTime
	  << " lostTimeout=" << setw(6) << lostTimeout
	  << " recvpause=" << setw(6) << recvpause
	  << " updatepause=" << setw(6) << updatepause
	  << " maxDifferens=" << setw(6) << maxDifferens
	  << " maxProcessingCount=" << setw(6) << maxProcessingCount
	  << " waitClean=" << waitClean
	  << " ]";

	return std::move(s.str());
}
// -----------------------------------------------------------------------------
