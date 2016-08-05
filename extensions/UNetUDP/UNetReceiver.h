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
#include <queue>
#include <unordered_map>
#include <cc++/socket.h>
#include <sigc++/sigc++.h>
#include <ev++.h>
#include "UniSetObject.h"
#include "Trigger.h"
#include "Mutex.h"
#include "SMInterface.h"
#include "SharedMemory.h"
#include "UDPPacket.h"
#include "CommonEventLoop.h"
#include "UDPCore.h"
// -----------------------------------------------------------------------------
/*  Основная идея: сделать проверку очерёдности пакетов, но при этом использовать UDP.
 * ===============
 * Собственно реализация сделана так:
 * В данных передаётся номер пакета. На случай если несколько пакетов придут не в той последовательности
 * что были посланы, сделана очередь с приоритетом. В качестве приориета используется номер пакета
 * (чем меньше тем старше). При этом обработка ведётся только тех пакетов, которые идут "подряд",
 * как только встречается "дырка" происходит ожидание её "заполения". Если в течение времени (lostTimeout)
 * "дырка" не исчезает, увеличивается счётчик потерянных пакетов и обработка продолжается дальше..
 * Всё это реализовано в функции UNetReceiver::real_update()
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
 * В текущей реализации размер map не контролируется (завязан на UDPMessage::getDataID()) и расчитан на статичность пакетов,
 * т.е. на то что UNetSender не будет с течением времени менять структуру отправляемых пакетов.
 *
 * Обработка сбоя или переполнения счётчика пакетов(перехода через максимум)
 * =========================================================================
 * Для защиты от сбоя счётика сделана следующая логика:
 * Если номер очередного пришедшего пакета отличается от последнего обработанного на maxDifferens, то считается,
 * что произошёл сбой счётчика и происходит ожидание пока функция update, не обработает основную очередь полностью.
 * При этом принимаемые пакеты складываются во временную очередь qtmp. Как только основная очередь пустеет,
 * в неё копируется всё накопленное во временной очереди..и опять идёт штатная обработка.
 * Если во время "ожидания" опять происходит "разрыв" в номерах пакетов, то временная очередь чиститься
 * и данные которые в ней были теряются! Аналог ограниченного буфера (у любых карт), когда новые данные
 * затирают старые, если их не успели вынуть и обработать.
 * \todo Сделать защиту от бесконечного ожидания "очистки" основной очереди.
 * =========================================================================
 * ОПТИМИЗАЦИЯ N1: см. UNetSender.h. Если номер последнего принятого пакета не менялся.. не обрабатываем..
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
		UNetReceiver( const std::string& host, const ost::tpport_t port, const std::shared_ptr<SMInterface>& smi, bool nocheckConnection = false );
		virtual ~UNetReceiver();

		void start();
		void stop();

		inline const std::string getName() const
		{
			return myname;
		}

		// блокировать сохранение данных в SM
		void setLockUpdate( bool st );
		inline bool isLockUpdate() const
		{
			return lockUpdate;
		}

		void resetTimeout();

		inline bool isRecvOK() const
		{
			return !ptRecvTimeout.checkTime();
		}
		inline size_t getLostPacketsNum() const
		{
			return lostPackets;
		}

		void setReceiveTimeout( timeout_t msec );
		void setReceivePause( timeout_t msec );
		void setUpdatePause( timeout_t msec );
		void setLostTimeout( timeout_t msec );
		void setPrepareTime( timeout_t msec );
		void setCheckConnectionPause( timeout_t msec );
		void setMaxDifferens( unsigned long set );

		void setRespondID( UniSetTypes::ObjectId id, bool invert = false );
		void setLostPacketsID( UniSetTypes::ObjectId id );

		void setMaxProcessingCount( int set );

		void forceUpdate(); // пересохранить очередной пакет в SM даже если данные не менялись

		inline ost::IPV4Address getAddress() const
		{
			return addr;
		}
		inline ost::tpport_t getPort() const
		{
			return port;
		}

		/*! Коды событий */
		enum Event
		{
			evOK,        /*!< связь есть */
			evTimeout    /*!< потеря связи */
		};

		typedef sigc::slot<void, const std::shared_ptr<UNetReceiver>&, Event> EventSlot;
		void connectEvent( EventSlot sl );

		inline std::shared_ptr<DebugStream> getLog()
		{
			return unetlog;
		}

		virtual const std::string getShortInfo() const;

	protected:

		const std::shared_ptr<SMInterface> shm;
		std::shared_ptr<DebugStream> unetlog;

		bool receive();
		void step();
		void update();
		void callback( ev::io& watcher, int revents );
		void readEvent( ev::io& watcher );
		void updateEvent( ev::periodic& watcher, int revents );
		void checkConnectionEvent( ev::periodic& watcher, int revents );
		void forceUpdateEvent( ev::timer& watcher, int revents );
		virtual void evprepare( const ev::loop_ref& eloop ) override;
		virtual void evfinish(const ev::loop_ref& eloop ) override;
		virtual std::string wname() override
		{
			return myname;
		}

		void initIterators();
		bool createConnection( bool throwEx = false );

	public:

		// функция определения приоритетного сообщения для обработки
		struct PacketCompare:
			public std::binary_function<UniSetUDP::UDPMessage, UniSetUDP::UDPMessage, bool>
		{
			inline bool operator()(const UniSetUDP::UDPMessage& lhs,
								   const UniSetUDP::UDPMessage& rhs) const
			{
				return lhs.num > rhs.num;
			}
		};
		typedef std::priority_queue<UniSetUDP::UDPMessage, std::vector<UniSetUDP::UDPMessage>, PacketCompare> PacketQueue;

	private:
		UNetReceiver();

		timeout_t recvpause = { 10 };      /*!< пауза меджду приёмами пакетов, [мсек] */
		timeout_t updatepause = { 100 };    /*!< переодичность обновления данных в SM, [мсек] */

		std::shared_ptr<UDPReceiveU> udp;
		ost::IPV4Address addr;
		ost::tpport_t port = { 0 };
		std::string myname;
		ev::io evReceive;
		ev::periodic evUpdate;
		ev::periodic evCheckConnection;
		ev::timer evForceUpdate;

		// делаем loop общим.. одним на всех!
		static CommonEventLoop loop;

		double updateTime = { 0.01 };
		double checkConnectionTime = { 10.0 }; // sec

		PassiveTimer ptRecvTimeout;
		PassiveTimer ptPrepare;
		timeout_t recvTimeout = { 5000 }; // msec
		timeout_t prepareTime = { 2000 };
		timeout_t lostTimeout = { 200 };
		PassiveTimer ptLostTimeout;
		size_t lostPackets = { 0 }; /*!< счётчик потерянных пакетов */

		UniSetTypes::ObjectId sidRespond = { UniSetTypes::DefaultObjectId };
		IOController::IOStateList::iterator itRespond;
		bool respondInvert = { false };
		UniSetTypes::ObjectId sidLostPackets;
		IOController::IOStateList::iterator itLostPackets;

		std::atomic_bool activated = { false };

		PacketQueue qpack;    /*!< очередь принятых пакетов (отсортированных по возрастанию номера пакета) */
		UniSetUDP::UDPMessage pack;        /*!< просто буфер для получения очередного сообщения */
		UniSetUDP::UDPPacket r_buf;
		size_t pnum = { 0 };    /*!< текущий номер обработанного сообщения, для проверки непрерывности последовательности пакетов */

		/*! максимальная разница межд номерами пакетов, при которой считается, что счётчик пакетов
		 * прошёл через максимум или сбился...
		 */
		size_t maxDifferens = { 20 };

		PacketQueue qtmp;    /*!< очередь на время обработки(очистки) основной очереди */
		bool waitClean = { false };    /*!< флаг означающий, что ждём очистики очереди до конца */
		size_t rnum = { 0 };    /*!< текущий номер принятого сообщения, для проверки "переполнения" или "сбоя" счётчика */

		size_t maxProcessingCount = { 100 }; /*!< максимальное число обрабатываемых за один раз сообщений */

		std::atomic_bool lockUpdate = { false }; /*!< флаг блокировки сохранения принятых данных в SM */

		EventSlot slEvent;
		Trigger trTimeout;
		UniSetTypes::uniset_rwmutex tmMutex;

		struct CacheItem
		{
			long id;
			IOController::IOStateList::iterator ioit;
			UniversalIO::IOType iotype;

			CacheItem():
				id(UniSetTypes::DefaultObjectId), iotype(UniversalIO::UnknownIOType) {}
		};

		typedef std::vector<CacheItem> CacheVec;
		struct CacheInfo
		{
			CacheInfo():
				cache_init_ok(false)
			{
			}
			bool cache_init_ok;
			CacheVec cache;
		};
		// ключом является UDPMessage::getDataID()
		typedef std::unordered_map<long, CacheInfo> CacheMap;
		CacheMap d_icache_map;     /*!< кэш итераторов для булевых */
		CacheMap a_icache_map;     /*!< кэш итераторов для аналоговых */

		bool d_cache_init_ok = { false };
		bool a_cache_init_ok = { false };

		void initDCache( UniSetUDP::UDPMessage& pack, bool force = false );
		void initACache( UniSetUDP::UDPMessage& pack, bool force = false );
};
// -----------------------------------------------------------------------------
#endif // UNetReceiver_H_
// -----------------------------------------------------------------------------
