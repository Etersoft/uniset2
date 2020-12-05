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
#include <cmath>
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
static UniSetUDP::UDPMessage emptyMessage;
// -----------------------------------------------------------------------------
UNetReceiver::UNetReceiver(const std::string& s_host, int _port
						   , const std::shared_ptr<SMInterface>& smi
						   , bool nocheckConnection
						   , const std::string& prefix ):
	shm(smi),
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
	cbuf(cbufSize),
	maxDifferens(20),
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

	if( !createConnection(nocheckConnection /* <-- это флаг throwEx */) )
		evCheckConnection.set<UNetReceiver, &UNetReceiver::checkConnectionEvent>(this);

	evStatistic.set<UNetReceiver, &UNetReceiver::statisticsEvent>(this);
	evUpdate.set<UNetReceiver, &UNetReceiver::updateEvent>(this);
	evInitPause.set<UNetReceiver, &UNetReceiver::initEvent>(this);

	ptLostTimeout.setTiming(lostTimeout);
	ptRecvTimeout.setTiming(recvTimeout);
}
// -----------------------------------------------------------------------------
UNetReceiver::~UNetReceiver()
{
}
// -----------------------------------------------------------------------------
void UNetReceiver::setBufferSize( size_t sz ) noexcept
{
	cbufSize = sz;
	cbuf.resize(sz);
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
void UNetReceiver::setUpdatePause( timeout_t msec ) noexcept
{
	updatepause = msec;

	if( evUpdate.is_active() )
		evUpdate.start(0, (float)updatepause / 1000.);
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
	evUpdate.set(eloop);
	evUpdate.start( 0, ((float)updatepause / 1000.) );

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
	rnum = 0; // сбрасываем запомненый номер последнего обработанного пакета
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
size_t UNetReceiver::rnext( size_t num )
{
	UniSetUDP::UDPMessage* p;
	size_t i = num + 1;

	while( i < wnum )
	{
		p = &cbuf[i % cbufSize];

		if( p->num > num )
			return i;

		i++;
	}

	return wnum;
}
// -----------------------------------------------------------------------------
void UNetReceiver::update() noexcept
{
	// ещё не было пакетов
	if( wnum == 1 && rnum == 0 )
		return;

	UniSetUDP::UDPMessage* p;
	CacheItem* c_it = nullptr;
	UniSetUDP::UDPAData* dat = nullptr;
	long s_id;

	// обрабатываем, пока очередь либо не опустеет,
	// либо обнаружится "дырка" в последовательности,
	while( rnum < wnum )
	{
		p = &cbuf[rnum % cbufSize];

		// если номер пакета не равен ожидаемому, ждём считая что это "дырка"
		// т.к. разрывы и другие случаи обрабатываются при приёме пакетов
		if( p->num != rnum )
		{
			if( !ptLostTimeout.checkTime() )
				return;

			size_t sub = (p->num - rnum);
			unetwarn << myname << "(update): lostTimeout(" << ptLostTimeout.getInterval() << ")! pnum=" << p->num << " lost " << sub << " packets " << endl;
			lostPackets += sub;

			// ищем следующий пакет для обработки
			rnum = rnext(rnum);
			continue;
		}

		ptLostTimeout.reset();
		rnum++;
		upCount++;

		// Обработка дискретных
		auto d_iv = getDCache(p, !d_cache_init_ok);

		for( size_t i = 0; i < p->dcount; i++ )
		{
			try
			{
				s_id = p->dID(i);
				c_it = &d_iv.cache[i];

				if( c_it->id != s_id )
				{
					unetwarn << myname << "(update): reinit dcache for sid=" << s_id << endl;
					c_it->id = s_id;
					shm->initIterator(c_it->ioit);
				}

				// обновление данных в SM (блокировано)
				if( lockUpdate )
					continue;

				shm->localSetValue(c_it->ioit, s_id, p->dValue(i), shm->ID());
			}
			catch( const uniset::Exception& ex)
			{
				unetcrit << myname << "(update): "
						 << " id=" << s_id
						 << " val=" << p->dValue(i)
						 << " error: " << ex
						 << std::endl;
			}
			catch(...)
			{
				unetcrit << myname << "(update): "
						 << " id=" << s_id
						 << " val=" << p->dValue(i)
						 << " error: catch..."
						 << std::endl;
			}
		}

		// Обработка аналоговых
		auto a_iv = getACache(p, !a_cache_init_ok);

		for( size_t i = 0; i < p->acount; i++ )
		{
			try
			{
				dat = &p->a_dat[i];
				c_it = &a_iv.cache[i];

				if( c_it->id != dat->id )
				{
					unetwarn << myname << "(update): reinit acache for sid=" << dat->id << endl;
					c_it->id = dat->id;
					shm->initIterator(c_it->ioit);
				}

				// обновление данных в SM (блокировано)
				if( lockUpdate )
					continue;

				shm->localSetValue(c_it->ioit, dat->id, dat->val, shm->ID());
			}
			catch( const uniset::Exception& ex)
			{
				unetcrit << myname << "(update): "
						 << " id=" << dat->id
						 << " val=" << dat->val
						 << " error: " << ex
						 << std::endl;
			}
			catch(...)
			{
				unetcrit << myname << "(update): "
						 << " id=" << dat->id
						 << " val=" << dat->val
						 << " error: catch..."
						 << std::endl;
			}
		}
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
	loop.evstop(this);
}
// -----------------------------------------------------------------------------
bool UNetReceiver::receive() noexcept
{
	try
	{
		ssize_t ret = udp->receiveBytes(r_buf.data, sizeof(r_buf.data));
		recvCount++;

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

		// сперва пробуем сохранить пакет в том месте, которе должно быть очередным на запись
		pack = &cbuf[wnum % cbufSize];
		size_t sz = UniSetUDP::UDPMessage::getMessage(*pack, r_buf);

		if( sz == 0 )
		{
			unetcrit << myname << "(receive): FAILED RECEIVE DATA ret=" << ret << endl;
			return false;
		}

		if( pack->magic != UniSetUDP::UNETUDP_MAGICNUM )
			return false;

		if( abs(long(pack->num - wnum)) > maxDifferens || abs( long(wnum - rnum) ) >= (cbufSize - 2) )
		{
			unetcrit << myname << "(receive): DISAGREE "
					 << " packnum=" << pack->num
					 << " wnum=" << wnum
					 << " rnum=" << rnum
					 << " (maxDiff=" << maxDifferens
					 << " indexDiff=" << abs( long(wnum - rnum) )
					 << ")"
					 << endl;

			lostPackets = pack->num > wnum ? (pack->num - wnum - 1) : lostPackets + 1;
			// реинициализируем позицию для чтения
			rnum = pack->num;
			wnum = pack->num + 1;

			// перемещаем пакет в нужное место (если требуется)
			if( wnum != pack->num )
			{
				cbuf[pack->num % cbufSize] = (*pack);
				pack->num = 0;
			}

			return true;
		}

		if( pack->num != wnum )
		{
			// перемещаем пакет в правильное место
			// в соответствии с его номером
			cbuf[pack->num % cbufSize] = (*pack);

			if( pack->num >= wnum )
				wnum = pack->num + 1;

			// обнуляем номер в том месте где записали, чтобы его не обрабатывал update
			pack->num = 0;
		}
		else if( pack->num >= wnum )
			wnum = pack->num + 1;

		// начальная инициализация для чтения
		if( rnum == 0 )
			rnum = pack->num;

		return true;
	}
	catch( Poco::Net::NetException& ex )
	{
		unetcrit << myname << "(receive): recv err: " << ex.displayText() << endl;
	}
	catch( exception& ex )
	{
		unetcrit << myname << "(receive): recv err: " << ex.what() << endl;
	}

	return false;
}
// -----------------------------------------------------------------------------
void UNetReceiver::initIterators() noexcept
{
	for( auto mit = d_icache_map.begin(); mit != d_icache_map.end(); ++mit )
	{
		CacheVec& d_icache(mit->second.cache);

		for( auto&& it : d_icache )
			shm->initIterator(it.ioit);
	}

	for( auto mit = a_icache_map.begin(); mit != a_icache_map.end(); ++mit )
	{
		CacheVec& a_icache(mit->second.cache);

		for( auto&& it : a_icache )
			shm->initIterator(it.ioit);
	}
}
// -----------------------------------------------------------------------------
UNetReceiver::CacheInfo& UNetReceiver::getDCache( UniSetUDP::UDPMessage* pack, bool force ) noexcept
{
	// если элемента нет, он будет создан
	CacheInfo& d_info = d_icache_map[pack->getDataID()];

	if( !force && pack->dcount == d_info.cache.size() )
		return d_info;

	if( d_info.cache_init_ok && pack->dcount == d_info.cache.size() )
	{
		d_cache_init_ok = true;
		auto it = d_icache_map.begin();

		for( ; it != d_icache_map.end(); ++it )
		{
			d_info = it->second;
			d_cache_init_ok = d_cache_init_ok && d_info.cache_init_ok;

			if(d_cache_init_ok == false)
				break;
		}

		return d_info;
	}

	unetinfo << myname << ": init dcache for " << pack->getDataID() << endl;

	d_info.cache_init_ok = true;
	d_info.cache.resize(pack->dcount);

	size_t sz = d_info.cache.size();
	auto conf = uniset_conf();

	for( size_t i = 0; i < sz; i++ )
	{
		CacheItem& d(d_info.cache[i]);

		if( d.id != pack->d_id[i] )
		{
			d.id = pack->d_id[i];
			shm->initIterator(d.ioit);
		}
	}

	return d_info;
}
// -----------------------------------------------------------------------------
UNetReceiver::CacheInfo& UNetReceiver::getACache( UniSetUDP::UDPMessage* pack, bool force ) noexcept
{
	// если элемента нет, он будет создан
	CacheInfo& a_info = a_icache_map[pack->getDataID()];

	if( !force && pack->acount == a_info.cache.size() )
		return a_info;

	if( a_info.cache_init_ok && pack->acount == a_info.cache.size() )
	{
		a_cache_init_ok = true;
		auto it = a_icache_map.begin();

		for( ; it != a_icache_map.end(); ++it )
		{
			a_info = it->second;
			a_cache_init_ok = a_cache_init_ok && a_info.cache_init_ok;

			if(a_cache_init_ok == false)
				break;
		}

		return a_info;
	}

	unetinfo << myname << ": init icache for " << pack->getDataID() << endl;
	a_info.cache_init_ok = true;
	auto conf = uniset_conf();

	a_info.cache.resize(pack->acount);

	size_t sz = a_info.cache.size();

	for( size_t i = 0; i < sz; i++ )
	{
		CacheItem& d(a_info.cache[i]);

		if( d.id != pack->a_dat[i].id )
		{
			d.id = pack->a_dat[i].id;
			shm->initIterator(d.ioit);
		}
	}

	return a_info;
}
// -----------------------------------------------------------------------------
void UNetReceiver::connectEvent( UNetReceiver::EventSlot sl ) noexcept
{
	slEvent = sl;
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
	  << endl
	  << "\t["
	  << " recvTimeout=" << setw(6) << recvTimeout
	  << " prepareTime=" << setw(6) << prepareTime
	  << " evrunTimeout=" << setw(6) << evrunTimeout
	  << " lostTimeout=" << setw(6) << lostTimeout
	  << " updatepause=" << setw(6) << updatepause
	  << " maxDifferens=" << setw(6) << maxDifferens
	  << " ]"
	  << endl
	  << "\t[ qsize=" << (wnum - rnum - 1) << " recv=" << statRecvPerSec << " update=" << statUpPerSec << " per sec ]";

	return s.str();
}
// -----------------------------------------------------------------------------
