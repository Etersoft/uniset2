#ifndef PassiveLProcessor_H_
#define PassiveLProcessor_H_
// --------------------------------------------------------------------------
#include <map>
#include "UniSetTypes.h"
#include "UniSetObject_LT.h"
#include "Extensions.h"
#include "SharedMemory.h"
#include "UniversalInterface.h"
#include "SMInterface.h"
#include "LProcessor.h"
// --------------------------------------------------------------------------
/*! Реализация LogicProccessor основанная на заказе датчиков */
class PassiveLProcessor:
	public UniSetObject_LT,
	protected LProcessor
{
	public:

		PassiveLProcessor( std::string schema, UniSetTypes::ObjectId objId,
							UniSetTypes::ObjectId shmID, SharedMemory* ic=0, const std::string& prefix="lproc" );
	    virtual ~PassiveLProcessor();

		enum Timers
		{
			tidStep
		};

	protected:
		PassiveLProcessor():shm(0),maxHeartBeat(0){};

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
		void initIterators();
		virtual bool activateObject();

		SMInterface* shm;

	private:
		PassiveTimer ptHeartBeat;
		UniSetTypes::ObjectId sidHeartBeat;
		int maxHeartBeat;
		IOController::AIOStateList::iterator aitHeartBeat;
		UniSetTypes::uniset_mutex mutex_start;
};
// ---------------------------------------------------------------------------
#endif
