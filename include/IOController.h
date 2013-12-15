/* This file is part of the UniSet project
 * Copyright (c) 2002 Free Software Foundation, Inc.
 * Copyright (c) 2002 Pavel Vainerman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
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
#include <map>
#include <list>
#include <sigc++/sigc++.h>
#include "IOController_i.hh"
#include "UniSetTypes.h"
#include "ObjectsManager.h"
#include "Configuration.h"
#include "Mutex.h"
//---------------------------------------------------------------------------
/*! Реализация интерфейса IOController-а */ 
class IOController: 
		public ObjectsManager,
		public POA_IOController_i
{
	public:
	
		IOController(const std::string name, const std::string section);
		IOController(UniSetTypes::ObjectId id);
		~IOController();

		virtual UniSetTypes::ObjectType getType(){ return UniSetTypes::getObjectType("IOController"); }

	  	virtual CORBA::Long getValue( const IOController_i::SensorInfo& si );	

//     -------------------- !!!!!!!!! ---------------------------------
//		Реализуются конкретным i/o контроллером
//		Не забывайте писать реализацию этих функций
		virtual void setValue( const IOController_i::SensorInfo& si, CORBA::Long value,
								UniSetTypes::ObjectId sup_id = UniSetTypes::DefaultObjectId );

		virtual void fastSetValue( const IOController_i::SensorInfo& si, CORBA::Long value,
								UniSetTypes::ObjectId sup_id = UniSetTypes::DefaultObjectId );

//     ----------------------------------------------------------------
		virtual void setUndefinedState(const IOController_i::SensorInfo& si, 
										CORBA::Boolean undefined, 
										UniSetTypes::ObjectId sup_id = UniSetTypes::DefaultObjectId );


		virtual IOController_i::SensorInfoSeq* getSensorSeq(const UniSetTypes::IDSeq& lst);
		virtual UniSetTypes::IDSeq* setOutputSeq(const IOController_i::OutSeq& lst, UniSetTypes::ObjectId sup_id);

//     ----------------------------------------------------------------
		virtual UniversalIO::IOType getIOType(const IOController_i::SensorInfo& si);

		virtual IOController_i::SensorInfoSeq* getSensorsMap();
		virtual IOController_i::SensorIOInfo getSensorIOInfo(const IOController_i::SensorInfo& si);

		virtual CORBA::Long getRawValue(const IOController_i::SensorInfo& si);
		virtual void calibrate(const IOController_i::SensorInfo& si, 
									const IOController_i::CalibrateInfo& ci,
									UniSetTypes::ObjectId adminId );
		
		IOController_i::CalibrateInfo getCalibrateInfo(const IOController_i::SensorInfo& si);

		inline IOController_i::SensorInfo SensorInfo(UniSetTypes::ObjectId id, 
								UniSetTypes::ObjectId node=UniSetTypes::conf->getLocalNode())
		{
			IOController_i::SensorInfo si;
			si.id = id;
			si.node = node;
			return si;
		};

		UniSetTypes::Message::Priority getPriority(const IOController_i::SensorInfo& si, 
													UniversalIO::IOType type);
													
		virtual IOController_i::ShortIOInfo getChangedTime(const IOController_i::SensorInfo& si);

		virtual IOController_i::ShortMapSeq* getSensors();

	public:

		typedef sigc::signal<void,const IOController_i::SensorInfo&, long, IOController*> ChangeSignal;

		// signal по изменению определённого датчика
		ChangeSignal signal_change_value( UniSetTypes::ObjectId id, UniSetTypes::ObjectId node );
		ChangeSignal signal_change_value( const IOController_i::SensorInfo& si );

		// предварительное объявление, чтобы в структуре объявить итератор..
		struct USensorIOInfo;
		typedef std::map<UniSetTypes::KeyType, USensorIOInfo> IOStateList;

		struct USensorIOInfo:
			public IOController_i::SensorIOInfo
		{
			USensorIOInfo():any(0),db_ignore(false),d_value(0),d_off_value(0)
			{
				undefined = false;
				d_si.id = UniSetTypes::DefaultObjectId;
				d_si.node = UniSetTypes::DefaultObjectId;
			}

			virtual ~USensorIOInfo(){}

			USensorIOInfo(IOController_i::SensorIOInfo& r);
			USensorIOInfo(IOController_i::SensorIOInfo* r);
			USensorIOInfo(const IOController_i::SensorIOInfo& r);

			USensorIOInfo& operator=(IOController_i::SensorIOInfo& r);
			const USensorIOInfo& operator=(const IOController_i::SensorIOInfo& r);
			USensorIOInfo& operator=(IOController_i::SensorIOInfo* r);

			// Дополнительные (вспомогательные поля)
			UniSetTypes::uniset_rwmutex val_lock; /*!< флаг блокирующий работу со значением */
		
			IOStateList::iterator it;

			void* any; 			/*!< расширение для возможности хранения своей информации */
            bool db_ignore;		/*!< не писать изменения в БД */

			// сигнал для реализации механизма зависимостией..
			ChangeSignal changeSignal;

			IOController_i::SensorInfo d_si;  /*!< идентификатор датчика, от которого зависит данный */
			long d_value; /*!< разрешающее работу значение датчика от которого зависит данный */
			long d_off_value; /*!< блокирующее значение */

			void checkDepend( const IOController_i::SensorInfo& si , long newval, IOController* );
		};

		inline IOStateList::iterator ioBegin(){ return ioList.begin(); }
		inline IOStateList::iterator ioEnd(){ return ioList.end(); }
		inline IOStateList::iterator find(UniSetTypes::KeyType k){ return ioList.find(k); }
		inline int ioCount(){ return ioList.size(); }

		// доступ к элементам через итератор
		virtual void localSetValue( IOStateList::iterator& it, const IOController_i::SensorInfo& si,
										CORBA::Long value, UniSetTypes::ObjectId sup_id );

		virtual long localGetValue( IOStateList::iterator& it, const IOController_i::SensorInfo& si );
		

		/*! функция выставления признака неопределённого состояния для аналоговых датчиков 
			// для дискретных датчиков необходимости для подобной функции нет.
			// см. логику выставления в функции localSaveState
		*/
		virtual void localSetUndefinedState( IOStateList::iterator& it, bool undefined,
												const IOController_i::SensorInfo& si );

	protected:
			// переопределяем для добавления вызова регистрации датчиков
			virtual bool disactivateObject();
			virtual bool activateObject();

			/*! регистрация датчиков, за информацию о которых отвечает данный IOController */
		    virtual void sensorsRegistration(){};
			/*! удаление из репозитория датчиков за информацию о которых отвечает данный IOController */
			virtual void sensorsUnregistration();
	
			/*! регистрация датчика
				force=true - не проверять на дублирование (оптимизация)
			*/
			void ioRegistration( const USensorIOInfo&, bool force=false );

			/*! разрегистрация датчика */
			void ioUnRegistration( const IOController_i::SensorInfo& si );
			
			UniSetTypes::Message::Priority getMessagePriority(UniSetTypes::KeyType k, UniversalIO::IOType type);
			
			// ------------------------------
			inline IOController_i::SensorIOInfo
				SensorIOInfo(long v, UniversalIO::IOType t, const IOController_i::SensorInfo& si,
								UniSetTypes::Message::Priority p = UniSetTypes::Message::Medium,
								long defval=0, IOController_i::CalibrateInfo* ci=0 )
			{
				IOController_i::SensorIOInfo ai;
				ai.si = si;
				ai.type = t;
				ai.value = v;
				ai.priority = p;
				ai.default_val = defval;
				ai.real_value = v;
				ai.blocked = false;
				if( ci!=0 )
					ai.ci = *ci;
				else
				{
					ai.ci.minRaw = 0;
					ai.ci.maxRaw = 0;
					ai.ci.minCal = 0;
					ai.ci.maxCal = 0;
					ai.ci.sensibility = 0;
					ai.ci.precision = 0;
				}
				return ai;	
			};

			//! сохранение информации об изменении состояния датчика
			virtual void logging(UniSetTypes::SensorMessage& sm);
			
			//! сохранение состояния всех датчиков в БД
			virtual void dumpToDB();

		IOController();	

		// доступ к списку c изменением только для своих
		IOStateList::iterator myioBegin();
		IOStateList::iterator myioEnd();
		IOStateList::iterator myiofind(UniSetTypes::KeyType k);
		// --------------------------
		// ФИЛЬТРОВАНИЕ
		// 
		typedef sigc::slot<bool,const USensorIOInfo&, CORBA::Long, UniSetTypes::ObjectId> IOFilterSlot;
		typedef std::list<IOFilterSlot> IOFilterSlotList;

		/*
			Фильтрующая функция должна возвращать:
			TRUE - если значение 'нормальное'
			FALSE - если значение не подходит (отбрасывается)
		
			Пример использования: 
				addIOFilter( sigc::mem_fun(my,&MyClass::my_filter) );
		*/
		IOFilterSlotList::iterator addIOFilter( IOFilterSlot sl, bool push_front=false );
		void eraseIOFilter(IOFilterSlotList::iterator& it);

		// функии проверки текущего значения
		bool checkIOFilters( const USensorIOInfo& ai, CORBA::Long& newvalue, UniSetTypes::ObjectId sup_id );

		inline bool iofiltersEmpty(){ return iofilters.empty(); }
		inline int iodiltersSize(){ return iofilters.size(); }

	private:		
		friend class NCRestorer;
	
		IOStateList ioList;	/*!< список с текущим состоянием аналоговых входов/выходов */
		UniSetTypes::uniset_rwmutex ioMutex; /*!< замок для блокирования совместного доступа к ioList */
		
		bool isPingDBServer;	// флаг связи с DBServer-ом 

		IOFilterSlotList iofilters; /*!< список фильтров для аналоговых значений */

		UniSetTypes::uniset_rwmutex loggingMutex; /*!< logging info mutex */
};
// --------------------------------------------------------------------------
#endif
// --------------------------------------------------------------------------
