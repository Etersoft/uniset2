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
// --------------------------------------------------------------------------
namespace uniset
{
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
	 *
	 * Стратегия обновления данных в SM
	 * ==================================
	 * При помощи функции setUpdateStrategy() можно выбрать стратегию обновления данных в SM.
	 * Поддерживается два варианта:
	 * 'thread' - отдельный поток обновления
	 * 'evloop' - использование общего с приёмом event loop (libev)
	*/
	// -----------------------------------------------------------------------------
	class UNetReceiver:
		protected EvWatcher,
		public std::enable_shared_from_this<UNetReceiver>
	{
		public:
			UNetReceiver( const std::string& host, int port, const std::shared_ptr<SMInterface>& smi
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
			void setReceivePause( timeout_t msec ) noexcept;
			void setUpdatePause( timeout_t msec ) noexcept;
			void setLostTimeout( timeout_t msec ) noexcept;
			void setPrepareTime( timeout_t msec ) noexcept;
			void setCheckConnectionPause( timeout_t msec ) noexcept;
			void setMaxDifferens( unsigned long set ) noexcept;
			void setEvrunTimeout(timeout_t msec ) noexcept;
			void setInitPause( timeout_t msec ) noexcept;

			void setRespondID( uniset::ObjectId id, bool invert = false ) noexcept;
			void setLostPacketsID( uniset::ObjectId id ) noexcept;

			void setMaxProcessingCount( int set ) noexcept;

			void forceUpdate() noexcept; // пересохранить очередной пакет в SM даже если данные не менялись

			inline std::string getAddress() const noexcept
			{
				return addr;
			}
			inline int getPort() const noexcept
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
			void connectEvent( EventSlot sl ) noexcept;

			// --------------------------------------------------------------------
			/*! Стратегия обработки сообщений */
			enum UpdateStrategy
			{
				useUpdateUnknown,
				useUpdateThread,	/*!< использовать отдельный поток */
				useUpdateEventLoop	/*!< использовать event loop (т.е. совместно с receive) */
			};

			static UpdateStrategy strToUpdateStrategy( const std::string& s ) noexcept;
			static std::string to_string( UpdateStrategy s) noexcept;

			//! функция должна вызываться до первого вызова start()
			void setUpdateStrategy( UpdateStrategy set );

			// специальная обёртка, захватывающая или нет mutex в зависимости от стратегии
			// (т.к. при evloop mutex захватытвать не нужно)
			class pack_guard
			{
				public:
					pack_guard( std::mutex& m, UpdateStrategy s );
					~pack_guard();

				protected:
					std::mutex& m;
					UpdateStrategy s;
			};

			// --------------------------------------------------------------------

			inline std::shared_ptr<DebugStream> getLog()
			{
				return unetlog;
			}

			virtual const std::string getShortInfo() const noexcept;

		protected:

			const std::shared_ptr<SMInterface> shm;
			std::shared_ptr<DebugStream> unetlog;

			bool receive() noexcept;
			void step() noexcept;
			void update() noexcept;
			void updateThread() noexcept;
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
			void checkConnection();

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

			std::unique_ptr<UDPReceiveU> udp;
			std::string addr;
			int port = { 0 };
			Poco::Net::SocketAddress saddr;
			std::string myname;
			ev::io evReceive;
			ev::periodic evCheckConnection;
			ev::periodic evStatistic;
			ev::periodic evUpdate;
			ev::timer evInitPause;

			UpdateStrategy upStrategy = { useUpdateEventLoop };

			// счётчики для подсчёта статистики
			size_t recvCount = { 0 };
			size_t upCount = { 0 };

			// текущая статистик
			size_t statRecvPerSec = { 0 }; /*!< количество принимаемых пакетов в секунду */
			size_t statUpPerSec = { 0 };	/*!< количество обработанных пакетов в секунду */

			std::unique_ptr< ThreadCreator<UNetReceiver> > upThread;    // update thread

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

			double initPause = { 5.0 }; // пауза на начальную инициализацию (сек)
			std::atomic_bool initOK = { false };

			PassiveTimer ptLostTimeout;
			size_t lostPackets = { 0 }; /*!< счётчик потерянных пакетов */

			uniset::ObjectId sidRespond = { uniset::DefaultObjectId };
			IOController::IOStateList::iterator itRespond;
			bool respondInvert = { false };
			uniset::ObjectId sidLostPackets;
			IOController::IOStateList::iterator itLostPackets;

			std::atomic_bool activated = { false };

			PacketQueue qpack;    /*!< очередь принятых пакетов (отсортированных по возрастанию номера пакета) */
			UniSetUDP::UDPMessage pack;        /*!< просто буфер для получения очередного сообщения */
			UniSetUDP::UDPPacket r_buf;
			std::mutex packMutex; /*!< mutex для работы с очередью */
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
			std::mutex tmMutex;

			struct CacheItem
			{
				long id = { uniset::DefaultObjectId };
				IOController::IOStateList::iterator ioit;

				CacheItem():
					id(uniset::DefaultObjectId) {}
			};

			typedef std::vector<CacheItem> CacheVec;
			struct CacheInfo
			{
				CacheInfo():
					cache_init_ok(false) {}

				bool cache_init_ok = { false };
				CacheVec cache;
			};

			// ключом является UDPMessage::getDataID()
			typedef std::unordered_map<long, CacheInfo> CacheMap;
			CacheMap d_icache_map;     /*!< кэш итераторов для булевых */
			CacheMap a_icache_map;     /*!< кэш итераторов для аналоговых */

			bool d_cache_init_ok = { false };
			bool a_cache_init_ok = { false };

			void initDCache( UniSetUDP::UDPMessage& pack, bool force = false ) noexcept;
			void initACache( UniSetUDP::UDPMessage& pack, bool force = false ) noexcept;
	};
	// --------------------------------------------------------------------------
} // end of namespace uniset
// -----------------------------------------------------------------------------
#endif // UNetReceiver_H_
// -----------------------------------------------------------------------------
