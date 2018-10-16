/*
 * Copyright (c) 2015 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Lesser Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
// --------------------------------------------------------------------------
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
namespace uniset
{
	// -------------------------------------------------------------------------
	/*! Реализация LogicProccessor основанная на заказе датчиков */
	class PassiveLProcessor:
		public UniSetObject,
		protected LProcessor
	{
		public:

			PassiveLProcessor(uniset::ObjectId objId,
							  uniset::ObjectId shmID, const std::shared_ptr<SharedMemory>& ic = nullptr, const std::string& prefix = "lproc" );
			virtual ~PassiveLProcessor();

			enum Timers
			{
				tidStep
			};

			static void help_print( int argc, const char* const* argv );

			static std::shared_ptr<PassiveLProcessor> init_plproc( int argc, const char* const* argv,
					uniset::ObjectId shmID, const std::shared_ptr<SharedMemory>& ic = nullptr,
					const std::string& prefix = "plproc" );

		protected:
			PassiveLProcessor(): shm(0), maxHeartBeat(0) {};

			virtual void step() override;
			virtual void getInputs() override;
			virtual void setOuts() override;

			void sysCommand( const uniset::SystemMessage* msg ) override;
			void sensorInfo( const uniset::SensorMessage* sm ) override;
			void timerInfo( const uniset::TimerMessage* tm ) override;
			void askSensors( const UniversalIO::UIOCommand cmd );
			//        void initOutput();

			void initIterators();
			virtual bool activateObject() override;
			virtual bool deactivateObject() override;

			std::shared_ptr<SMInterface> shm;

		private:
			PassiveTimer ptHeartBeat;
			uniset::ObjectId sidHeartBeat = { uniset::DefaultObjectId };
			int maxHeartBeat = { 10 };
			IOController::IOStateList::iterator itHeartBeat;
			std::mutex mutex_start;
			std::atomic_bool cannceled = { false };
	};
	// --------------------------------------------------------------------------
} // end of namespace uniset
// ---------------------------------------------------------------------------
#endif
