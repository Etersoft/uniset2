#ifndef UNetExchange_H_
#define UNetExchange_H_
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
#include "UNetReceiver.h"
#include "UNetSender.h"
// -----------------------------------------------------------------------------
class UNetExchange:
	public UniSetObject_LT
{
	public:
		UNetExchange( UniSetTypes::ObjectId objId, UniSetTypes::ObjectId shmID, SharedMemory* ic=0 );
		virtual ~UNetExchange();
	
		/*! глобальная функция для инициализации объекта */
		static UNetExchange* init_unetexchange( int argc, char* argv[],
											UniSetTypes::ObjectId shmID, SharedMemory* ic=0 );

		/*! глобальная функция для вывода help-а */
		static void help_print( int argc, char* argv[] );

		bool checkExistUNetHost( const std::string host, ost::tpport_t port );

	protected:

		xmlNode* cnode;
		std::string s_field;
		std::string s_fvalue;

		SMInterface* shm;
		void step();

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
		void startReceivers();
		void initSender( const std::string host, const ost::tpport_t port, UniXML_iterator& it );

		enum Timer
		{
			tmStep
		};

	private:
		UNetExchange();
		bool initPause;
		UniSetTypes::uniset_mutex mutex_start;

		PassiveTimer ptHeartBeat;
		UniSetTypes::ObjectId sidHeartBeat;
		int maxHeartBeat;
		IOController::AIOStateList::iterator aitHeartBeat;
		UniSetTypes::ObjectId test_id;

		int steptime;	/*!< периодичность вызова step, [мсек] */

		bool activated;
		int activateTimeout;

		typedef std::list<UNetReceiver*> ReceiverList;
		ReceiverList recvlist;

		bool no_sender;  /*!< флаг отключения посылки сообщений */
		UNetSender* sender;
};
// -----------------------------------------------------------------------------
#endif // UNetExchange_H_
// -----------------------------------------------------------------------------
