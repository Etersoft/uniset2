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
};
// -----------------------------------------------------------------------------
#endif // UDPReceiver_H_
// -----------------------------------------------------------------------------
