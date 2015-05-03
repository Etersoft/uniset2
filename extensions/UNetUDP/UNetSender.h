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
 *    ОПТИМИЗАЦИЯ N1: Для оптимизации обработки посылаемых пакетов (на стороне UNetReceiver) следана следующая логика:
 *                  Номер очередного посылаемого пакета меняется (увеличивается) только, если изменились данные с момента
                    последней посылки. Для по данным каждый раз производится расчёт UNetUDP::makeCRC() и сравнивается с последним..
 */
class UNetSender
{
	public:
		UNetSender( const std::string& host, const ost::tpport_t port, const std::shared_ptr<SMInterface>& smi,
					const std::string& s_field = "", const std::string& s_fvalue = "" );

		~UNetSender();

		struct UItem
		{
			UItem():
				iotype(UniversalIO::UnknownIOType),
				id(UniSetTypes::DefaultObjectId),
				pack_ind(-1) {}

			UniversalIO::IOType iotype;
			UniSetTypes::ObjectId id;
			IOController::IOStateList::iterator ioit;
			int pack_ind;

			friend std::ostream& operator<<( std::ostream& os, UItem& p );
		};

		typedef std::vector<UItem> DMap;

		void start();
		void stop();

		void send();
		void real_send();

		/*! (принудительно) обновить все данные (из SM) */
		void updateFromSM();

		/*! Обновить значение по ID датчика */
		void updateSensor( UniSetTypes::ObjectId id, long value );

		/*! Обновить значение по итератору */
		void updateItem( DMap::iterator& it, long value );

		inline void setSendPause( int msec )
		{
			sendpause = msec;
		}

		/*! заказать датчики */
		void askSensors( UniversalIO::UIOCommand cmd );

		/*! инициализация  итераторов */
		void initIterators();

	protected:

		std::string s_field;
		std::string s_fvalue;

		const std::shared_ptr<SMInterface> shm;

		bool initItem( UniXML::iterator& it );
		bool readItem( const std::shared_ptr<UniXML>& xml, UniXML::iterator& it, xmlNode* sec );

		void readConfiguration();

	private:
		UNetSender();

		std::shared_ptr<ost::UDPBroadcast> udp;
		ost::IPV4Address addr;
		ost::tpport_t port;
		std::string s_host;

		std::string myname;
		int sendpause;
		std::atomic_bool activated;

		UniSetTypes::uniset_rwmutex pack_mutex;
		UniSetUDP::UDPMessage mypack;
		DMap dlist;
		int maxItem;
		unsigned long packetnum;
		unsigned short lastcrc;
		UniSetUDP::UDPPacket s_msg;

		std::shared_ptr< ThreadCreator<UNetSender> > s_thr;    // send thread
};
// -----------------------------------------------------------------------------
#endif // UNetSender_H_
// -----------------------------------------------------------------------------
