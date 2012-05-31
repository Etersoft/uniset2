#ifndef UNetReceiver_H_
#define UNetReceiver_H_
// -----------------------------------------------------------------------------
#include <ostream>
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
 * Кэш расчитан на то, что принимаемые пакеты всегда имеют одну и ту же длину и последовательность.
 * Идея проста: сделан вектор размером с количество принимаемых данных. В векторе хранятся итераторы (и всё что необходимо).
 * Порядковый номер данных в пакете является индексом в кэше.
 * Для защиты от изменения поседовательности внутри пакета, в кэше хранится ID сохраняемого датчика, и если он не совпадёт с тем,
 * ID который пришёл в пакете - элемент кэша обновляется.
 * Если количество пришедших данных не совпадают с размером кэша.. кэш обновляется.
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
*/
// -----------------------------------------------------------------------------
class UNetReceiver
{
	public:
		UNetReceiver( const std::string host, const ost::tpport_t port, SMInterface* smi );
		~UNetReceiver();

		 void start();
		 void stop();

		 void receive();
		 void update();

		 inline std::string getName(){ return myname; }

		 // блокировать сохранение данный в SM
		 void setLockUpdate( bool st );

		 void resetTimeout();

		 inline bool isRecvOK(){ return !ptRecvTimeout.checkTime(); }
		 inline unsigned long getLostPacketsNum(){ return lostPackets; }

		 void setReceiveTimeout( timeout_t msec );
		 void setReceivePause( timeout_t msec );
		 void setUpdatePause( timeout_t msec );
		 void setLostTimeout( timeout_t msec );
		 void setMaxDifferens( unsigned long set );

		 void setRespondID( UniSetTypes::ObjectId id, bool invert=false );
		 void setLostPacketsID( UniSetTypes::ObjectId id );

		 void setMaxProcessingCount( int set );

		 inline ost::IPV4Address getAddress(){ return addr; }
		 inline ost::tpport_t getPort(){ return port; }

		 /*! Коды событий */
		 enum Event
		 {
			 evOK,		/*!< связь есть */
			 evTimeout	/*!< потеря связи */
		 };

		 typedef sigc::slot<void,UNetReceiver*,Event> EventSlot;
		 void connectEvent( EventSlot sl );

	protected:

		SMInterface* shm;

		bool recv();
		void step();
		void real_update();

		void initIterators();

	private:
		UNetReceiver();

		int recvpause;		/*!< пауза меджду приёмами пакетов, [мсек] */
		int updatepause;	/*!< переодичность обновления данных в SM, [мсек] */

		ost::UDPReceive* udp;
		ost::IPV4Address addr;
		ost::tpport_t port;
		std::string myname;

		UniSetTypes::uniset_mutex pollMutex;
		PassiveTimer ptRecvTimeout;
		timeout_t recvTimeout;
		timeout_t lostTimeout;
		PassiveTimer ptLostTimeout;
		unsigned long lostPackets; /*!< счётчик потерянных пакетов */

		UniSetTypes::ObjectId sidRespond;
		IOController::DIOStateList::iterator ditRespond;
		bool respondInvert;
		UniSetTypes::ObjectId sidLostPackets;
		IOController::AIOStateList::iterator aitLostPackets;

		bool activated;

		ThreadCreator<UNetReceiver>* r_thr;		// receive thread
		ThreadCreator<UNetReceiver>* u_thr;		// update thread

		// функция определения приоритетного сообщения для обработки
		struct PacketCompare:
		public std::binary_function<UniSetUDP::UDPMessage, UniSetUDP::UDPMessage, bool>
		{
			inline bool operator()(const UniSetUDP::UDPMessage& lhs,
							const UniSetUDP::UDPMessage& rhs) const
					{ return lhs.num > rhs.num; }
		};
		typedef std::priority_queue<UniSetUDP::UDPMessage,std::vector<UniSetUDP::UDPMessage>,PacketCompare> PacketQueue;
		PacketQueue qpack;	/*!< очередь принятых пакетов (отсортированных по возрастанию номера пакета) */
		UniSetUDP::UDPMessage pack;		/*!< просто буфер для получения очередного сообщения */
		UniSetUDP::UDPPacket r_buf;
		UniSetTypes::uniset_mutex packMutex; /*!< mutex для работы с очередью */
		unsigned long pnum;	/*!< текущий номер обработанного сообщения, для проверки непрерывности последовательности пакетов */

		/*! максимальная разница межд номерами пакетов, при которой считается, что счётчик пакетов
		 * прошёл через максимум или сбился...
		 */
		unsigned long maxDifferens;

		PacketQueue qtmp;	/*!< очередь на время обработки(очистки) основной очереди */
		bool waitClean;		/*!< флаг означающий, что ждём очистики очереди до конца */
		unsigned long rnum;	/*!< текущий номер принятого сообщения, для проверки "переполнения" или "сбоя" счётчика */

		int maxProcessingCount; /*!< максимальное число обрабатываемых за один раз сообщений */

		bool lockUpdate; /*!< флаг блокировки сохранения принятых данных в SM */
		UniSetTypes::uniset_mutex lockMutex;

		EventSlot slEvent;
		Trigger trTimeout;
		UniSetTypes::uniset_mutex tmMutex;

		struct ItemInfo
		{
			long id;
			IOController::AIOStateList::iterator ait;
			IOController::DIOStateList::iterator dit;
			UniversalIO::IOTypes iotype;

			ItemInfo():
				id(UniSetTypes::DefaultObjectId),
				iotype(UniversalIO::UnknownIOType){}
		};

		typedef std::vector<ItemInfo> ItemVec;
		ItemVec d_icache;	/*!< кэш итераторов для булевых */
		ItemVec a_icache;	/*!< кэш итераторов для аналоговых */

		bool d_cache_init_ok;
		bool a_cache_init_ok;

		void initDCache( UniSetUDP::UDPMessage& pack, bool force=false );
		void initACache( UniSetUDP::UDPMessage& pack, bool force=false );
};
// -----------------------------------------------------------------------------
#endif // UNetReceiver_H_
// -----------------------------------------------------------------------------
