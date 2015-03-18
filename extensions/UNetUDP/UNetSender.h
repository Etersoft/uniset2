#ifndef UNetSender_H_
#define UNetSender_H_
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
/*
 *
 */
class UNetSender
{
	public:
		UNetSender( const std::string host, const ost::tpport_t port, SMInterface* smi,
					const std::string s_field="", const std::string s_fvalue="", SharedMemory* ic=0 );

		virtual ~UNetSender();
	
		struct UItem
		{
			UItem():
				iotype(UniversalIO::UnknownIOType),
				id(UniSetTypes::DefaultObjectId),
				pack_ind(-1){}

			UniversalIO::IOTypes iotype;
			UniSetTypes::ObjectId id;
			IOController::AIOStateList::iterator ait;
			IOController::DIOStateList::iterator dit;
			int pack_ind;

			friend std::ostream& operator<<( std::ostream& os, UItem& p );
		};
		
		typedef std::vector<UItem> DMap;

		void start();
		void stop();

		void send();
		void real_send();
		
		/*! (принудительно) обновить все данные (из SM) */
		virtual void updateFromSM();

		/*! Обновить значение по ID датчика */
		void updateSensor( UniSetTypes::ObjectId id, long value );

		/*! Обновить значение по итератору */
		void updateItem( DMap::iterator& it, long value );

		inline void setSendPause( int msec ){ sendpause = msec; }
		
		/*! заказать датчики */
		virtual void askSensors( UniversalIO::UIOCommand cmd );

		/*! инициализация  итераторов */
		void initIterators();
		
	protected:
		UNetSender();

		std::string myname;
		UniSetUDP::UDPMessage mypack;
		DMap dlist;
		int maxItem;
		
		std::string s_field;
		std::string s_fvalue;

		SMInterface* shm;

		virtual bool initItem( UniXML_iterator& it );
		bool readItem( UniXML& xml, UniXML_iterator& it, xmlNode* sec );

		virtual void readConfiguration();

	private:

		ost::UDPBroadcast* udp;
		ost::IPV4Address addr;
		ost::tpport_t port;
		std::string s_host;

		int sendpause;
		bool activated;
		
		UniSetTypes::uniset_mutex pack_mutex;
		unsigned long packetnum;
		UniSetUDP::UDPPacket s_msg;

		ThreadCreator<UNetSender>* s_thr;	// send thread
};
// -----------------------------------------------------------------------------
#endif // UNetSender_H_
// -----------------------------------------------------------------------------
