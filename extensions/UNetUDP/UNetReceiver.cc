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
// -----------------------------------------------------------------------------
UNetReceiver::UNetReceiver(std::unique_ptr<UNetReceiveTransport>&& _transport
                           , const std::shared_ptr<SMInterface>& smi
                           , bool nocheckConnection
                           , const std::string& prefix ):
    shm(smi), transport(std::move(_transport)),
    cbuf(cbufSize)
{
    {
        ostringstream s;
        s << "R(" << transport->toString() << ")";
        myname = s.str();
    }

    addr = transport->toString();

    ostringstream logname;
    logname << prefix << "-R-" << transport->toString();

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
    if( sz > 0 )
    {
        cbufSize = sz;
        cbuf.resize(sz);
    }
}
// -----------------------------------------------------------------------------
void UNetReceiver::setMaxReceiveAtTime( size_t sz ) noexcept
{
    if( sz > 0 )
        maxReceiveCount = sz;
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
        // делаем неблокирующее чтение (нужно для libev)
        if( !transport->createConnection(throwEx, recvTimeout, true) )
            return false;

        evReceive.set<UNetReceiver, &UNetReceiver::callback>(this);
        evUpdate.set<UNetReceiver, &UNetReceiver::updateEvent>(this);

        if( evCheckConnection.is_active() )
            evCheckConnection.stop();

        ptRecvTimeout.setTiming(recvTimeout);
        ptPrepare.setTiming(prepareTime);
        evprepare(loop.evloop());
        return true;
    }
    catch( const std::exception& e )
    {
        ostringstream s;
        s << myname << "(createConnection): " << e.what();
        unetcrit << s.str() << std::endl;

        if( throwEx )
            throw SystemError(s.str());
    }
    catch( ... )
    {
        unetcrit << "(createConnection): catch ..." << std::endl;

        if( throwEx )
            throw;
    }

    return false;
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

    if( !transport->isConnected() )
    {
        evCheckConnection.set(eloop);
        evCheckConnection.start(0, checkConnectionTime);
        evInitPause.stop();
    }
    else
    {
        evReceive.set(eloop);
        evReceive.start(transport->getSocket(), ev::READ);
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

    transport->disconnect();
}
// -----------------------------------------------------------------------------
void UNetReceiver::forceUpdate() noexcept
{
    // сбрасываем запомненый номер последнего обработанного пакета
    // и тем самым заставляем обработать заново последний пакет и обновить данные в SM (см. update)
    rnum = wnum - 1;
}
// -----------------------------------------------------------------------------
void UNetReceiver::statisticsEvent(ev::periodic& tm, int revents) noexcept
{
    if( EV_ERROR & revents )
    {
        unetcrit << myname << "(statisticsEvent): EVENT ERROR.." << endl;
        return;
    }

    auto t_end = chrono::high_resolution_clock::now();
    float sec = chrono::duration_cast<chrono::duration<float>>(t_end - t_stats).count();
    t_stats = t_end;
    stats.recvPerSec = recvCount / sec;
    stats.upPerSec = upCount / sec;

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

        if( p->num() > num )
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

    // обрабатываем, пока очередь либо не опустеет,
    // либо обнаружится "дырка" в последовательности,
    while( rnum < wnum )
    {
        p = &(cbuf[rnum % cbufSize]);

        // если номер пакета не равен ожидаемому, ждём считая что это "дырка"
        // т.к. разрывы и другие случаи обрабатываются при приёме пакетов
        if( p->num() != rnum )
        {
            if( !ptLostTimeout.checkTime() )
                return;

            size_t sub = 1;

            if( p->num() > rnum )
                sub = (p->num() - rnum);

            unetwarn << myname << "(update): lostTimeout(" << ptLostTimeout.getInterval() << ")! pnum="
                     << p->num() << " lost "
                     << sub << " packets "
                     << endl;

            lostPackets += sub;

            // ищем следующий пакет для обработки
            rnum = rnext(rnum);
            continue;
        }

        ptLostTimeout.reset();
        rnum++;
        upCount++;

        // обновление данных в SM (блокировано)
        if( lockUpdate )
            continue;

        // Обработка дискретных
        auto d_iv = getDCache(p);

        for( size_t i = 0; i < p->dsize(); i++ )
        {
            try
            {
                c_it = &(*d_iv)[i];

                if( c_it->id != p->dID(i) )
                {
                    unetwarn << myname << "(update): reinit dcache for sid=" << p->dID(i) << endl;
                    c_it->id = p->dID(i);
                    shm->initIterator(c_it->ioit);
                }

                shm->localSetValue(c_it->ioit, p->dID(i), p->dValue(i), shm->ID());
            }
            catch( const uniset::Exception& ex)
            {
                unetcrit << myname << "(update): D:"
                         << " id=" << p->dID(i)
                         << " val=" << p->dValue(i)
                         << " error: " << ex
                         << std::endl;
            }
            catch(...)
            {
                unetcrit << myname << "(update): D:"
                         << " id=" << p->dID(i)
                         << " val=" << p->dValue(i)
                         << " error: catch..."
                         << std::endl;
            }
        }

        // Обработка аналоговых
        auto a_iv = getACache(p);

        for( size_t i = 0; i < p->asize(); i++ )
        {
            try
            {
                c_it = &(*a_iv)[i];

                if( c_it->id != p->aID(i) )
                {
                    unetwarn << myname << "(update): reinit acache for sid=" << p->aID(i) << endl;
                    c_it->id = p->aID(i);
                    shm->initIterator(c_it->ioit);
                }

                shm->localSetValue(c_it->ioit, p->aID(i), p->aValue(i), shm->ID());
            }
            catch( const uniset::Exception& ex)
            {
                unetcrit << myname << "(update): A:"
                         << " id=" << p->aID(i)
                         << " val=" << p->aValue(i)
                         << " error: " << ex
                         << std::endl;
            }
            catch(...)
            {
                unetcrit << myname << "(update): A:"
                         << " id=" << p->aID(i)
                         << " val=" << p->aValue(i)
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

    bool ok = false;
    t_start = chrono::high_resolution_clock::now();

    try
    {
        for( size_t i = 0; transport->available() > 0 && i < maxReceiveCount; i++ )
        {
            if( receive() != retOK )
                break;

            ok = true;
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

    if( ok )
    {
        std::lock_guard<std::mutex> l(tmMutex);
        ptRecvTimeout.reset();
    }

    t_end = chrono::high_resolution_clock::now();
    stats.recvProcessingTime_microsec = std::chrono::duration_cast<std::chrono::microseconds>(t_end - t_start).count();
}
// -----------------------------------------------------------------------------
bool UNetReceiver::checkConnection()
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
            try
            {
                if (tout)
                    slEvent(w, evTimeout);
                else
                    slEvent(w, evOK);
            }
            catch( std::exception& ex )
            {
                unetcrit << myname << "(checkConnection): exception: " << ex.what() << endl;
            }
        }
    }

    return !tout;
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

    // смотрим есть ли связь..
    bool recvOk = checkConnection();

    // обновление данных в SM
    t_start = chrono::high_resolution_clock::now();

    try
    {
        update();
    }
    catch( std::exception& ex )
    {
        unetcrit << myname << "(updateEvent): " << ex.what() << std::endl;
    }

    t_end = chrono::high_resolution_clock::now();
    stats.upProcessingTime_microsec = std::chrono::duration_cast<std::chrono::microseconds>(t_end - t_start).count();

    if( sidRespond != DefaultObjectId )
    {
        try
        {
            if( isInitOK() )
            {
                bool r = respondInvert ? !recvOk : recvOk;
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
UNetReceiver::ReceiveRetCode UNetReceiver::receive() noexcept
{
    try
    {
        // сперва пробуем сохранить пакет в том месте, где должен быть очередной пакет
        pack = &(cbuf[wnum % cbufSize]);
        ssize_t ret = transport->receive(rbuf, sizeof(rbuf));

        if( ret < 0 )
        {
            unetcrit << myname << "(receive): recv err(" << errno << "): " << strerror(errno) << endl;
            return retError;
        }

        if( ret == 0 )
        {
            unetwarn << myname << "(receive): disconnected?!... recv 0 bytes.." << endl;
            return retNoData;
        }

        if( !pack->initFromBuffer(rbuf, ret) )
        {
            unetwarn << myname << "(receive): parse message error.." << endl;
            return retError;
        }

        recvCount++;

        if( !pack->isOk() )
            return retError;

        if( size_t(abs(long(pack->num() - wnum))) > maxDifferens || size_t(abs( long(wnum - rnum) )) >= (cbufSize - 2) )
        {
            unetcrit << myname << "(receive): DISAGREE "
                     << " packnum=" << pack->num()
                     << " wnum=" << wnum
                     << " rnum=" << rnum
                     << " (maxDiff=" << maxDifferens
                     << " indexDiff=" << abs( long(wnum - rnum) )
                     << ")"
                     << endl;

            lostPackets = pack->num() > wnum ? (pack->num() - wnum - 1) : lostPackets + 1;
            // реинициализируем позицию для чтения
            rnum = pack->num();
            wnum = pack->num() + 1;

            // перемещаем пакет в нужное место (если требуется)
            if( wnum != pack->num() )
            {
                cbuf[pack->num() % cbufSize] = (*pack);
                pack->setNum(0);
            }

            return retOK;
        }

        if( pack->num() != wnum )
        {
            // перемещаем пакет в правильное место
            // в соответствии с его номером
            cbuf[pack->num() % cbufSize] = (*pack);

            if( pack->num() >= wnum )
                wnum = pack->num() + 1;

            // обнуляем номер в том месте где записали, чтобы его не обрабатывал update
            pack->setNum(0);
        }
        else
            wnum++;

        // начальная инициализация для чтения
        if( rnum == 0 )
            rnum = pack->num();

        return retOK;
    }
    catch( Poco::Net::NetException& ex )
    {
        unetcrit << myname << "(receive): recv err: " << ex.displayText() << endl;
    }
    catch( exception& ex )
    {
        unetcrit << myname << "(receive): recv err: " << ex.what() << endl;
    }

    return retError;
}
// -----------------------------------------------------------------------------
void UNetReceiver::initIterators() noexcept
{
    for( auto mit = d_icache_map.begin(); mit != d_icache_map.end(); ++mit )
    {
        CacheVec& d_icache = mit->second;

        for( auto&& it : d_icache )
            shm->initIterator(it.ioit);
    }

    for( auto mit = a_icache_map.begin(); mit != a_icache_map.end(); ++mit )
    {
        CacheVec& a_icache = mit->second;

        for( auto&& it : a_icache )
            shm->initIterator(it.ioit);
    }
}
// -----------------------------------------------------------------------------
UNetReceiver::CacheVec* UNetReceiver::getDCache( UniSetUDP::UDPMessage* pack ) noexcept
{
    auto dID = pack->getDataID();
    auto dit = d_icache_map.find(dID);

    if( dit == d_icache_map.end() )
    {
        auto p = d_icache_map.emplace(dID, UNetReceiver::CacheVec());
        dit = p.first;
    }

    CacheVec* d_info = &dit->second;

    if( pack->dsize() == d_info->size() )
        return d_info;

    unetinfo << myname << ": init dcache[" << pack->dsize() << "] for " << dID << endl;

    d_info->resize(pack->dsize());

    for( size_t i = 0; i < pack->dsize(); i++ )
    {
        CacheItem& d = (*d_info)[i];

        if( d.id != pack->dID(i) )
        {
            d.id = pack->dID(i);
            shm->initIterator(d.ioit);
        }
    }

    return d_info;
}
// -----------------------------------------------------------------------------
UNetReceiver::CacheVec* UNetReceiver::getACache( UniSetUDP::UDPMessage* pack ) noexcept
{
    auto dID = pack->getDataID();
    auto ait = a_icache_map.find(dID);

    if( ait == a_icache_map.end() )
    {
        auto p = a_icache_map.emplace(dID, UNetReceiver::CacheVec());
        ait = p.first;
    }

    CacheVec* a_info = &ait->second;

    if( pack->asize() == a_info->size() )
        return a_info;

    unetinfo << myname << ": init acache[" << pack->asize() << "] for " << dID << endl;

    a_info->resize(pack->asize());

    for( size_t i = 0; i < pack->asize(); i++ )
    {
        CacheItem& d = (*a_info)[i];

        if( d.id != pack->aID(i) )
        {
            d.id = pack->aID(i);
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

    s << setw(15) << std::right << transport->toString()
      << "[ " << setw(7) << ( isLockUpdate() ? "PASSIVE" : "ACTIVE" ) << " ]"
      << "    recvOK=" << isRecvOK()
      << " receivepack=" << rnum
      << " lostPackets=" << setw(6) << getLostPacketsNum()
      << endl
      << "\t["
      << " recvTimeout=" << recvTimeout
      << " prepareTime=" << prepareTime
      << " evrunTimeout=" << evrunTimeout
      << " lostTimeout=" << lostTimeout
      << " updatepause=" << updatepause
      << " maxDifferens=" << maxDifferens
      << " ]"
      << endl
      << "\t[ qsize:" << setw(3) << (wnum - rnum)
      << " recv:" << setprecision(3) << setw(6) << stats.recvPerSec << " msg/sec"
      << " update:" << setprecision(3) << setw(6) << stats.upPerSec << " msg/sec"
      << " upTime:" << setw(6) << stats.upProcessingTime_microsec << " usec"
      << " recvTime:" << setw(6) << stats.recvProcessingTime_microsec << " usec"
      << " ]";

    return s.str();
}
// -----------------------------------------------------------------------------
