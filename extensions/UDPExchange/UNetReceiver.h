#ifndef UNetReceiver_H_
#define UNetReceiver_H_
// -----------------------------------------------------------------------------
#include <ostream>
#include <string>
#include <queue>
#include <cc++/socket.h>
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
 * (чем меньше тем старше). И при этом эта очередь постоянно поддерживается наполненной на minBufSize записей.
 * Это гарантирует, что соседние пакеты пришедшие не в той последовательности, тем не менее обработаны будут в правильной.
 * Т.к. в очереди они "отсортируются" по номеру пакета, ещё до обработки.
 *
 *
 * КЭШ
 * ===
 * Для оптимизации работы с SM, т.к. в пакетах приходят только пары [id,value] сделан кэш итераторов.
 * Кэш расчитан на то, что принимаемые пакеты всегда имеют одну и ту же длину и последовательность.
 * Идея проста: сделан вектор размером с количество принимаемых данных. В векторе хранятся итераторы (и всё что необходимо).
 * Порядокый номер данных в пакете является индексом в кэше.
 * Для защиты от изменения поседовательности внутри пакета, в кэше хранится ID сохраняемого датчика, и если он не совпадёт с тем,
 * ID который пришёл в пакете - элемент кэша обновляется.
 * Если количество пришедших данных не совпадают с размером кэша.. кэш обновляется.
 */
// -----------------------------------------------------------------------------
class UNetReceiver
{
	public:
		UNetReceiver( const std::string host, const ost::tpport_t port, SMInterface* smi );
		~UNetReceiver();

		 void start();

		 void receive();
		 void update();

		 inline bool isRecvOK(){ return ptRecvTimeout.checkTime(); }

		 void setReceiveTimeout( int msec );
		 void setReceivePause( int msec );
		 void setUpdatePause( int msec );

		 void setMinBudSize( int set );
		 void setMaxProcessingCount( int set );

		 inline ost::IPV4Address getAddress(){ return addr; }
		 inline ost::tpport_t getPort(){ return port; }

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

		ost::UDPDuplex* udp;
		ost::IPV4Address addr;
		ost::tpport_t port;
		std::string myname;

		UniSetTypes::uniset_mutex pollMutex;
		PassiveTimer ptRecvTimeout;
		int recvTimeout;

		bool activated;

		ThreadCreator<UNetReceiver>* r_thr;		// receive thread
		ThreadCreator<UNetReceiver>* u_thr;		// update thread

		// функция определения приоритетного сообщения для обработки
		struct PacketCompare:
		public std::binary_function<UniSetUDP::UDPMessage, UniSetUDP::UDPMessage, bool>
		{
			bool operator()(const UniSetUDP::UDPMessage& lhs,
							const UniSetUDP::UDPMessage& rhs) const;
		};
		typedef std::priority_queue<UniSetUDP::UDPMessage,std::vector<UniSetUDP::UDPMessage>,PacketCompare> PacketQueue;
		PacketQueue qpack;	/*!< очередь принятых пакетов (отсортированных по возрастанию номера пакета) */
		UniSetUDP::UDPMessage pack;		/*!< прсто буфер для получения очерещного сообщения */
		UniSetTypes::uniset_mutex packMutex; /*!< mutex для работы с очередью */
		long pnum;	/*!< текущий номер обработанного сообщения, для проверки непрерывности последовательности пакетов */

		/*! Минимальный размер очереди.
		 * Предназначен для создания буфера, чтобы обработка сообщений шла
		 * в порядке возрастания номеров пакетов. Даже если при приёме последовательность нарушалась
		 */
		int minBufSize;

		int maxProcessingCount; /*! максимальное число обрабатываемых за один раз сообщений */

		struct ItemInfo
		{
			long id;
			IOController::AIOStateList::iterator ait;
			IOController::DIOStateList::iterator dit;
			UniversalIO::IOTypes iotype;
		};

		typedef std::vector<ItemInfo> ItemVec;
		ItemVec icache;	/*!< кэш итераторов */
		bool cache_init_ok;
		void initCache( UniSetUDP::UDPMessage& pack, bool force=false );

};
// -----------------------------------------------------------------------------
#endif // UNetReceiver_H_
// -----------------------------------------------------------------------------
