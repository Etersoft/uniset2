/* $Id: SharedMemory.h,v 1.2 2009/01/22 02:11:24 vpashka Exp $ */
// -----------------------------------------------------------------------------
#ifndef SharedMemory_H_
#define SharedMemory_H_
// -----------------------------------------------------------------------------
#include <string>
#include <list>
#include "IONotifyController_LT.h"
#include "Mutex.h"
#include "PassiveTimer.h"
#include "NCRestorer.h"
#include "WDTInterface.h"
// -----------------------------------------------------------------------------

/*! Реализация разделямой между процессами памяти 
	Помимо функции хранения, осуществляет логику слежения
	за процессами (инкремент специальных датчиков),
	а также выставление дискретных датчиков "живости" процессов.
*/
class SharedMemory:
	public IONotifyController_LT
{
	public:
		SharedMemory( UniSetTypes::ObjectId id, std::string datafile );
		virtual ~SharedMemory();

		/*! глобальная функция для инициализации объекта */
		static SharedMemory* init_smemory( int argc, char* argv[] );
		/*! глобальная функция для вывода help-а */
		static void help_print( int argc, char* argv[] );


	    virtual void saveValue(const IOController_i::SensorInfo& si, CORBA::Long value,
								UniversalIO::IOTypes type = UniversalIO::AnalogInput,
								UniSetTypes::ObjectId sup_id = UniSetTypes::DefaultObjectId );

	    virtual void fastSaveValue(const IOController_i::SensorInfo& si, CORBA::Long value,
								UniversalIO::IOTypes type = UniversalIO::AnalogInput,
								UniSetTypes::ObjectId sup_id = UniSetTypes::DefaultObjectId );


		// функция определяет "готовность" SM к работе.
		// должна использоваться другими процессами, для того, 
		// чтобы понять, когда можно получать от SM данные.
		virtual CORBA::Boolean exist();

		void addReadItem( Restorer_XML::ReaderSlot sl );
		
	protected:
		typedef std::list<Restorer_XML::ReaderSlot> ReadSlotList;
		ReadSlotList lstRSlot;

		virtual void processingMessage( UniSetTypes::VoidMessage *msg );
		void sysCommand( UniSetTypes::SystemMessage *sm );
		void sensorInfo( UniSetTypes::SensorMessage *sm );
		void timerInfo( UniSetTypes::TimerMessage *tm );
		void askSensors( UniversalIO::UIOCommand cmd );
		void sendEvent( UniSetTypes::SystemMessage& sm );

		// действия при завершении работы
		virtual void sigterm( int signo );
		bool activateObject();
//		virtual void logging(UniSetTypes::SensorMessage& sm){}
//		virtual void dumpToDB(){}
		bool readItem( UniXML& xml, UniXML_iterator& it, xmlNode* sec );

	
		void buildEventList( xmlNode* cnode );
		void readEventList( std::string oname );
		
		UniSetTypes::uniset_mutex mutex_start;
		
		struct HeartBeatInfo
		{
			HeartBeatInfo():
				a_sid(UniSetTypes::DefaultObjectId),
				d_sid(UniSetTypes::DefaultObjectId),
				reboot_msec(UniSetTimer::WaitUpTime),
				timer_running(false),
				ptReboot(UniSetTimer::WaitUpTime)
			{}
			
			UniSetTypes::ObjectId a_sid; // аналоговый счётчик
			UniSetTypes::ObjectId d_sid; // дискретный датчик состояния процесса
			AIOStateList::iterator ait;
			DIOStateList::iterator dit;

			int reboot_msec; /*!< Время в течение которого, процесс обязан подтвердить своё существование,
								иначе будет произведена перезагрузка контроллера по WDT (в случае если он включён).
								Если данный параметр не указывать, то "не живость" процесса просто игнорируется
								(т.е. только отображение на GUI). */

			bool timer_running;
			PassiveTimer ptReboot;
		};

		enum Timers
		{
			tmHeartBeatCheck,
			tmEvent
		};
		
		int heartbeatCheckTime;
		std::string heartbeat_node;
		
		void checkHeartBeat();

		typedef std::list<HeartBeatInfo> HeartBeatList;
		HeartBeatList hlist; // список датчиков "сердцебиения"
		UniSetTypes::uniset_mutex hbmutex;
		WDTInterface* wdt;
		bool activated;
		bool workready;

		typedef std::list<UniSetTypes::ObjectId> EventList;
		EventList elst;
		std::string e_filter;
		int evntPause;
		int activateTimeout;

		virtual void loggingInfo(UniSetTypes::SensorMessage& sm);
		virtual void dumpOrdersList(const IOController_i::SensorInfo& si, const IONotifyController::ConsumerList& lst){}
		virtual void dumpThresholdList(const IOController_i::SensorInfo& si, const IONotifyController::ThresholdExtList& lst){}

		bool dblogging;
	
	private:

};
// -----------------------------------------------------------------------------
#endif // SharedMemory_H_
// -----------------------------------------------------------------------------
