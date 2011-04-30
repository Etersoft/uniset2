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
/*!
	\page pageUNetExchange2 Сетевой обмен на основе UDP (UNet2)

	\par Обмен построен  на основе протокола UDP.
		Основная идея заключается в том, что каждый узел на порту равном своему ID
	посылает в сеть UDP-пакеты содержащие данные считанные из локальной SM. Формат данных - это набор
	пар "id - value". Другие узлы принимают их. Помимо этого процесс, данный процесс запускает
	по потоку приёма для каждого другого узла и ловит пакеты от них, сохраняя данные в SM.

	\par При своём старте процесс считывает из секции <nodes> список узлов с которыми необходимо вести обмен.
		Открывает по потоку приёма на каждый узел и поток передачи для своих данных. А так же параметры
	своего узла.

	\par Пример конфигурирования
		По умолчанию при считывании используются свойства \a ip и \a id - в качестве порта.
	Но можно переопределять эти параметры, при помощи указания \a unet_port и/или \a unet_ip.
	\code
	<nodes port="2809">
		<item ip="127.0.0.1" name="LocalhostNode" textname="Локальный узел" unet_ignore="1" unet_port="3000" unet_ip="192.168.56.1">
		<iocards>
			...
		</iocards>
		</item>
		<item ip="192.168.56.10" name="Node1" textname="Node1" unet_port="3001" unet_ip="192.168.56.2"/>
		<item ip="192.168.56.11" name="Node2" textname="Node2" unet_port="3002" unet_ip="192.168.56.3"/>
	</nodes>
	\endcode


*/
// -----------------------------------------------------------------------------
class UNetExchange:
	public UniSetObject_LT
{
	public:
		UNetExchange( UniSetTypes::ObjectId objId, UniSetTypes::ObjectId shmID, SharedMemory* ic=0 );
		virtual ~UNetExchange();

		/*! глобальная функция для инициализации объекта */
		static UNetExchange* init_unetexchange( int argc, const char* argv[],
											UniSetTypes::ObjectId shmID, SharedMemory* ic=0 );

		/*! глобальная функция для вывода help-а */
		static void help_print( int argc, const char* argv[] );

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
