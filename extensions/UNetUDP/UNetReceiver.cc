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
using namespace uniset::extensions;
// -----------------------------------------------------------------------------
namespace uniset
{
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
        evForceUpdate.set<UNetReceiver, &UNetReceiver::onForceUpdate>(this);

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
    void UNetReceiver::setIgnoreCRC( bool set ) noexcept
    {
        ignoreCRC = set;
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
    void UNetReceiver::setModeID( uniset::ObjectId id ) noexcept
    {
        sidMode = id;
        shm->initIterator(itMode);
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
            evForceUpdate.set<UNetReceiver, &UNetReceiver::onForceUpdate>(this);

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
        evForceUpdate.set(eloop);
        evForceUpdate.start();

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

        if( evForceUpdate.is_active() )
            evForceUpdate.stop();

        transport->disconnect();
    }
    // -----------------------------------------------------------------------------
    void UNetReceiver::forceUpdate() noexcept
    {
        if( evForceUpdate.is_active() )
            evForceUpdate.send();
    }
    // -----------------------------------------------------------------------------
    void UNetReceiver::statisticsEvent(ev::periodic& tm, int revents) noexcept
    {
        if( EV_ERROR & revents )
        {
            unetcrit << myname << "(statisticsEvent): EVENT ERROR.." << endl;
            return;
        }

        t_end = chrono::steady_clock::now();
        float sec = chrono::duration_cast<chrono::duration<float>>(t_end - t_stats).count();
        t_stats = t_end;
        stats.recvPerSec = recvCount / sec;
        stats.upPerSec = upCount / sec;

        recvCount = 0;
        upCount = 0;
        tm.again();
    }
    // -----------------------------------------------------------------------------
    void UNetReceiver::onForceUpdate( ev::async& watcher, int revents ) noexcept
    {
        if( EV_ERROR & revents )
        {
            unetcrit << myname << "(onForceUpdate): EVENT ERROR.." << endl;
            return;
        }

        // ещё не было пакетов
        if( wnum == 1 && rnum == 0 )
            return;

        // сбрасываем кэш
        for( auto&& c : d_icache_map )
            c.second.crc = 0;

        for( auto&& c : a_icache_map )
            c.second.crc = 0;

        // сбрасываем запомненый номер последнего обработанного пакета
        // и тем самым заставляем обработать заново последний пакет и обновить данные в SM (см. update)
        if( rnum > 0 )
            rnum--;

        update();
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

            if( p->header.num > num )
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
            p = &(cbuf[rnum % cbufSize]);

            // если номер пакета не равен ожидаемому, ждём считая что это "дырка"
            // т.к. разрывы и другие случаи обрабатываются при приёме пакетов
            if( p->header.num != rnum )
            {
                if( !ptLostTimeout.checkTime() )
                    return;

                size_t sub = 1;

                if( p->header.num > rnum )
                    sub = (p->header.num - rnum);

                unetwarn << myname << "(update): lostTimeout(" << ptLostTimeout.getInterval() << ")! pnum=" << p->header.num << " lost " << sub << " packets " << endl;
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

            if( mode == Mode::mDisabled )
                continue;

            // Обработка дискретных
            auto dcache = getDCache(p);

            if( p->header.dcrc == 0 || dcache->crc != p->header.dcrc || ignoreCRC )
            {
                dcache->crc = p->header.dcrc;

                for( size_t i = 0; i < p->header.dcount; i++ )
                {
                    try
                    {
                        s_id = p->dID(i);
                        c_it = &(dcache->items[i]);

                        if (c_it->id != s_id)
                        {
                            unetwarn << myname << "(update): reinit dcache for sid=" << s_id << endl;
                            c_it->id = s_id;
                            shm->initIterator(c_it->ioit);
                        }

                        shm->localSetValue(c_it->ioit, s_id, p->dValue(i), shm->ID());
                    }
                    catch (const uniset::Exception& ex)
                    {
                        // сбрасываем crc, т.к. данные не удалось успешно обновить
                        dcache->crc = 0;
                        unetcrit << myname << "(update): D:"
                                 << " id=" << s_id
                                 << " val=" << p->dValue(i)
                                 << " error: " << ex
                                 << std::endl;
                    }
                    catch (...)
                    {
                        // сбрасываем crc, т.к. данные не удалось успешно обновить
                        dcache->crc = 0;
                        unetcrit << myname << "(update): D:"
                                 << " id=" << s_id
                                 << " val=" << p->dValue(i)
                                 << " error: catch..."
                                 << std::endl;
                    }
                }
            }

            // Обработка аналоговых
            auto acache = getACache(p);

            if( p->header.acrc == 0 || acache->crc != p->header.acrc || ignoreCRC )
            {
                acache->crc = p->header.acrc;

                for( size_t i = 0; i < p->header.acount; i++ )
                {
                    try
                    {
                        dat = &p->a_dat[i];
                        c_it = &(acache->items)[i];

                        if (c_it->id != dat->id)
                        {
                            unetwarn << myname << "(update): reinit acache for sid=" << dat->id << endl;
                            c_it->id = dat->id;
                            shm->initIterator(c_it->ioit);
                        }

                        shm->localSetValue(c_it->ioit, dat->id, dat->val, shm->ID());
                    }
                    catch (const uniset::Exception& ex)
                    {
                        acache->crc = 0;
                        unetcrit << myname << "(update): A:"
                                 << " id=" << dat->id
                                 << " val=" << dat->val
                                 << " error: " << ex
                                 << std::endl;
                    }
                    catch (...)
                    {
                        acache->crc = 0;
                        unetcrit << myname << "(update): A:"
                                 << " id=" << dat->id
                                 << " val=" << dat->val
                                 << " error: catch..."
                                 << std::endl;
                    }
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
        t_start = chrono::steady_clock::now();

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

        t_end = chrono::steady_clock::now();
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
        t_start = chrono::steady_clock::now();

        try
        {
            update();
        }
        catch( std::exception& ex )
        {
            unetcrit << myname << "(updateEvent): " << ex.what() << std::endl;
        }

        t_end = chrono::steady_clock::now();
        stats.upProcessingTime_microsec = std::chrono::duration_cast<std::chrono::microseconds>(t_end - t_start).count();

        if( sidMode != DefaultObjectId )
        {
            try
            {
                mode = (Mode)shm->localGetValue(itMode, sidMode);
            }
            catch( const std::exception& ex )
            {
                unetcrit << myname << "(updateEvent): mode update error: " << ex.what() << std::endl;
            }

            // сброс при включении режима mEnabled
            if( trOnMode.hi( mode == Mode::mEnabled ) )
                forceUpdate();
        }

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
            ssize_t ret = transport->receive(pack, sizeof(UniSetUDP::UDPMessage));

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

            recvCount++;

            // конвертируем byte order
            pack->ntoh();

            if( !pack->isOk() )
                return retError;

            if( size_t(abs(long(pack->header.num - wnum))) > maxDifferens || size_t(abs( long(wnum - rnum) )) >= (cbufSize - 2) )
            {
                unetcrit << myname << "(receive): DISAGREE "
                         << " packnum=" << pack->header.num
                         << " wnum=" << wnum
                         << " rnum=" << rnum
                         << " (maxDiff=" << maxDifferens
                         << " indexDiff=" << abs( long(wnum - rnum) )
                         << ")"
                         << endl;

                lostPackets += pack->header.num > wnum ? (pack->header.num - wnum - 1) : 1;
                // реинициализируем позицию для чтения
                rnum = pack->header.num;
                wnum = pack->header.num + 1;

                // перемещаем пакет в нужное место (если требуется)
                if( wnum != pack->header.num )
                {
                    cbuf[pack->header.num % cbufSize] = (*pack);
                    pack->header.num = 0;
                }

                return retOK;
            }

            if( pack->header.num != wnum )
            {
                // перемещаем пакет в правильное место
                // в соответствии с его номером
                cbuf[pack->header.num % cbufSize] = (*pack);

                if( pack->header.num >= wnum )
                    wnum = pack->header.num + 1;

                // обнуляем номер в том месте где записали, чтобы его не обрабатывал update
                pack->header.num = 0;
            }
            else
                wnum++;

            // начальная инициализация для чтения
            if( rnum == 0 )
                rnum = pack->header.num;

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
            CacheInfo& d_icache = mit->second;

            for( auto&& it : d_icache.items )
                shm->initIterator(it.ioit);
        }

        for( auto mit = a_icache_map.begin(); mit != a_icache_map.end(); ++mit )
        {
            CacheInfo& a_icache = mit->second;

            for( auto&& it : a_icache.items )
                shm->initIterator(it.ioit);
        }
    }
    // -----------------------------------------------------------------------------
    UNetReceiver::CacheInfo* UNetReceiver::getDCache( UniSetUDP::UDPMessage* upack ) noexcept
    {
        auto dID = upack->getDataID();
        auto dit = d_icache_map.find(dID);

        if( dit == d_icache_map.end() )
        {
            auto p = d_icache_map.emplace(dID, UNetReceiver::CacheInfo());
            dit = p.first;
        }

        CacheInfo* d_info = &dit->second;

        if(upack->header.dcount == d_info->items.size() )
            return &dit->second;

        unetinfo << myname << ": init dcache[" << upack->header.dcount << "] for dataID=" << dID << endl;

        d_info->items.resize(upack->header.dcount);
        d_info->crc = 0;
        cacheMissed++;

        for(size_t i = 0; i < upack->header.dcount; i++ )
        {
            CacheItem& d = d_info->items[i];

            if(d.id != upack->d_id[i] )
            {
                d.id = upack->d_id[i];
                shm->initIterator(d.ioit);
            }
        }

        return d_info;
    }
    // -----------------------------------------------------------------------------
    UNetReceiver::CacheInfo* UNetReceiver::getACache( UniSetUDP::UDPMessage* upack ) noexcept
    {
        auto dID = upack->getDataID();
        auto ait = a_icache_map.find(dID);

        if( ait == a_icache_map.end() )
        {
            auto p = a_icache_map.emplace(dID, UNetReceiver::CacheInfo());
            ait = p.first;
        }

        CacheInfo* a_info = &ait->second;

        if( upack->header.acount == a_info->items.size() )
            return a_info;

        unetinfo << myname << ": init acache[" << upack->header.acount << "] for dataID=" << dID << endl;

        a_info->items.resize(upack->header.acount);
        a_info->crc = 0;
        cacheMissed++;

        for( size_t i = 0; i < upack->header.acount; i++ )
        {
            CacheItem& d = a_info->items[i];

            if( d.id != upack->a_dat[i].id )
            {
                d.id = upack->a_dat[i].id;
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
    std::string UNetReceiver::getShortInfo() const noexcept
    {
        // warning: будет вызываться из другого потока
        // (считаем что чтение безопасно)

        ostringstream s;

        std::string smode = isLockUpdate() ? "PASSIVE" : "ACTIVE";

        if( mode != Mode::mEnabled )
            smode = to_string(mode);

        s << setw(15) << std::right << transport->toString()
          << "[ " << setw(7) << smode << " ]"
          << "    recvOK=" << isRecvOK()
          << " receivepack=" << rnum
          << " lostPackets=" << setw(6) << getLostPacketsNum()
          << " cacheMissed=" << setw(6) << cacheMissed
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
#ifndef DISABLE_REST_API
    Poco::JSON::Object::Ptr UNetReceiver::httpInfo( Poco::JSON::Object::Ptr root ) const
    {
        Poco::JSON::Object::Ptr json = root;

        if( !json )
            json = new Poco::JSON::Object();

        std::string smode = isLockUpdate() ? "PASSIVE" : "ACTIVE";

        if( mode != Mode::mEnabled )
            smode = to_string(mode);

        json->set("transport", transport->toString());
        json->set("mode", smode);
        json->set("recvOK", isRecvOK());
        json->set("receivepack", (int)rnum);
        json->set("lostPackets", (int)getLostPacketsNum());
        json->set("cacheMissed", (int)cacheMissed);

        // params
        Poco::JSON::Object::Ptr params = new Poco::JSON::Object();
        params->set("recvTimeout", (int)recvTimeout);
        params->set("prepareTime", (int)prepareTime);
        params->set("evrunTimeout", (int)evrunTimeout);
        params->set("lostTimeout", (int)lostTimeout);
        params->set("updatepause", (int)updatepause);
        params->set("maxDifferens", (int)maxDifferens);
        json->set("params", params);

        // stats
        Poco::JSON::Object::Ptr st = new Poco::JSON::Object();
        st->set("qsize", (int)(wnum - rnum));
        st->set("recvPerSec", stats.recvPerSec);
        st->set("upPerSec", stats.upPerSec);
        st->set("upProcessingTime_usec", (int)stats.upProcessingTime_microsec);
        st->set("recvProcessingTime_usec", (int)stats.recvProcessingTime_microsec);
        json->set("stats", st);

        return json;
    }
#endif
    // -----------------------------------------------------------------------------
} // end of namespace uniset
// -----------------------------------------------------------------------------
namespace std
{
    std::string to_string( const uniset::UNetReceiver::Mode& m )
    {
        if( m == uniset::UNetReceiver::Mode::mEnabled )
            return "Enabled";

        if( m == uniset::UNetReceiver::Mode::mDisabled )
            return "Disabled";

        return "";
    }
}
// -----------------------------------------------------------------------------