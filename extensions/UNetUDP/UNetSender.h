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
#include "DebugStream.h"
// -----------------------------------------------------------------------------
/*
 *    Распределение датчиков по пакетам
 *    =========================================================================
 *    В функции initItem() идет добавление датчика в пакет и создание нового пакета при переполнении. Причем так как дискретные и аналоговые
 *    датчики обрабатываются отдельно, то датчики, которые первые переполнятся, те и создадут новый пакет. "Отставшие" же будут использовать уже созданные.
 *    В свою очередь в initItem() каждому UItem в dlist кроме pack_ind присваивается еще и номер пакета pack_num, который гарантировано соответствует
 *    существующему пакету, поэтому в дальнейшем при использовании pack_num в качестве ключа в mypacks мы не проверяем пакет на существование.
 *
 *    ОПТИМИЗАЦИЯ N1: Для оптимизации обработки посылаемых пакетов (на стороне UNetReceiver) следана следующая логика:
 *                  Номер очередного посылаемого пакета меняется (увеличивается) только, если изменились данные с момента
                    последней посылки. Для этого по данным каждый раз производится расчёт UNetUDP::makeCRC() и сравнивается с последним..
 */
class UNetSender
{
	public:
		UNetSender( const std::string& host, const ost::tpport_t port, const std::shared_ptr<SMInterface>& smi,
					const std::string& s_field = "", const std::string& s_fvalue = "" );

		virtual ~UNetSender();

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

		inline std::shared_ptr<DebugStream> getLog()
		{
			return unetlog;
		}

		virtual const std::string getShortInfo() const;


		inline ost::IPV4Address getAddress() const
		{
			return addr;
		}
		inline ost::tpport_t getPort() const
		{
			return port;
		}

	protected:

		std::string s_field;
		std::string s_fvalue;

		const std::shared_ptr<SMInterface> shm;
		std::shared_ptr<DebugStream> unetlog;

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
