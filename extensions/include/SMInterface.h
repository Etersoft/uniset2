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
//--------------------------------------------------------------------------
#ifndef SMInterface_H_
#define SMInterface_H_
//--------------------------------------------------------------------------
#include <string>
#include <memory>
#include <atomic>
#include "UniSetTypes.h"
#include "Mutex.h"
#include "IONotifyController.h"
#include "UInterface.h"
// --------------------------------------------------------------------------
namespace uniset
{
	// --------------------------------------------------------------------------
	class SMInterface
	{
		public:

			SMInterface( uniset::ObjectId _shmID, const std::shared_ptr<UInterface>& ui,
						 uniset::ObjectId myid, const std::shared_ptr<IONotifyController> ic = nullptr );
			~SMInterface();

			void setValue ( uniset::ObjectId, long value );
			void setUndefinedState( const IOController_i::SensorInfo& si, bool undefined, uniset::ObjectId supplier );

			long getValue ( uniset::ObjectId id );

			void askSensor( uniset::ObjectId id, UniversalIO::UIOCommand cmd,
							uniset::ObjectId backid = uniset::DefaultObjectId );

			IOController_i::SensorInfoSeq* getSensorsMap();
			IONotifyController_i::ThresholdsListSeq* getThresholdsList();

			void localSetValue( IOController::IOStateList::iterator& it,
								uniset::ObjectId sid,
								CORBA::Long newvalue, uniset::ObjectId sup_id );

			long localGetValue( IOController::IOStateList::iterator& it,
								uniset::ObjectId sid );

			/*! функция выставления признака неопределённого состояния для аналоговых датчиков
			    // для дискретных датчиков необходимости для подобной функции нет.
			    // см. логику выставления в функции localSaveState
			*/
			void localSetUndefinedState( IOController::IOStateList::iterator& it,
										 bool undefined, uniset::ObjectId sid );

			// специальные функции
			IOController::IOStateList::iterator ioEnd();
			void initIterator( IOController::IOStateList::iterator& it );

			bool exist();
			bool waitSMready( int msec, int pause = 5000 );
			bool waitSMworking( uniset::ObjectId, int msec, int pause = 3000 );
			bool waitSMreadyWithCancellation( int msec, std::atomic_bool& cancelFlag, int pause = 5000 );

			inline bool isLocalwork() const noexcept
			{
				return (ic == NULL);
			}
			inline uniset::ObjectId ID() const noexcept
			{
				return myid;
			}
			inline const std::shared_ptr<IONotifyController> SM() noexcept
			{
				return ic;
			}
			inline uniset::ObjectId getSMID() const noexcept
			{
				return shmID;
			}

#ifndef DISABLE_REST_API
			std::string apiRequest( const std::string& query );
#endif

		protected:
			const std::shared_ptr<IONotifyController> ic;
			const std::shared_ptr<UInterface> ui;
			CORBA::Object_var oref;
			uniset::ObjectId shmID;
			uniset::ObjectId myid;
			uniset::uniset_rwmutex shmMutex;
	};
	// --------------------------------------------------------------------------
} // end of namespace uniset
//--------------------------------------------------------------------------
#endif
