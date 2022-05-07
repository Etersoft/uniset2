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
// -----------------------------------------------------------------------------
#ifndef UNetReceiver_H_
#define UNetReceiver_H_
// -----------------------------------------------------------------------------
#include <ostream>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <sigc++/sigc++.h>
#include <ev++.h>
#include "UniSetObject.h"
#include "Trigger.h"
#include "Mutex.h"
#include "SMInterface.h"
#include "SharedMemory.h"
#include "UDPPacket.h"
#include "CommonEventLoop.h"
#include "UNetTransport.h"
// --------------------------------------------------------------------------
namespace uniset
{
    // -----------------------------------------------------------------------------
    /*  Основная идея: сделать проверку очерёдности пакетов, но при этом использовать UDP.
     * ===============
     * В данных передаётся номер пакета. На случай если несколько пакетов придут не в той последовательности
     * что были посланы, сделан циклический буфер (буфер сразу выделяет память при старте).
     * Т.к. номер получаемых пакетов монотонно растёт на основе него вычисляется индекс
     * куда поместить пакет в буфере. Есть два индекса
     * rnum - (read number) номер последнего обработанного пакета + 1
     * wnum - (write number) номер следующего ожидаемого пакета (номер последнего принятого + 1)
     * WARNING: Если придёт два пакета с одинаковым номером, то новый пакет перезатрёт прошлый в буфере
     *
     * При этом обработка ведётся по порядку (только пакеты идущие подряд)
     * как только встречается "дырка" происходит ожидание её "заполения". Если в течение времени (lostTimeout)
     * "дырка" не исчезает, увеличивается счётчик потерянных пакетов и обработка продолжается с нового места.
     * Т.к. используется libev и нет многопоточной работы, события обрабатываются последовательно.
     * Раз в updatetime msec происходит обновление данных в SM, все накопившиеся пакеты обрабатываются
     * либо пока не встретиться "дырка", либо пока rnum не догонит wnum.
     *
     * КЭШ
     * ===
     * Для оптимизации работы с SM, т.к. в пакетах приходят только пары [id,value] сделан кэш итераторов.
     * Идея проста: сделан вектор размером с количество принимаемых данных. В векторе хранятся итераторы (и всё что необходимо).
     * Порядковый номер данных в пакете является индексом в кэше.
     * Для защиты от изменения последовательности внутри пакета, в кэше хранится ID сохраняемого датчика, и если он не совпадёт с тем,
     * ID который пришёл в пакете - элемент кэша обновляется.
     * Если количество пришедших данных не совпадают с размером кэша.. кэш обновляется.
     *
     * КЭШ (ДОПОЛНЕНИЕ)
     * ===
     * Т.к. в общем случае, данные могут быть разбиты не несколько (много) пакетов, то для каждого из них выделен свой кэш и создан отдельный
     * map, ключом в котором является идентификатор данных (см. UDPMessage::getDataID()).
     * Кэш в map добавляется когда приходит пакет с новым UDPMessage::getDataID() и в дальнейшим используется для этого пакета.
     * В текущей реализации размер map не контролируется (завязан на UDPMessage::getDataID()) и рассчитан на статичность пакетов,
     * т.е. на то что UNetSender не будет с течением времени менять структуру отправляемых пакетов.
     *
     * Обработка сбоев в номере пакетов
     * =========================================================================
     * Если в какой-то момент расстояние между rnum и wnum превышает maxDifferens пакетов
     * то считается, что произошёл сбой или узел который посылал пакеты перезагрузился
     * Идёт попытка обработать все текущие пакеты (до первой дырки), а дальше происходит
     * реинициализация и обработка продолжается с нового номера.
     *
     * =========================================================================
     * ОПТИМИЗАЦИЯ N1: см. UNetSender.h. Если номер последнего принятого пакета не менялся.. не обрабатываем.
     *
     * Создание соединения (открытие сокета)
     * ======================================
     * Попытка создать сокет производиться сразу в конструкторе, если это не получается,
     * то создаётся таймер (evCheckConnection), который периодически (checkConnectionTime) пытается вновь
     * открыть сокет.. и так бесконечно, пока не получиться. Это важно для систем, где в момент загрузки программы
     * (в момент создания объекта UNetReceiver) ещё может быть не поднята сеть или какой-то сбой с сетью и требуется
     * ожидание (без вылета программы) пока "внешняя система мониторинга" не поднимет сеть).
     * Если такая логика не требуется, то можно задать в конструкторе
     * последним аргументом флаг nocheckconnection=true, тогда при создании объекта UNetReceiver, в конструкторе будет
     * выкинуто исключение при неудачной попытке создания соединения.
    */
    // -----------------------------------------------------------------------------
    class UNetReceiver:
        protected EvWatcher,
        public std::enable_shared_from_this<UNetReceiver>
    {
        public:
            UNetReceiver( std::unique_ptr<UNetReceiveTransport>&& transport, const std::shared_ptr<SMInterface>& smi
                          , bool nocheckConnection = false
                          , const std::string& prefix = "unet" );
            virtual ~UNetReceiver();

            void start();
            void stop();

            inline const std::string getName() const
            {
                return myname;
            }

            // блокировать сохранение данных в SM
            void setLockUpdate( bool st ) noexcept;
            bool isLockUpdate() const noexcept;

            void resetTimeout() noexcept;

            bool isInitOK() const noexcept;
            bool isRecvOK() const noexcept;
            size_t getLostPacketsNum() const noexcept;

            void setReceiveTimeout( timeout_t msec ) noexcept;
            void setUpdatePause( timeout_t msec ) noexcept;
            void setLostTimeout( timeout_t msec ) noexcept;
            void setPrepareTime( timeout_t msec ) noexcept;
            void setCheckConnectionPause( timeout_t msec ) noexcept;
            void setMaxDifferens( unsigned long set ) noexcept;
            void setEvrunTimeout(timeout_t msec ) noexcept;
            void setInitPause( timeout_t msec ) noexcept;
            void setBufferSize( size_t sz ) noexcept;
            void setMaxReceiveAtTime( size_t sz ) noexcept;

            void setRespondID( uniset::ObjectId id, bool invert = false ) noexcept;
            void setLostPacketsID( uniset::ObjectId id ) noexcept;

            void forceUpdate() noexcept; // пересохранить очередной пакет в SM даже если данные не менялись

            inline std::string getTransportID() const noexcept
            {
                return transport->ID();
            }

            /*! Коды событий */
            enum Event
            {
                evOK,        /*!< связь есть */
                evTimeout    /*!< потеря связи */
            };

            typedef sigc::slot<void, const std::shared_ptr<UNetReceiver>&, Event> EventSlot;
            void connectEvent( EventSlot sl ) noexcept;

            // --------------------------------------------------------------------
            inline std::shared_ptr<DebugStream> getLog()
            {
                return unetlog;
            }

            virtual const std::string getShortInfo() const noexcept;

        protected:

            const std::shared_ptr<SMInterface> shm;
            std::shared_ptr<DebugStream> unetlog;

            enum ReceiveRetCode
            {
                retOK = 0,
                retError = 1,
                retNoData = 2
            };

            ReceiveRetCode receive() noexcept;
            void step() noexcept;
            void update() noexcept;
            void callback( ev::io& watcher, int revents ) noexcept;
            void readEvent( ev::io& watcher ) noexcept;
            void updateEvent( ev::periodic& watcher, int revents ) noexcept;
            void checkConnectionEvent( ev::periodic& watcher, int revents ) noexcept;
            void statisticsEvent( ev::periodic& watcher, int revents ) noexcept;
            void initEvent( ev::timer& watcher, int revents ) noexcept;
            virtual void evprepare( const ev::loop_ref& eloop ) noexcept override;
            virtual void evfinish(const ev::loop_ref& eloop ) noexcept override;
            virtual std::string wname() const noexcept override
            {
                return myname;
            }

            void initIterators() noexcept;
            bool createConnection( bool throwEx = false );
            bool checkConnection();
            size_t rnext( size_t num );

        private:
            UNetReceiver();

            timeout_t updatepause = { 100 };   /*!< периодичность обновления данных в SM, [мсек] */

            std::unique_ptr<UNetReceiveTransport> transport;
            std::string addr;
            std::string myname;
            ev::io evReceive;
            ev::periodic evCheckConnection;
            ev::periodic evStatistic;
            ev::periodic evUpdate;
            ev::timer evInitPause;

            // счётчики для подсчёта статистики
            size_t recvCount = { 0 };
            size_t upCount = { 0 };

            // текущая статистика
            size_t statRecvPerSec = { 0 }; /*!< количество принимаемых пакетов в секунду */
            size_t statUpPerSec = { 0 };    /*!< количество обработанных пакетов в секунду */

            // делаем loop общим.. одним на всех!
            static CommonEventLoop loop;

            double checkConnectionTime = { 10.0 }; // sec
            std::mutex checkConnMutex;

            PassiveTimer ptRecvTimeout;
            PassiveTimer ptPrepare;
            timeout_t recvTimeout = { 5000 }; // msec
            timeout_t prepareTime = { 2000 };
            timeout_t evrunTimeout = { 15000 };
            timeout_t lostTimeout = { 200 };
            size_t maxReceiveCount = { 5 }; // количество читаемых за один раз

            double initPause = { 5.0 }; // пауза на начальную инициализацию (сек)
            std::atomic_bool initOK = { false };

            PassiveTimer ptLostTimeout;
            size_t lostPackets = { 0 }; /*!< счётчик потерянных пакетов */

            uniset::ObjectId sidRespond = { uniset::DefaultObjectId };
            IOController::IOStateList::iterator itRespond;
            bool respondInvert = { false };
            uniset::ObjectId sidLostPackets = { uniset::DefaultObjectId };
            IOController::IOStateList::iterator itLostPackets;

            std::atomic_bool activated = { false };

            size_t cbufSize = { 100 }; /*!< размер буфера для сообщений по умолчанию */
            std::vector<UniSetUDP::UDPMessage> cbuf; // circular buffer
            size_t wnum = { 1 }; /*!< номер следующего ожидаемого пакета */
            size_t rnum = { 0 }; /*!< номер последнего обработанного пакета */
            UniSetUDP::UDPMessage* pack; // текущий обрабатываемый пакет

            /*! максимальная разница между номерами пакетов, при которой считается, что счётчик пакетов
             * прошёл через максимум или сбился...
             */
            size_t maxDifferens = { 20 };

            std::atomic_bool lockUpdate = { false }; /*!< флаг блокировки сохранения принятых данных в SM */

            EventSlot slEvent;
            Trigger trTimeout;
            std::mutex tmMutex;

            struct CacheItem
            {
                long id = { uniset::DefaultObjectId };
                IOController::IOStateList::iterator ioit;

                CacheItem():
                    id(uniset::DefaultObjectId) {}
            };

            typedef std::vector<CacheItem> CacheVec;

            // ключом является UDPMessage::getDataID()
            typedef std::unordered_map<long, CacheVec> CacheMap;
            CacheMap d_icache_map;     /*!< кэш итераторов для булевых */
            CacheMap a_icache_map;     /*!< кэш итераторов для аналоговых */

            CacheVec* getDCache( UniSetUDP::UDPMessage* pack ) noexcept;
            CacheVec* getACache( UniSetUDP::UDPMessage* pack ) noexcept;
    };
    // --------------------------------------------------------------------------
} // end of namespace uniset
// -----------------------------------------------------------------------------
#endif // UNetReceiver_H_
// -----------------------------------------------------------------------------
