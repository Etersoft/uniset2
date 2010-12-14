#ifndef UDPReceiver_H_
#define UDPReceiver_H_
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
 * что были посланы, сделана очередь с приориеттом. В качестве приориета используется номер пакета
 * (чем меньше тем старше). И при этом эта очередь постоянно поддерживается наполненной на minBufSize записей.
 * Это гарантирует, что соседние пакеты пришедшие не в той последовательности, тем не менее обработаны будут в правильной.
 * Т.к. в очереди они "отсортируются" по номеру пакета, ещё до обработки.
 *
 *
 * КЭШ
 * ===
 * Для оптимизации работы с SM, т.к. в пакетах приходят только пары [id,value] сделан кэш итераторов.
 * Кэш расчитан на то, что принимаемые пакеты всегда имеют одну и ту же длину и последовательность.
 * Идея проста: сделан вектор размером с количество принимаемы данных. В векторе хранятся итераторы (и всё что необходимо).
 * Порядокый номер данных в пакете является индексом в кэше.
 * Для защиты от изменения поседовательности внутри пакета, в кэше хранится ID сохраняемого датчика, и если он не совпадёт с тем,
 * ID который пришёл в пакете - элемент кэша обновляется.
 * Если количество пришедших данных не совпадают с размеров кэша.. кэш обновляется.
 */
// -----------------------------------------------------------------------------
class UDPReceiver:
	public UniSetObject_LT
{
	public:
		UDPReceiver( UniSetTypes::ObjectId objId, UniSetTypes::ObjectId shmID, SharedMemory* ic=0 );
		virtual ~UDPReceiver();
	
		/*! глобальная функция для инициализации объекта */
		static UDPReceiver* init_udpreceiver( int argc, char* argv[],
											UniSetTypes::ObjectId shmID, SharedMemory* ic=0 );

		/*! глобальная функция для вывода help-а */
		static void help_print( int argc, char* argv[] );
	protected:

		xmlNode* cnode;
		std::string s_field;
		std::string s_fvalue;

		SMInterface* shm;

		void poll();
		void recv();
		void step();
		void update();

		virtual void processingMessage( UniSetTypes::VoidMessage *msg );
		void sysCommand( UniSetTypes::SystemMessage *msg );
		void sensorInfo( UniSetTypes::SensorMessage*sm );
		void timerInfo( UniSetTypes::TimerMessage *tm );
		void askSensors( UniversalIO::UIOCommand cmd );
		void waitSMReady();

		virtual bool activateObject();
		
		// действия при завершении работы
		virtual void sigterm( int signo );

		void initIterators();

		enum Timer
		{
			tmUpdate,
			tmStep
		};

	private:
		UDPReceiver();
		bool initPause;
		UniSetTypes::uniset_mutex mutex_start;

		PassiveTimer ptHeartBeat;
		UniSetTypes::ObjectId sidHeartBeat;
		int maxHeartBeat;
		IOController::AIOStateList::iterator aitHeartBeat;
		UniSetTypes::ObjectId test_id;

		int polltime;	/*!< пауза меджду приёмами пакетов, [мсек] */
		int updatetime;	/*!< переодичность обновления данных в SM, [мсек] */
		int steptime;	/*!< периодичность вызова step, [мсек] */

		ost::UDPDuplex* udp;
		ost::IPV4Host host;
		ost::tpport_t port;

		UniSetTypes::uniset_mutex pollMutex;
		Trigger trTimeout;
		int recvTimeout;
		
		bool activated;
		int activateTimeout;
		
		ThreadCreator<UDPReceiver>* thr;

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
#endif // UDPReceiver_H_
// -----------------------------------------------------------------------------
