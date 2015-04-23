#ifndef UNetReceiver_H_
#define UNetReceiver_H_
// -----------------------------------------------------------------------------
#include <ostream>
#include <memory>
#include <string>
#include <queue>
#include <cc++/socket.h>
#include <sigc++/sigc++.h>
#include "UniSetObject_LT.h"
#include "Trigger.h"
#include "Mutex.h"
#include "SMInterface.h"
#include "SharedMemory.h"
#include "ThreadCreator.h"
#include "UDPPacket.h"
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
 * map, ключом в котором является идентификатор данных (см. UDPMessage::getIdentity()).
 * Кэш в map добавляется когда приходит пакет с новым UDPMessage::getIdentity() и в дальнейшим используется для этого пакета.
 * Так текущий момент размер map не контролируется (завязан на UDPMessage::getIdentity()) и расчитан на статичность пакетов,
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
*/
// -----------------------------------------------------------------------------
class UNetReceiver:
	public std::enable_shared_from_this<UNetReceiver>
{
	public:
		UNetReceiver( const std::string& host, const ost::tpport_t port, const std::shared_ptr<SMInterface>& smi );
		virtual ~UNetReceiver();

		void start();
		void stop();

		void receive();
		void update();

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
		inline unsigned long getLostPacketsNum() const
		{
			return lostPackets;
		}

		void setReceiveTimeout( timeout_t msec );
		void setReceivePause( timeout_t msec );
		void setUpdatePause( timeout_t msec );
		void setLostTimeout( timeout_t msec );
		void setPrepareTime( timeout_t msec );
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

		bool recv();
		void step();
		void real_update();

		void initIterators();

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

		int recvpause;        /*!< пауза меджду приёмами пакетов, [мсек] */
		int updatepause;    /*!< переодичность обновления данных в SM, [мсек] */

		std::shared_ptr<ost::UDPReceive> udp;
		ost::IPV4Address addr;
		ost::tpport_t port;
		std::string myname;

		UniSetTypes::uniset_rwmutex pollMutex;
		PassiveTimer ptRecvTimeout;
		PassiveTimer ptPrepare;
		timeout_t recvTimeout;
		timeout_t prepareTime;
		timeout_t lostTimeout;
		PassiveTimer ptLostTimeout;
		unsigned long lostPackets; /*!< счётчик потерянных пакетов */

		UniSetTypes::ObjectId sidRespond;
		IOController::IOStateList::iterator itRespond;
		bool respondInvert;
		UniSetTypes::ObjectId sidLostPackets;
		IOController::IOStateList::iterator itLostPackets;

		std::atomic_bool activated;

		std::shared_ptr< ThreadCreator<UNetReceiver> > r_thr;        // receive thread
		std::shared_ptr< ThreadCreator<UNetReceiver> > u_thr;        // update thread

		PacketQueue qpack;    /*!< очередь принятых пакетов (отсортированных по возрастанию номера пакета) */
		UniSetUDP::UDPMessage pack;        /*!< просто буфер для получения очередного сообщения */
		UniSetUDP::UDPPacket r_buf;
		UniSetTypes::uniset_rwmutex packMutex; /*!< mutex для работы с очередью */
		unsigned long pnum;    /*!< текущий номер обработанного сообщения, для проверки непрерывности последовательности пакетов */

		/*! максимальная разница межд номерами пакетов, при которой считается, что счётчик пакетов
		 * прошёл через максимум или сбился...
		 */
		unsigned long maxDifferens;

		PacketQueue qtmp;    /*!< очередь на время обработки(очистки) основной очереди */
		bool waitClean;        /*!< флаг означающий, что ждём очистики очереди до конца */
		unsigned long rnum;    /*!< текущий номер принятого сообщения, для проверки "переполнения" или "сбоя" счётчика */

		int maxProcessingCount; /*!< максимальное число обрабатываемых за один раз сообщений */

		bool lockUpdate; /*!< флаг блокировки сохранения принятых данных в SM */
		UniSetTypes::uniset_rwmutex lockMutex;

		EventSlot slEvent;
		Trigger trTimeout;
		UniSetTypes::uniset_rwmutex tmMutex;

		struct ItemInfo
		{
			long id;
			IOController::IOStateList::iterator ioit;
			UniversalIO::IOType iotype;

			ItemInfo():
				id(UniSetTypes::DefaultObjectId), iotype(UniversalIO::UnknownIOType) {}
		};

		typedef std::vector<ItemInfo> ItemVec;
		ItemVec d_icache;    /*!< кэш итераторов для булевых */
		ItemVec a_icache;    /*!< кэш итераторов для аналоговых */

		bool d_cache_init_ok;
		bool a_cache_init_ok;

		void initDCache( UniSetUDP::UDPMessage& pack, bool force = false );
		void initACache( UniSetUDP::UDPMessage& pack, bool force = false );
};
// -----------------------------------------------------------------------------
#endif // UNetReceiver_H_
// -----------------------------------------------------------------------------
