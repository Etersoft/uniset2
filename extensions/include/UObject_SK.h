// -----------------------------------------------------------------------------
#ifndef UObject_SK_H_
#define UObject_SK_H_
// -----------------------------------------------------------------------------
#include "UniSetObject.h"
#include "LT_Object.h"
#include "UniXML.h"
#include "Trigger.h"
#include "SMInterface.h"
// -----------------------------------------------------------------------------
class UObject_SK:
	public UniSetObject,
	public LT_Object
{
	public:
		UObject_SK( UniSetTypes::ObjectId id, xmlNode* node=UniSetTypes::conf->getNode("UObject"),
					 UniSetTypes::ObjectId shmID = UniSetTypes::DefaultObjectId );
		UObject_SK();
		virtual ~UObject_SK();

		

		bool alarm( UniSetTypes::ObjectId sid, bool state );
		bool getState( UniSetTypes::ObjectId sid );
		void setValue( UniSetTypes::ObjectId sid, long value );
		void setState( UniSetTypes::ObjectId sid, bool state );
		void askState( UniSetTypes::ObjectId sid, UniversalIO::UIOCommand );
		void askValue( UniSetTypes::ObjectId sid, UniversalIO::UIOCommand );
		void askThreshold ( UniSetTypes::ObjectId sensorId, UniSetTypes::ThresholdId tid,
							UniversalIO::UIOCommand cmd,
							CORBA::Long lowLimit, CORBA::Long hiLimit, CORBA::Long sensibility,
							UniSetTypes::ObjectId backid = UniSetTypes::DefaultObjectId );

		void updateValues();
		void setInfo( UniSetTypes::ObjectId code, bool state );		

	protected:
		
		virtual void callback();
		virtual void processingMessage( UniSetTypes::VoidMessage* msg );
		virtual void sysCommand( UniSetTypes::SystemMessage* sm );
		virtual void askSensors( UniversalIO::UIOCommand cmd );
		virtual void sensorInfo( UniSetTypes::SensorMessage* sm ){};
		virtual void timerInfo( UniSetTypes::TimerMessage* tm ){};
		virtual void sigterm( int signo );
		virtual bool activateObject();
		virtual void testMode( bool state );
		void updatePreviousValues();
		void checkSensors();
		void updateOutputs( bool force );
		bool checkTestMode();
		void preSensorInfo( UniSetTypes::SensorMessage* sm );
		void preTimerInfo( UniSetTypes::TimerMessage* tm );
		void waitSM( int wait_msec );

		void resetMsg();
		Trigger trResetMsg;
		PassiveTimer ptResetMsg;
		int resetMsgTime;

		// Выполнение очередного шага программы
		virtual void step()=0;

		int sleep_msec; /*!< пауза между итерациями */
		bool active;
		bool isTestMode;
		Trigger trTestMode;
		UniSetTypes::ObjectId idTestMode_S;		  	/*!< идентификатор для флага тестовго режима (для всех) */
		UniSetTypes::ObjectId idLocalTestMode_S;	/*!< идентификатор для флага тестовго режима (для данного узла) */
		bool in_TestMode_S;
		bool in_LocalTestMode_S;
		
		xmlNode* confnode;
		SMInterface shm;
		int smReadyTimeout; 	/*!< время ожидания готовности SM */
		bool activated;
		int activateTimeout;	/*!< время ожидания готовности UniSetObject к работе */
		PassiveTimer ptStartUpTimeout;	/*!< время на блокировку обработки WatchDog, если недавно был StartUp */

		
	private:
		
		IOController_i::SensorInfo si;
};

// -----------------------------------------------------------------------------
#endif // UObject_SK_H_
