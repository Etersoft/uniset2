#ifndef UNetExchange_H_
#define UNetExchange_H_
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
#include "UNetPacket.h"
#include "UNetNReceiver.h"
// -----------------------------------------------------------------------------
class UNetExchange:
	public UniSetObject_LT
{
	public:
		UNetExchange( UniSetTypes::ObjectId objId, UniSetTypes::ObjectId shmID, SharedMemory* ic=0 );
		virtual ~UNetExchange();

		/*! глобальная функция для инициализации объекта */
		static UNetExchange* init_UNetexchange( int argc, char* argv[],
											UniSetTypes::ObjectId shmID, SharedMemory* ic=0 );

		/*! глобальная функция для вывода help-а */
		static void help_print( int argc, char* argv[] );

		struct UItem
		{
			UItem():
				val(0)
			{}

			IOController_i::SensorInfo si;
			IOController::AIOStateList::iterator ait;
			IOController::DIOStateList::iterator dit;
			UniSetTypes::uniset_spin_mutex val_lock;
			UniSetUNet::UNetMessage::UNetDataList::iterator pack_it;
			long val;

			friend std::ostream& operator<<( std::ostream& os, UItem& p );
		};

	protected:

		xmlNode* cnode;
		std::string s_field;
		std::string s_fvalue;

		SMInterface* shm;

		void poll();
		void recv();
		void send();

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
		bool initItem( UniXML_iterator& it );
		bool readItem( UniXML& xml, UniXML_iterator& it, xmlNode* sec );

		void readConfiguration();
		bool check_item( UniXML_iterator& it );
		void buildReceiverList();

	private:
		UNetExchange();
		bool initPause;
		UniSetTypes::uniset_mutex mutex_start;

		PassiveTimer ptHeartBeat;
		UniSetTypes::ObjectId sidHeartBeat;
		int maxHeartBeat;
		IOController::AIOStateList::iterator aitHeartBeat;
		UniSetTypes::ObjectId test_id;

		int polltime;	/*!< переодичность обновления данных, [мсек] */

		ost::UNetBroadcast* UNet;
		ost::IPV4Host host;
		ost::tpport_t port;
		std::string s_host;

		UniSetTypes::uniset_mutex pollMutex;
		Trigger trTimeout;
		int recvTimeout;
		int sendTimeout;

		bool activated;
		int activateTimeout;

		UniSetUNet::UNetMessage mypack;
		typedef std::vector<UItem> DMap;
		DMap dlist;
		int maxItem;


		typedef std::list<UNetNReceiver*> ReceiverList;
		ReceiverList rlist;

		ThreadCreator<UNetExchange>* thr;
};
// -----------------------------------------------------------------------------
#endif // UNetExchange_H_
// -----------------------------------------------------------------------------
