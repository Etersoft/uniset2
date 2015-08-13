#ifndef UNetSender_H_
#define UNetSender_H_
// -----------------------------------------------------------------------------
#include <ostream>
#include <string>
#include <vector>
#include <unordered_map>
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
 *    Распределение датчиков по пакетам
 *    =========================================================================
 *	  Все пересылаемые данные разбиваются в группы по частоте посылки("sendfactor").
 *    Частота посылки кратна sendpause, задаётся для каждого датчика, при помощи свойства prefix_sendfactor.
 *    Внутри каждой группы пакеты набираются по мере "заполнения".
 *
 *    Добавление датчика в пакет и создание нового пакета при переполнении происходит в функции initItem().
 *    Причем так как дискретные и аналоговые датчики обрабатываются отдельно (но пересылаются в одном пакете),
 *    то датчики, которые первые переполнятся приводят к тому, что создаётся новый пакет.
 *    "Отставшие" же будут использовать уже созданные.
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
					const std::string& s_field = "", const std::string& s_fvalue = "", const std::string& prefix = "unet",
					size_t maxDCount = UniSetUDP::MaxDCount, size_t maxACount = UniSetUDP::MaxACount );

		virtual ~UNetSender();

		typedef int sendfactor_t;

		struct UItem
		{
			UItem():
				iotype(UniversalIO::UnknownIOType),
				id(UniSetTypes::DefaultObjectId),
				pack_num(-1),
				pack_ind(-1),
				pack_sendfactor(0) {}

			UniversalIO::IOType iotype;
			UniSetTypes::ObjectId id;
			IOController::IOStateList::iterator ioit;
			int pack_num;
			int pack_ind;
			sendfactor_t pack_sendfactor = { 0 };

			friend std::ostream& operator<<( std::ostream& os, UItem& p );
		};

		typedef std::vector<UItem> DMap;

		int getDataPackCount();

		void start();
		void stop();

		void send();
		void real_send(UniSetUDP::UDPMessage& mypack);

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
		inline void setPackSendPause( int msec )
		{
			packsendpause = msec;
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

		inline size_t getADataSize()
		{
			return maxAData;
		}
		inline size_t getDDataSize()
		{
			return maxDData;
		}

	protected:

		std::string s_field;
		std::string s_fvalue;
		std::string prefix;

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
		int packsendpause;
		std::atomic_bool activated;

		UniSetTypes::uniset_rwmutex pack_mutex;

		// int - sendfactor
		typedef std::unordered_map<sendfactor_t, std::vector<UniSetUDP::UDPMessage>> Packs;

		Packs mypacks;
		std::unordered_map<sendfactor_t, int> packs_anum;
		std::unordered_map<sendfactor_t, int> packs_dnum;
		DMap dlist;
		int maxItem;
		unsigned long packetnum; /*!< номер очередного посылаемого пакета */
		unsigned short lastcrc;
		UniSetUDP::UDPPacket s_msg;

		size_t maxAData;
		size_t maxDData;

		std::shared_ptr< ThreadCreator<UNetSender> > s_thr;    // send thread

		unsigned long ncycle = { 0 }; /*!< номер цикла посылки */

};
// -----------------------------------------------------------------------------
#endif // UNetSender_H_
// -----------------------------------------------------------------------------
