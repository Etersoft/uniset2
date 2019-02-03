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
#include <sstream>
#include <iomanip>
#include <Poco/Net/NetException.h>
#include "unisetstd.h"
#include "Exceptions.h"
#include "Extensions.h"
#include "UNetReceiver.h"
#include "UNetLogSugar.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
using namespace uniset::extensions;
// -----------------------------------------------------------------------------
CommonEventLoop UNetReceiver::loop;
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
UNetReceiver::UNetReceiver(const std::string& s_host, int _port
						   , const std::shared_ptr<SMInterface>& smi
						   , bool nocheckConnection
						   , const std::string& prefix ):
	shm(smi),
	recvpause(10),
	updatepause(100),
	port(_port),
	saddr(s_host, _port),
	recvTimeout(5000),
	prepareTime(2000),
	lostTimeout(200), /* 2*updatepause */
	lostPackets(0),
	sidRespond(uniset::DefaultObjectId),
	respondInvert(false),
	sidLostPackets(uniset::DefaultObjectId),
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

	addr = s_host.c_str();

	ostringstream logname;
	logname << prefix << "-R-" << s_host << ":" << setw(4) << port;

	unetlog = make_shared<DebugStream>();
	unetlog->setLogName(logname.str());

	auto conf = uniset_conf();
	conf->initLogStream(unetlog, prefix + "-log");

	upThread = unisetstd::make_unique< ThreadCreator<UNetReceiver> >(this, &UNetReceiver::updateThread);

	if( !createConnection(nocheckConnection /* <-- это флаг throwEx */) )
		evCheckConnection.set<UNetReceiver, &UNetReceiver::checkConnectionEvent>(this);

	evStatistic.set<UNetReceiver, &UNetReceiver::statisticsEvent>(this);
	evUpdate.set<UNetReceiver, &UNetReceiver::updateEvent>(this);
	evInitPause.set<UNetReceiver, &UNetReceiver::initEvent>(this);
}
// -----------------------------------------------------------------------------
UNetReceiver::~UNetReceiver()
{
}
// -----------------------------------------------------------------------------
void UNetReceiver::setReceiveTimeout( timeout_t msec ) noexcept
{
	std::lock_guard<std::mutex> l(tmMutex);
	recvTimeout = msec;
	ptRecvTimeout.setTiming(msec);
}
// -----------------------------------------------------------------------------
void UNetReceiver::setPrepareTime( timeout_t msec ) noexcept
{
	prepareTime = msec;
	ptPrepare.setTiming(msec);
}
// -----------------------------------------------------------------------------
void UNetReceiver::setCheckConnectionPause( timeout_t msec ) noexcept
{
	checkConnectionTime = (double)msec / 1000.0;

	if( evCheckConnection.is_active() )
		evCheckConnection.start(0, checkConnectionTime);
}
// -----------------------------------------------------------------------------
void UNetReceiver::setLostTimeout( timeout_t msec ) noexcept
{
	lostTimeout = msec;
	ptLostTimeout.setTiming(msec);
}
// -----------------------------------------------------------------------------
void UNetReceiver::setReceivePause( timeout_t msec ) noexcept
{
	recvpause = msec;
}
// -----------------------------------------------------------------------------
void UNetReceiver::setUpdatePause( timeout_t msec ) noexcept
{
	updatepause = msec;

	if( upStrategy == useUpdateEventLoop && evUpdate.is_active() )
		evUpdate.start(0, (float)updatepause / 1000.);
}
// -----------------------------------------------------------------------------
void UNetReceiver::setMaxProcessingCount( int set ) noexcept
{
	maxProcessingCount = set;
}
// -----------------------------------------------------------------------------
void UNetReceiver::setMaxDifferens( unsigned long set ) noexcept
{
	maxDifferens = set;
}
// -----------------------------------------------------------------------------
void UNetReceiver::setEvrunTimeout( timeout_t msec ) noexcept
{
	evrunTimeout = msec;
}
// -----------------------------------------------------------------------------
void UNetReceiver::setInitPause( timeout_t msec ) noexcept
{
	initPause = (msec / 1000.0);
}
// -----------------------------------------------------------------------------
void UNetReceiver::setRespondID( uniset::ObjectId id, bool invert ) noexcept
{
	sidRespond = id;
	respondInvert = invert;
	shm->initIterator(itRespond);
}
// -----------------------------------------------------------------------------
void UNetReceiver::setLostPacketsID( uniset::ObjectId id ) noexcept
{
	sidLostPackets = id;
	shm->initIterator(itLostPackets);
}
// -----------------------------------------------------------------------------
void UNetReceiver::setLockUpdate( bool st ) noexcept
{

	lockUpdate = st;

	if( !st )
		ptPrepare.reset();
}
// -----------------------------------------------------------------------------
bool UNetReceiver::isLockUpdate() const noexcept
{
	return lockUpdate;
}
// -----------------------------------------------------------------------------
bool UNetReceiver::isInitOK() const noexcept
{
	return initOK.load();
}
// -----------------------------------------------------------------------------
void UNetReceiver::resetTimeout() noexcept
{
	std::lock_guard<std::mutex> l(tmMutex);
	ptRecvTimeout.reset();
	trTimeout.change(false);
}
// -----------------------------------------------------------------------------
bool UNetReceiver::isRecvOK() const noexcept
{
	return !ptRecvTimeout.checkTime();
}
// -----------------------------------------------------------------------------
size_t UNetReceiver::getLostPacketsNum() const noexcept
{
	return lostPackets;
}
// -----------------------------------------------------------------------------
bool UNetReceiver::createConnection( bool throwEx )
{
	if( !activated )
		return false;

	try
	{
		udp = unisetstd::make_unique<UDPReceiveU>(addr, port);
		udp->setBlocking(false); // делаем неблокирующее чтение (нужно для libev)
		evReceive.set<UNetReceiver, &UNetReceiver::callback>(this);

		if( upStrategy == useUpdateEventLoop )
			evUpdate.set<UNetReceiver, &UNetReceiver::updateEvent>(this);

		if( evCheckConnection.is_active() )
			evCheckConnection.stop();

		ptRecvTimeout.setTiming(recvTimeout);
		ptPrepare.setTiming(prepareTime);

		if( activated )
			evprepare(loop.evloop());
	}
	catch( const std::exception& e )
	{
		ostringstream s;
		s << myname << "(createConnection): " << e.what();
		unetcrit << s.str() << std::endl;

		if( throwEx )
			throw SystemError(s.str());

		udp = nullptr;
	}
	catch( ... )
	{
		ostringstream s;
		s << myname << "(createConnection): catch...";
		unetcrit << s.str() << std::endl;

		if( throwEx )
			throw SystemError(s.str());

		udp = nullptr;
	}

	return ( udp != nullptr );
}
// -----------------------------------------------------------------------------
void UNetReceiver::start()
{
	unetinfo << myname << ":... start... " << endl;

	if( !activated )
	{
		activated = true;

		if( !loop.async_evrun(this, evrunTimeout) )
		{
			unetcrit << myname << "(start): evrun FAILED! (timeout=" << evrunTimeout << " msec)" << endl;
			std::terminate();
			return;
		}

		if( upStrategy == useUpdateThread && !upThread->isRunning() )
			upThread->start();
	}
	else
		forceUpdate();
}
// -----------------------------------------------------------------------------
void UNetReceiver::evprepare( const ev::loop_ref& eloop ) noexcept
{
	evStatistic.set(eloop);
	evStatistic.start(0, 1.0); // раз в сек

	evInitPause.set(eloop);

	if( upStrategy == useUpdateEventLoop )
	{
		evUpdate.set(eloop);
		evUpdate.start( 0, ((float)updatepause / 1000.) );
	}

	if( !udp )
	{
		evCheckConnection.set(eloop);
		evCheckConnection.start(0, checkConnectionTime);
		evInitPause.stop();
	}
	else
	{
		evReceive.set(eloop);
		evReceive.start(udp->getSocket(), ev::READ);
		evInitPause.start(0);
	}
}
// -----------------------------------------------------------------------------
void UNetReceiver::evfinish( const ev::loop_ref& eloop ) noexcept
{
	activated = false;

	{
		std::lock_guard<std::mutex> l(checkConnMutex);

		if( evCheckConnection.is_active() )
			evCheckConnection.stop();
	}

	if( evReceive.is_active() )
		evReceive.stop();

	if( evStatistic.is_active() )
		evStatistic.stop();

	if( evUpdate.is_active() )
		evUpdate.stop();

	//udp->disconnect();
	udp = nullptr;
}
// -----------------------------------------------------------------------------
void UNetReceiver::forceUpdate() noexcept
{
	pack_guard l(packMutex, upStrategy);
	pnum = 0; // сбрасываем запомненый номер последнего обработанного пакета
	// и тем самым заставляем обновить данные в SM (см. update)
}
// -----------------------------------------------------------------------------
void UNetReceiver::statisticsEvent(ev::periodic& tm, int revents) noexcept
{
	if( EV_ERROR & revents )
	{
		unetcrit << myname << "(statisticsEvent): EVENT ERROR.." << endl;
		return;
	}

	statRecvPerSec = recvCount;
	statUpPerSec = upCount;

	//	unetlog9 << myname << "(statisctics):"
	//			 << " recvCount=" << recvCount << "[per sec]"
	//			 << " upCount=" << upCount << "[per sec]"
	//			 << endl;

	recvCount = 0;
	upCount = 0;
	tm.again();
}
// -----------------------------------------------------------------------------
void UNetReceiver::initEvent( ev::timer& tmr, int revents ) noexcept
{
	if( EV_ERROR & revents )
	{
		unetcrit << myname << "(initEvent): EVENT ERROR.." << endl;
		return;
	}

	initOK.store(true);
	tmr.stop();
}
// -----------------------------------------------------------------------------
void UNetReceiver::update() noexcept
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
			pack_guard l(packMutex, upStrategy);

			if( qpack.empty() )
				return;

			p = qpack.top();
			size_t sub = labs( (long int)(p.num - pnum) );

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

		upCount++;
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
				if( lockUpdate )
					continue;

				shm->localSetValue(ii.ioit, id, val, shm->ID());
			}
			catch( const uniset::Exception& ex)
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
				if( lockUpdate )
					continue;

				shm->localSetValue(ii.ioit, d.id, d.val, shm->ID());
			}
			catch( const uniset::Exception& ex)
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
void UNetReceiver::updateThread() noexcept
{
	while( activated )
	{
		try
		{
			update();
		}
		catch( std::exception& ex )
		{
			unetcrit << myname << "(update_thread): " << ex.what() << endl;
		}

		// смотрим есть ли связь..
		checkConnection();

		if( sidRespond != DefaultObjectId )
		{
			try
			{
				if( isInitOK() )
				{
					bool r = respondInvert ? !isRecvOK() : isRecvOK();
					shm->localSetValue(itRespond, sidRespond, ( r ? 1 : 0 ), shm->ID());
				}
			}
			catch( const std::exception& ex )
			{
				unetcrit << myname << "(update_thread): (respond) " << ex.what() << std::endl;
			}
		}

		if( sidLostPackets != DefaultObjectId )
		{
			try
			{
				shm->localSetValue(itLostPackets, sidLostPackets, getLostPacketsNum(), shm->ID());
			}
			catch( const std::exception& ex )
			{
				unetcrit << myname << "(update_thread): (lostPackets) " << ex.what() << std::endl;
			}
		}

		msleep(updatepause);
	}
}
// -----------------------------------------------------------------------------
void UNetReceiver::callback( ev::io& watcher, int revents ) noexcept
{
	if( EV_ERROR & revents )
	{
		unetcrit << myname << "(callback): EVENT ERROR.." << endl;
		return;
	}

	if( revents & EV_READ )
		readEvent(watcher);
}
// -----------------------------------------------------------------------------
void UNetReceiver::readEvent( ev::io& watcher ) noexcept
{
	if( !activated )
		return;

	try
	{
		if( receive() )
		{
			std::lock_guard<std::mutex> l(tmMutex);
			ptRecvTimeout.reset();
		}
	}
	catch( uniset::Exception& ex)
	{
		unetwarn << myname << "(receive): " << ex << std::endl;
	}
	catch( const std::exception& e )
	{
		unetwarn << myname << "(receive): " << e.what() << std::endl;
	}
}
// -----------------------------------------------------------------------------
void UNetReceiver::checkConnection()
{
	bool tout = false;

	// делаем через промежуточную переменную
	// чтобы поскорее освободить mutex
	{
		std::lock_guard<std::mutex> l(tmMutex);
		tout = ptRecvTimeout.checkTime();
	}

	// только если "режим подготовки закончился, то можем генерировать "события"
	if( ptPrepare.checkTime() && trTimeout.change(tout) )
	{
		auto w = shared_from_this();

		if( w )
		{
			if( tout )
				slEvent(w, evTimeout);
			else
				slEvent(w, evOK);
		}
	}
}
// -----------------------------------------------------------------------------
void UNetReceiver::updateEvent( ev::periodic& tm, int revents ) noexcept
{
	if( EV_ERROR & revents )
	{
		unetcrit << myname << "(updateEvent): EVENT ERROR.." << endl;
		return;
	}

	if( !activated )
		return;

	// взводим таймер опять..
	tm.again();

	// собственно обработка события
	try
	{
		update();
	}
	catch( std::exception& ex )
	{
		unetcrit << myname << "(updateEvent): " << ex.what() << std::endl;
	}

	// смотрим есть ли связь..
	checkConnection();

	if( sidRespond != DefaultObjectId )
	{
		try
		{
			if( isInitOK() )
			{
				bool r = respondInvert ? !isRecvOK() : isRecvOK();
				shm->localSetValue(itRespond, sidRespond, ( r ? 1 : 0 ), shm->ID());
			}
		}
		catch( const std::exception& ex )
		{
			unetcrit << myname << "(updateEvent): (respond) " << ex.what() << std::endl;
		}
	}

	if( sidLostPackets != DefaultObjectId )
	{
		try
		{
			shm->localSetValue(itLostPackets, sidLostPackets, getLostPacketsNum(), shm->ID());
		}
		catch( const std::exception& ex )
		{
			unetcrit << myname << "(updateEvent): (lostPackets) " << ex.what() << std::endl;
		}
	}
}
// -----------------------------------------------------------------------------
void UNetReceiver::checkConnectionEvent( ev::periodic& tm, int revents ) noexcept
{
	if( EV_ERROR & revents )
	{
		unetcrit << myname << "(checkConnectionEvent): EVENT ERROR.." << endl;
		return;
	}

	if( !activated )
		return;

	unetinfo << myname << "(checkConnectionEvent): check connection event.." << endl;

	std::lock_guard<std::mutex> l(checkConnMutex);

	if( !createConnection(false) )
		tm.again();
}
// -----------------------------------------------------------------------------
void UNetReceiver::stop()
{
	unetinfo << myname << ": stop.." << endl;
	activated = false;
	upThread->join();
	loop.evstop(this);
}
// -----------------------------------------------------------------------------
bool UNetReceiver::receive() noexcept
{
	try
	{
		ssize_t ret = udp->receiveBytes(r_buf.data, sizeof(r_buf.data));
		recvCount++;
		//ssize_t ret = udp->receiveFrom(r_buf.data, sizeof(r_buf.data),saddr);

		if( ret < 0 )
		{
			unetcrit << myname << "(receive): recv err(" << errno << "): " << strerror(errno) << endl;
			return false;
		}

		if( ret == 0 )
		{
			unetwarn << myname << "(receive): disconnected?!... recv 0 byte.." << endl;
			return false;
		}

		size_t sz = UniSetUDP::UDPMessage::getMessage(pack, r_buf);

		if( sz == 0 )
		{
			unetcrit << myname << "(receive): FAILED RECEIVE DATA ret=" << ret << endl;
			return false;
		}
	}
	catch( Poco::Net::NetException& ex )
	{
		unetcrit << myname << "(receive): recv err: " << ex.displayText() << endl;
		return false;
	}
	catch( exception& ex )
	{
		unetcrit << myname << "(receive): recv err: " << ex.what() << endl;
		return false;
	}

	if( pack.magic != UniSetUDP::UNETUDP_MAGICNUM )
	{
		// пакет не нашей "системы"
		return false;
	}

	if( rnum > 0 && labs( (long int)(pack.num - rnum) ) > maxDifferens )
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
		pack_guard l(packMutex, upStrategy);

		if( !waitClean )
		{
			qpack.push(pack);
			return true;
		}

		if( !qpack.empty() )
		{
			qtmp.push(pack);
		}
		else
		{
			// основная очередь освободилась..
			// копируем в неё всё что набралось в qtmp...
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
void UNetReceiver::initIterators() noexcept
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
void UNetReceiver::initDCache( UniSetUDP::UDPMessage& pack, bool force ) noexcept
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
			shm->initIterator(d.ioit);
		}
	}
}
// -----------------------------------------------------------------------------
void UNetReceiver::initACache( UniSetUDP::UDPMessage& pack, bool force ) noexcept
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
			shm->initIterator(d.ioit);
		}
	}
}
// -----------------------------------------------------------------------------
void UNetReceiver::connectEvent( UNetReceiver::EventSlot sl ) noexcept
{
	slEvent = sl;
}
// -----------------------------------------------------------------------------
UNetReceiver::UpdateStrategy UNetReceiver::strToUpdateStrategy( const string& s ) noexcept
{
	if( s == "thread" || s == "THREAD" )
		return useUpdateThread;

	if( s == "evloop" || s == "EVLOOP" )
		return useUpdateEventLoop;

	return useUpdateUnknown;
}
// -----------------------------------------------------------------------------
string UNetReceiver::to_string( UNetReceiver::UpdateStrategy s ) noexcept
{
	if( s == useUpdateThread )
		return "thread";

	if( s == useUpdateEventLoop )
		return "evloop";

	return "";
}
// -----------------------------------------------------------------------------
void UNetReceiver::setUpdateStrategy( UNetReceiver::UpdateStrategy set )
{
	if( set == useUpdateEventLoop && upThread->isRunning() )
	{
		ostringstream err;
		err << myname << "(setUpdateStrategy): set 'useUpdateEventLoop' strategy but updateThread is running!";
		unetcrit << err.str() << endl;
		throw SystemError(err.str());
	}

	if( set == useUpdateThread && evUpdate.is_active() )
	{
		ostringstream err;
		err << myname << "(setUpdateStrategy): set 'useUpdateThread' strategy but update event loop is running!";
		unetcrit << err.str() << endl;
		throw SystemError(err.str());
	}

	upStrategy = set;
}
// -----------------------------------------------------------------------------
const std::string UNetReceiver::getShortInfo() const noexcept
{
	// warning: будет вызываться из другого потока
	// (считаем что чтение безопасно)

	ostringstream s;

	s << setw(15) << std::right << getAddress() << ":" << std::left << setw(6) << getPort()
	  << "[ " << setw(7) << ( isLockUpdate() ? "PASSIVE" : "ACTIVE" ) << " ]"
	  << "    recvOK=" << isRecvOK()
	  << " receivepack=" << rnum
	  << " lostPackets=" << setw(6) << getLostPacketsNum()
	  << " updateStrategy=" << to_string(upStrategy)
	  << endl
	  << "\t["
	  << " recvTimeout=" << setw(6) << recvTimeout
	  << " prepareTime=" << setw(6) << prepareTime
	  << " evrunTimeout=" << setw(6) << evrunTimeout
	  << " lostTimeout=" << setw(6) << lostTimeout
	  << " recvpause=" << setw(6) << recvpause
	  << " updatepause=" << setw(6) << updatepause
	  << " maxDifferens=" << setw(6) << maxDifferens
	  << " maxProcessingCount=" << setw(6) << maxProcessingCount
	  << " waitClean=" << waitClean
	  << " ]"
	  << endl
	  << "\t[ qsize=" << qpack.size() << " recv=" << statRecvPerSec << " update=" << statUpPerSec << " per sec ]";

	return s.str();
}
// -----------------------------------------------------------------------------
UNetReceiver::pack_guard::pack_guard( mutex& _m, UNetReceiver::UpdateStrategy _s ):
	m(_m),
	s(_s)
{
	if( s == useUpdateThread )
		m.lock();
}
// -----------------------------------------------------------------------------
UNetReceiver::pack_guard::~pack_guard()
{
	if( s == useUpdateThread )
		m.unlock();
}
// -----------------------------------------------------------------------------
