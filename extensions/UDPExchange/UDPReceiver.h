#ifndef UDPReceiver_H_
#define UDPReceiver_H_
// -----------------------------------------------------------------------------
#include <ostream>
#include <string>
#include <map>
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
		virtual void step();
		void update_data();

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
			tmExchange
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

		int polltime;	/*!< переодичность обновления данных, [мсек] */

		ost::UDPDuplex* udp;
		ost::IPV4Host host;
		ost::tpport_t port;

		UniSetTypes::uniset_mutex pollMutex;
		Trigger trTimeout;
		int recvTimeout;

		bool activated;
		int activateTimeout;

		long pnum;

		ThreadCreator<UDPReceiver>* thr;

//		typedef std::map<unsigned long,UniSetUDP::UDPMessage> QueuePacket;
//		QueuePacket qpack;
		UniSetUDP::UDPMessage pack;
		UniSetTypes::uniset_mutex packMutex;

		// функция определения приоритетного сообщения для обработки
		struct PacketCompare:
		public std::binary_function<UniSetUDP::UDPMessage, UniSetUDP::UDPMessage, bool>
		{
			bool operator()(const UniSetUDP::UDPMessage& lhs,
							const UniSetUDP::UDPMessage& rhs) const;
		};
		typedef std::priority_queue<UniSetUDP::UDPMessage,std::vector<UniSetUDP::UDPMessage>,PacketCompare> PacketQueue;
		PacketQueue qpack;
		static const int max_buf_size = 20;

		PassiveTimer ptUpdate;
};
// -----------------------------------------------------------------------------
#endif // UDPReceiver_H_
// -----------------------------------------------------------------------------
