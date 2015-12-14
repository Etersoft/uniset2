#ifndef PassiveLProcessor_H_
#define PassiveLProcessor_H_
// --------------------------------------------------------------------------
#include <map>
#include <memory>
#include "UniSetTypes.h"
#include "UniSetObject.h"
#include "Extensions.h"
#include "SharedMemory.h"
#include "UInterface.h"
#include "SMInterface.h"
#include "LProcessor.h"
// --------------------------------------------------------------------------
/*! Реализация LogicProccessor основанная на заказе датчиков */
class PassiveLProcessor:
	public UniSetObject,
	protected LProcessor
{
	public:

		PassiveLProcessor(UniSetTypes::ObjectId objId,
						   UniSetTypes::ObjectId shmID, const std::shared_ptr<SharedMemory>& ic = nullptr, const std::string& prefix = "lproc" );
		virtual ~PassiveLProcessor();

		enum Timers
		{
			tidStep
		};

		static void help_print( int argc, const char* const* argv );

		static std::shared_ptr<PassiveLProcessor> init_plproc( int argc, const char* const* argv,
				UniSetTypes::ObjectId shmID, const std::shared_ptr<SharedMemory>& ic = nullptr,
				const std::string& prefix = "plproc" );

	protected:
		PassiveLProcessor(): shm(0), maxHeartBeat(0) {};

		virtual void step();
		virtual void getInputs();
		virtual void setOuts();

		void sysCommand( const UniSetTypes::SystemMessage* msg ) override;
		void sensorInfo( const UniSetTypes::SensorMessage* sm ) override;
		void timerInfo( const UniSetTypes::TimerMessage* tm ) override;
		void askSensors( const UniversalIO::UIOCommand cmd );
		//        void initOutput();

		// действия при завершении работы
		virtual void sigterm( int signo ) override;
		void initIterators();
		virtual bool activateObject() override;

		std::shared_ptr<SMInterface> shm;

	private:
		PassiveTimer ptHeartBeat;
		UniSetTypes::ObjectId sidHeartBeat = { UniSetTypes::DefaultObjectId };
		int maxHeartBeat = { 10 };
		IOController::IOStateList::iterator itHeartBeat;
		UniSetTypes::uniset_mutex mutex_start;
};
// ---------------------------------------------------------------------------
#endif
