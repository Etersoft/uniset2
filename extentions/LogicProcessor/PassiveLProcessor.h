#ifndef PassiveLProcessor_H_
#define PassiveLProcessor_H_
// --------------------------------------------------------------------------
#include <map>
#include "UniSetTypes.h"
#include "UniSetObject_LT.h"
#include "Extentions.h"
#include "SharedMemory.h"
#include "UniversalInterface.h"
#include "SMInterface.h"
#include "LProcessor.h"
// --------------------------------------------------------------------------
class PassiveLProcessor:
	public UniSetObject_LT,
	protected LProcessor
{
	public:

		PassiveLProcessor( std::string schema, UniSetTypes::ObjectId objId, 
							UniSetTypes::ObjectId shmID, SharedMemory* ic=0 );
	    virtual ~PassiveLProcessor();

		enum Timers
		{
			tidStep
		};

	protected:
		PassiveLProcessor(){};

		virtual void step();
		virtual void getInputs();
		virtual void setOuts();

		virtual void processingMessage( UniSetTypes::VoidMessage *msg );
		void sysCommand( UniSetTypes::SystemMessage *msg );
		void sensorInfo( UniSetTypes::SensorMessage*sm );
		void timerInfo( UniSetTypes::TimerMessage *tm );
		void askSensors( UniversalIO::UIOCommand cmd );	
//		void initOutput();

		// действия при завершении работы
		virtual void sigterm( int signo );

		SMInterface* shm;

	private:
		PassiveTimer ptHeartBeat;
		UniSetTypes::ObjectId sidHeartBeat;
		int maxHeartBeat;
		IOController::AIOStateList::iterator aitHeartBeat;
};
// ---------------------------------------------------------------------------
#endif
