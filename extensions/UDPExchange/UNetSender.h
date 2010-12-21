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
#include "UDPNReceiver.h"
// -----------------------------------------------------------------------------
/*
 * Для защиты от потери пакета при переполнении "номера пакета".
 * UNetReceiver при обнаружении "разырва" в последовательнности, просто игнорирует пакет, обновляет счётчик
 * и начинает обработку уже со следующего.
 * Соотвественно здесь, реализован следующий механизм: При переходе номера пакета через maxnum,
 * в сеть посылается один и тоже пакет данных с номерами идущими подряд.
 * В результате первй будет откинут, как идущий "не подряд", а второй - будет обработан.
 */
class UNetSender
{
	public:
		UNetSender( const std::string host, const ost::tpport_t port, SMInterface* smi,
					const std::string s_field="", const std::string s_fvalue="", SharedMemory* ic=0 );

		~UNetSender();

		struct UItem
		{
			UItem():
				val(0)
			{}

			IOController_i::SensorInfo si;
			IOController::AIOStateList::iterator ait;
			IOController::DIOStateList::iterator dit;
			UniSetTypes::uniset_spin_mutex val_lock;
			int pack_ind;
			long val;

			friend std::ostream& operator<<( std::ostream& os, UItem& p );
		};

		void start();

		void send();
		void real_send();
		void update( UniSetTypes::ObjectId id, long value );

		inline void setSendPause( int msec ){ sendpause = msec; }

	protected:

		std::string s_field;
		std::string s_fvalue;

		SMInterface* shm;

		void initIterators();
		bool initItem( UniXML_iterator& it );
		bool readItem( UniXML& xml, UniXML_iterator& it, xmlNode* sec );

		void readConfiguration();
		bool check_item( UniXML_iterator& it );

	private:
		UNetSender();

		ost::UDPBroadcast* udp;
		ost::IPV4Address addr;
		ost::tpport_t port;
		std::string s_host;

		std::string myname;
		int sendpause;
		bool activated;

		UniSetUDP::UDPMessage mypack;
		typedef std::vector<UItem> DMap;
		DMap dlist;
		int maxItem;
		long packetnum;

		ThreadCreator<UNetSender>* s_thr;	// send thread
};
// -----------------------------------------------------------------------------
#endif // UNetSender_H_
// -----------------------------------------------------------------------------
