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
/*! \file
 * \brief Реализация IOController_i
 * \author Pavel Vainerman
*/
// --------------------------------------------------------------------------
#ifndef IOController_H_
#define IOController_H_
//---------------------------------------------------------------------------
#include <unordered_map>
#include <list>
#include <limits>
#include <sigc++/sigc++.h>
#include "IOController_i.hh"
#include "UniSetTypes.h"
#include "UniSetManager.h"
#include "Configuration.h"
#include "Mutex.h"
//---------------------------------------------------------------------------
namespace uniset
{
	/*! Реализация интерфейса IOController-а
	 * Важной особенностью данной реализации является то, что
	 * список входов/выходов (ioList) формируется один раз во время создания объекта
	 * и не меняется (!) в процессе работы. На этом построены некоторые оптимизации!
	 * Поэтому неизменность ioList во время всей жизни объекта должна гарантироваться.
	 * В частности, очень важной является структура USensorInfo, а также userdata,
	 * которые используются для "кэширования" (сохранения) указателей на специальные данные.
	 * (см. также IONotifyContoller).
	*/
	class IOController:
		public UniSetManager,
		public POA_IOController_i
	{
		public:

			IOController( const std::string& name, const std::string& section );
			IOController( const uniset::ObjectId id );
			virtual ~IOController();

			virtual uniset::ObjectType getType() override
			{
				return uniset::ObjectType("IOController");
			}

			virtual uniset::SimpleInfo* getInfo( const char* userparam = "" ) override;

			// ----------------------------------------------------------------
			// Публичный (IDL) интерфейс IOController_i
			// ----------------------------------------------------------------

			virtual CORBA::Long getValue( uniset::ObjectId sid ) override;

			virtual void setValue( uniset::ObjectId sid, CORBA::Long value,
								   uniset::ObjectId sup_id = uniset::DefaultObjectId ) override;
			virtual void setUndefinedState( uniset::ObjectId sid,
											CORBA::Boolean undefined,
											uniset::ObjectId sup_id = uniset::DefaultObjectId ) override;


			virtual IOController_i::SensorInfoSeq* getSensorSeq( const uniset::IDSeq& lst ) override;
			virtual uniset::IDSeq* setOutputSeq( const IOController_i::OutSeq& lst, uniset::ObjectId sup_id ) override;

			//     ----------------------------------------------------------------
			virtual UniversalIO::IOType getIOType( uniset::ObjectId sid ) override;

			virtual IOController_i::SensorInfoSeq* getSensorsMap() override;
			virtual IOController_i::SensorIOInfo getSensorIOInfo( uniset::ObjectId sid ) override;

			virtual CORBA::Long getRawValue( uniset::ObjectId sid ) override;
			virtual void calibrate( uniset::ObjectId sid,
									const IOController_i::CalibrateInfo& ci,
									uniset::ObjectId adminId ) override;

			IOController_i::CalibrateInfo getCalibrateInfo( uniset::ObjectId sid ) override;

			inline IOController_i::SensorInfo SensorInfo( const uniset::ObjectId sid,
					const uniset::ObjectId node = uniset::uniset_conf()->getLocalNode())
			{
				IOController_i::SensorInfo si;
				si.id = sid;
				si.node = node;
				return si;
			};

			uniset::Message::Priority getPriority( const uniset::ObjectId id );

			virtual IOController_i::ShortIOInfo getTimeChange( const uniset::ObjectId id ) override;

			virtual IOController_i::ShortMapSeq* getSensors() override;

#ifndef DISABLE_REST_API
			// http API
			virtual Poco::JSON::Object::Ptr httpHelp( const Poco::URI::QueryParameters& p ) override;
			virtual Poco::JSON::Object::Ptr httpRequest( const std::string& req, const Poco::URI::QueryParameters& p ) override;
#endif

		public:

			// предварительное объявление..
			struct USensorInfo;
			typedef std::unordered_map<uniset::ObjectId, std::shared_ptr<USensorInfo>> IOStateList;

			static const long not_specified_value = { std::numeric_limits<long>::max() };

			// ================== Достпуные сигналы =================
			/*!
			// \warning  В сигнале напрямую передаётся указатель на внутреннюю структуру!
			// Это не очень хорошо, с точки зрения "архитектуры", но оптимальнее по быстродействию!
			// необходимо в обработчике не забывать использовать uniset_rwmutex_wrlock(val_lock) или uniset_rwmutex_rlock(val_lock)
			*/
			typedef sigc::signal<void, std::shared_ptr<USensorInfo>&, IOController*> ChangeSignal;
			typedef sigc::signal<void, std::shared_ptr<USensorInfo>&, IOController*> ChangeUndefinedStateSignal;

			// signal по изменению определённого датчика
			ChangeSignal signal_change_value( uniset::ObjectId sid );

			// signal по изменению любого датчика
			ChangeSignal signal_change_value();

			// сигналы по изменению флага "неопределённое состояние" (обрыв датчика например)
			ChangeUndefinedStateSignal signal_change_undefined_state( uniset::ObjectId sid );
			ChangeUndefinedStateSignal signal_change_undefined_state();
			// -----------------------------------------------------------------------------------------
			// полнейшее нарушение икапсуляции
			// но пока, это попытка оптимизировать работу с IOController через указатель.
			// Т.е. работая с датчиками через итераторы..
#if 1
			inline IOStateList::iterator ioBegin()
			{
				return ioList.begin();
			}
			inline IOStateList::iterator ioEnd()
			{
				return ioList.end();
			}
			inline IOStateList::iterator find( uniset::ObjectId k )
			{
				return ioList.find(k);
			}
#endif
			inline int ioCount() const noexcept
			{
				return ioList.size();
			}

		protected:

			// доступ к элементам через итератор
			// return итоговое значение
			virtual long localSetValueIt( IOStateList::iterator& it, const uniset::ObjectId sid,
										  CORBA::Long value, uniset::ObjectId sup_id );

			virtual long localGetValue( IOStateList::iterator& it, const uniset::ObjectId sid );

			/*! функция выставления признака неопределённого состояния для аналоговых датчиков
				// для дискретных датчиков необходимости для подобной функции нет.
				// см. логику выставления в функции localSaveState
			*/
			virtual void localSetUndefinedState( IOStateList::iterator& it, bool undefined,
												 const uniset::ObjectId sid );

			// -- работа через указатель ---
			virtual long localSetValue( std::shared_ptr<USensorInfo>& usi, CORBA::Long value, uniset::ObjectId sup_id );
			long localGetValue( std::shared_ptr<USensorInfo>& usi) ;

#ifndef DISABLE_REST_API
			// http API
			virtual Poco::JSON::Object::Ptr request_get( const std::string& req, const Poco::URI::QueryParameters& p );
			virtual Poco::JSON::Object::Ptr request_sensors( const std::string& req, const Poco::URI::QueryParameters& p );
			void getSensorInfo( Poco::JSON::Array::Ptr& jdata, std::shared_ptr<USensorInfo>& s , bool shortInfo = false );
#endif

			// переопределяем для добавления вызова регистрации датчиков
			virtual bool deactivateObject() override;
			virtual bool activateObject() override;

			/*! Начальная инициализация (выставление значений) */
			virtual void activateInit();

			/*! регистрация датчиков, за информацию о которых отвечает данный IOController */
			virtual void sensorsRegistration() {};
			/*! удаление из репозитория датчиков за информацию о которых отвечает данный IOController */
			virtual void sensorsUnregistration();

			typedef sigc::signal<void, std::shared_ptr<USensorInfo>&, IOController*> InitSignal;

			// signal по изменению определённого датчика
			InitSignal signal_init();

			/*! регистрация датчика в репозитории */
			void ioRegistration(std::shared_ptr<USensorInfo>& usi );

			/*! разрегистрация датчика */
			void ioUnRegistration( const uniset::ObjectId sid );

			// ------------------------------
			inline IOController_i::SensorIOInfo
			SensorIOInfo(long v, UniversalIO::IOType t, const IOController_i::SensorInfo& si,
						 uniset::Message::Priority p = uniset::Message::Medium,
						 long defval = 0, IOController_i::CalibrateInfo* ci = 0,
						 uniset::ObjectId sup_id = uniset::DefaultObjectId )
			{
				IOController_i::SensorIOInfo ai;
				ai.si = si;
				ai.type = t;
				ai.value = v;
				ai.priority = p;
				ai.default_val = defval;
				ai.real_value = v;
				ai.blocked = false;
				ai.supplier = sup_id;

				if( ci != 0 )
					ai.ci = *ci;
				else
				{
					ai.ci.minRaw = 0;
					ai.ci.maxRaw = 0;
					ai.ci.minCal = 0;
					ai.ci.maxCal = 0;
					ai.ci.precision = 0;
				}

				return ai;
			};

			//! сохранение информации об изменении состояния датчика
			virtual void logging( uniset::SensorMessage& sm );

			//! сохранение состояния всех датчиков в БД
			virtual void dumpToDB();

			IOController();

			// доступ к списку c изменением только для своих
			IOStateList::iterator myioBegin();
			IOStateList::iterator myioEnd();
			IOStateList::iterator myiofind( uniset::ObjectId id );

			void initIOList( const IOStateList&& l );

			typedef std::function<void(std::shared_ptr<USensorInfo>&)> UFunction;
			// функция работает с mutex
			void for_iolist( UFunction f );

		private:
			friend class NCRestorer;
			friend class SMInterface;

			std::mutex siganyMutex;
			ChangeSignal sigAnyChange;

			std::mutex siganyundefMutex;
			ChangeSignal sigAnyUndefChange;
			InitSignal sigInit;

			IOStateList ioList;    /*!< список с текущим состоянием аналоговых входов/выходов */
			uniset::uniset_rwmutex ioMutex; /*!< замок для блокирования совместного доступа к ioList */

			bool isPingDBServer;    // флаг связи с DBServer-ом
			uniset::ObjectId dbserverID = { uniset::DefaultObjectId };

			std::mutex loggingMutex; /*!< logging info mutex */

		public:

			struct UThresholdInfo;
			typedef std::list<std::shared_ptr<UThresholdInfo>> ThresholdExtList;

			struct USensorInfo:
				public IOController_i::SensorIOInfo
			{
				USensorInfo( const USensorInfo& ) = delete;
				const USensorInfo& operator=(const USensorInfo& ) = delete;
				USensorInfo( USensorInfo&& ) = default;
				USensorInfo& operator=(USensorInfo&& ) = default;

				USensorInfo();
				virtual ~USensorInfo() {}

				USensorInfo(IOController_i::SensorIOInfo& r);
				USensorInfo(IOController_i::SensorIOInfo* r);
				USensorInfo(const IOController_i::SensorIOInfo& r);

				USensorInfo& operator=(IOController_i::SensorIOInfo& r);
				const USensorInfo& operator=(const IOController_i::SensorIOInfo& r);
				USensorInfo& operator=(IOController_i::SensorIOInfo* r);

				// Дополнительные (вспомогательные поля)
				uniset::uniset_rwmutex val_lock; /*!< флаг блокирующий работу со значением */

				// userdata (универсальный, но небезопасный способ расширения информации связанной с датчиком)
				static const size_t MaxUserData = 4;
				void* userdata[MaxUserData] = { nullptr, nullptr, nullptr, nullptr }; /*!< расширение для возможности хранения своей информации */
				uniset::uniset_rwmutex userdata_lock; /*!< mutex для работы с userdata */

				void* getUserData( size_t index );
				void setUserData( size_t index, void* data );

				// сигнал для реализации механизма зависимостией..
				// (все зависимые датчики подключаются к нему (см. NCRestorer::init_depends_signals)
				uniset::uniset_rwmutex changeMutex;
				ChangeSignal sigChange;

				uniset::uniset_rwmutex undefMutex;
				ChangeUndefinedStateSignal sigUndefChange;

				IOController_i::SensorInfo d_si = { uniset::DefaultObjectId, uniset::DefaultObjectId };  /*!< идентификатор датчика, от которого зависит данный */
				long d_value = { 1 }; /*!< разрешающее работу значение датчика от которого зависит данный */
				long d_off_value = { 0 }; /*!< блокирующее значение */
				std::shared_ptr<USensorInfo> d_usi; // shared_ptr на датчик от которого зависит этот.

				// список пороговых датчиков для данного
				uniset::uniset_rwmutex tmut;
				ThresholdExtList thresholds;

				size_t nchanges = { 0 }; // количество изменений датчика

				long undef_value = { not_specified_value }; // значение для "неопределённого состояния датчика"

				// функция обработки информации об изменении состояния датчика, от которого зависит данный
				void checkDepend( std::shared_ptr<USensorInfo>& d_usi, IOController* );

				void init( const IOController_i::SensorIOInfo& s );

				inline IOController_i::SensorIOInfo makeSensorIOInfo()
				{
					uniset::uniset_rwmutex_rlock lock(val_lock);
					IOController_i::SensorIOInfo s(*this);
					return s;
				}

				inline uniset::SensorMessage makeSensorMessage( bool with_lock = false )
				{
					uniset::SensorMessage sm;
					sm.id           = si.id;
					sm.node         = si.node; // uniset_conf()->getLocalNode()?
					sm.sensor_type  = type;
					sm.priority     = (uniset::Message::Priority)priority;

					// лочим только изменяемые поля
					if( with_lock )
					{
						uniset::uniset_rwmutex_rlock lock(val_lock);
						sm.value        = value;
						sm.sm_tv.tv_sec    = tv_sec;
						sm.sm_tv.tv_nsec   = tv_nsec;
						sm.ci           = ci;
						sm.supplier     = supplier;
						sm.undefined    = undefined;
					}
					else
					{
						sm.value        = value;
						sm.sm_tv.tv_sec    = tv_sec;
						sm.sm_tv.tv_nsec   = tv_nsec;
						sm.ci           = ci;
						sm.supplier     = supplier;
						sm.undefined    = undefined;
					}

					return sm;
				}
			};

			/*! Информация о пороговом значении */
			struct UThresholdInfo:
				public IONotifyController_i::ThresholdInfo
			{
				UThresholdInfo( uniset::ThresholdId tid, CORBA::Long low, CORBA::Long hi, bool inv,
								uniset::ObjectId _sid = uniset::DefaultObjectId ):
					sid(_sid),
					invert(inv)
				{
					id       = tid;
					hilimit  = hi;
					lowlimit = low;
					state    = IONotifyController_i::NormalThreshold;
				}

				/*! идентификатор дискретного датчика связанного с данным порогом */
				uniset::ObjectId sid;

				/*! итератор в списке датчиков (для быстрого доступа) */
				IOController::IOStateList::iterator sit;

				/*! инверсная логика */
				bool invert;

				inline bool operator== ( const ThresholdInfo& r ) const
				{
					return ((id == r.id) &&
							(hilimit == r.hilimit) &&
							(lowlimit == r.lowlimit) &&
							(invert == r.invert) );
				}

				UThresholdInfo( const UThresholdInfo& ) = delete;
				UThresholdInfo& operator=( const UThresholdInfo& ) = delete;
				UThresholdInfo( UThresholdInfo&& ) = default;
				UThresholdInfo& operator=(UThresholdInfo&& ) = default;
			};
	};
	// -------------------------------------------------------------------------
} // end of uniset namespace
// --------------------------------------------------------------------------
#endif
// --------------------------------------------------------------------------
