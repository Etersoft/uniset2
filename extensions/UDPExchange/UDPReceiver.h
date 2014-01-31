#ifndef UDPReceiver_H_
#define UDPReceiver_H_
// -----------------------------------------------------------------------------
#include <ostream>
#include <string>
#include <vector>
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

		virtual void processingMessage( UniSetTypes::VoidMessage *msg );
		void sysCommand( UniSetTypes::SystemMessage *msg );
		void sensorInfo( UniSetTypes::SensorMessage*sm );
		void askSensors( UniversalIO::UIOCommand cmd );
		void waitSMReady();

		virtual bool activateObject();

		// действия при завершении работы
		virtual void sigterm( int signo );

		void initIterators();

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

		ThreadCreator<UDPReceiver>* thr;
};
// -----------------------------------------------------------------------------
#endif // UDPReceiver_H_
// -----------------------------------------------------------------------------
