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
 * \date $Date: 2008/11/29 21:24:25 $
 * \version $Id: IOController.h,v 1.28 2008/11/29 21:24:25 vpashka Exp $
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

		virtual CORBA::Boolean getState( const IOController_i::SensorInfo& si );
	  	virtual CORBA::Long getValue( const IOController_i::SensorInfo& si );	

//     -------------------- !!!!!!!!! ---------------------------------
//		Реализуются конкретным i/o контроллером
//		Не забывайте писать реализацию этих функций
	  	virtual void setState( const IOController_i::SensorInfo& si, CORBA::Boolean state,
								UniSetTypes::ObjectId sup_id = UniSetTypes::DefaultObjectId );
		virtual void setValue( const IOController_i::SensorInfo& si, CORBA::Long value,
								UniSetTypes::ObjectId sup_id = UniSetTypes::DefaultObjectId );

	  	virtual void fastSetState( const IOController_i::SensorInfo& si, CORBA::Boolean state,
								UniSetTypes::ObjectId sup_id = UniSetTypes::DefaultObjectId );
		virtual void fastSetValue( const IOController_i::SensorInfo& si, CORBA::Long value,
								UniSetTypes::ObjectId sup_id = UniSetTypes::DefaultObjectId );

//     ----------------------------------------------------------------

		/*! \warning Не сделано проверки, зарегистрирован ли такой датчик */
		virtual void saveState(const IOController_i::SensorInfo& si, CORBA::Boolean state,
								UniversalIO::IOTypes type = UniversalIO::DigitalInput,
								UniSetTypes::ObjectId sup_id = UniSetTypes::DefaultObjectId );

		virtual void fastSaveState(const IOController_i::SensorInfo& si, CORBA::Boolean state,
								UniversalIO::IOTypes type = UniversalIO::DigitalInput,
								UniSetTypes::ObjectId sup_id = UniSetTypes::DefaultObjectId );

		/*! \warning Не сделано проверки, зарегистрирован ли такой датчик */
	    virtual void saveValue(const IOController_i::SensorInfo& si, CORBA::Long value,
								UniversalIO::IOTypes type = UniversalIO::AnalogInput,
								UniSetTypes::ObjectId sup_id = UniSetTypes::DefaultObjectId );

	    virtual void fastSaveValue(const IOController_i::SensorInfo& si, CORBA::Long value,
								UniversalIO::IOTypes type = UniversalIO::AnalogInput,
								UniSetTypes::ObjectId sup_id = UniSetTypes::DefaultObjectId );

		virtual void setUndefinedState(const IOController_i::SensorInfo& si, 
										CORBA::Boolean undefined, 
										UniSetTypes::ObjectId sup_id = UniSetTypes::DefaultObjectId );


		virtual IOController_i::ASensorInfoSeq* getSensorSeq(const UniSetTypes::IDSeq& lst);
		virtual UniSetTypes::IDSeq* setOutputSeq(const IOController_i::OutSeq& lst, UniSetTypes::ObjectId sup_id);

//     ----------------------------------------------------------------
		virtual UniversalIO::IOTypes getIOType(const IOController_i::SensorInfo& si);

		virtual IOController_i::ASensorInfoSeq* getAnalogSensorsMap();
		virtual IOController_i::DSensorInfoSeq* getDigitalSensorsMap();

		virtual IOController_i::DigitalIOInfo getDInfo(const IOController_i::SensorInfo& si);
		virtual IOController_i::AnalogIOInfo getAInfo(const IOController_i::SensorInfo& si);


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
													UniversalIO::IOTypes type);
													
		virtual IOController_i::ShortIOInfo getChangedTime(const IOController_i::SensorInfo& si);

		virtual IOController_i::ShortMapSeq* getSensors();

	public:

		struct DependsInfo;
		typedef std::list<DependsInfo> DependsList;

		/*! слот для подключения функции вызываемой при изменении состояния датчика 
			\param it 	- интератор из DependsList
			\param bool	- текущее состояние undefined (TRUE|FALSE)
		*/
		typedef sigc::slot<void,DependsList::iterator,bool> DependsSlot;
		
		/*! \warning В данной реализации call-back функция только одна!
			Потом можно будет перейти на список (типа AFilter и DFilter)
		*/
		void setDependsSlot( DependsSlot sl );
		void setBlockDependsSlot( DependsSlot sl );

		// структуры для внутреннего хранения информации по датчикам
		struct UniDigitalIOInfo:
			public IOController_i::DigitalIOInfo
		{
			UniDigitalIOInfo():any(0),dlst_lock(false),block_state(false)
				{ undefined = false; blocked=false; }
			virtual ~UniDigitalIOInfo(){}
			
			UniDigitalIOInfo(IOController_i::DigitalIOInfo& r);
			UniDigitalIOInfo(const IOController_i::DigitalIOInfo& r);
			UniDigitalIOInfo(IOController_i::DigitalIOInfo* r);

			UniDigitalIOInfo& operator=(IOController_i::DigitalIOInfo& r);
			UniDigitalIOInfo& operator=(IOController_i::DigitalIOInfo* r);
			const UniDigitalIOInfo& operator=(const IOController_i::DigitalIOInfo& r);
		
			void* any; 			/*!< расширение для возможности хранения своей информации */
			
			DependsList dlst; 	/*!< список io зависящих от данного */
			bool dlst_lock;		/*!< флаг блокирующий работу со списком */
			bool block_state;

			UniSetTypes::uniset_spin_mutex val_lock; /*!< флаг блокирующий работу со значением */
		};

		struct UniAnalogIOInfo:
			public IOController_i::AnalogIOInfo
		{
			UniAnalogIOInfo():any(0),dlst_lock(false),block_value(0)
				{ undefined = false; blocked=false; }
			virtual ~UniAnalogIOInfo(){}

			UniAnalogIOInfo(IOController_i::AnalogIOInfo& r);
			UniAnalogIOInfo(IOController_i::AnalogIOInfo* r);
			UniAnalogIOInfo(const IOController_i::AnalogIOInfo& r);

			UniAnalogIOInfo& operator=(IOController_i::AnalogIOInfo& r);
			const UniAnalogIOInfo& operator=(const IOController_i::AnalogIOInfo& r);
			UniAnalogIOInfo& operator=(IOController_i::AnalogIOInfo* r);
		
			void* any; 			/*!< расширение для возможности хранения своей информации */
			DependsList dlst; 	/*!< список io зависящих от данного (для выставления поля undefined) */
			bool dlst_lock; 	/*!< флаг блокирующий работу со списком */
			long block_value;

			UniSetTypes::uniset_spin_mutex val_lock; /*!< флаг блокирующий работу со значением */
		};


		// Функции работы со списками датчиков (без изменения 'const')
		typedef std::map<UniSetTypes::KeyType, UniDigitalIOInfo> DIOStateList;
		typedef std::map<UniSetTypes::KeyType, UniAnalogIOInfo> AIOStateList;
	
		inline DIOStateList::iterator dioBegin(){ return dioList.begin(); }
		inline DIOStateList::iterator dioEnd(){ return dioList.end(); }
		inline DIOStateList::iterator dfind(UniSetTypes::KeyType k){ return dioList.find(k); }
		inline int dioCount(){ return dioList.size(); }		

		inline AIOStateList::iterator aioBegin(){ return aioList.begin(); }
		inline AIOStateList::iterator aioEnd(){ return aioList.end(); }
		inline AIOStateList::iterator afind(UniSetTypes::KeyType k){ return aioList.find(k); }
		inline int aioCount(){ return aioList.size(); }

		struct DependsInfo
		{
			DependsInfo( bool init=false );
			DependsInfo( IOController_i::SensorInfo& si,
						 DIOStateList::iterator& dit, AIOStateList::iterator& ait );
			
			IOController_i::SensorInfo si;
			DIOStateList::iterator dit;
			AIOStateList::iterator ait;
			bool block_invert;	/*!< инвертирование логики для блокирования */
			bool init;
		};

		// доступ к элементам через итератор
		virtual void localSaveValue( AIOStateList::iterator& it, const IOController_i::SensorInfo& si,
										CORBA::Long newvalue, UniSetTypes::ObjectId sup_id );
		virtual void localSaveState( DIOStateList::iterator& it, const IOController_i::SensorInfo& si,
										CORBA::Boolean newstate, UniSetTypes::ObjectId sup_id );

	  	virtual void localSetState( DIOStateList::iterator& it, const IOController_i::SensorInfo& si,
										CORBA::Boolean newstate, UniSetTypes::ObjectId sup_id );

		virtual void localSetValue( AIOStateList::iterator& it, const IOController_i::SensorInfo& si,
										CORBA::Long value, UniSetTypes::ObjectId sup_id );

		virtual bool localGetState( DIOStateList::iterator& it, const IOController_i::SensorInfo& si );
	  	virtual long localGetValue( AIOStateList::iterator& it, const IOController_i::SensorInfo& si );
		

		/*! функция выставления признака неопределённого состояния для аналоговых датчиков 
			// для дискретных датчиков необходимости для подобной функции нет.
			// см. логику выставления в функции localSaveState
		*/
		virtual void localSetUndefinedState( AIOStateList::iterator& it, bool undefined,
												const IOController_i::SensorInfo& si );


	protected:
			// переопределяем для добавления вызова регистрации датчиков
			virtual bool disactivateObject();
			virtual bool activateObject();

			/*! регистрация датчиков, за информацию о которых отвечает данный IOController */
		    virtual void sensorsRegistration(){};
			/*! удаление из репозитория датчиков за информацию о которых отвечает данный IOController */
			virtual void sensorsUnregistration();
	
			/*! регистрация дискретного датчика 
				force=true - не проверять на дублирование (оптимизация)
			*/
			void dsRegistration( const UniDigitalIOInfo&, bool force=false );
			

			/*! регистрация аналогового датчика
				force=true - не проверять на дублирование (оптимизация)
			*/
			void asRegistration( const UniAnalogIOInfo&, bool force=false );

			
			/*! разрегистрация датчика */
			void sUnRegistration(const IOController_i::SensorInfo& si);
			
			
			UniSetTypes::Message::Priority getMessagePriority(UniSetTypes::KeyType k, UniversalIO::IOTypes type);
			
			// ------------------------------
			inline IOController_i::DigitalIOInfo
					DigitalIOInfo(bool s, UniversalIO::IOTypes t, const IOController_i::SensorInfo& si, 
									UniSetTypes::Message::Priority p = UniSetTypes::Message::Medium,
									bool defval=false )
			{
				IOController_i::DigitalIOInfo di;
				di.si = si;
				di.state = s;
				di.real_state = s;
				di.type = t;
				di.priority = p;
				di.default_val = defval;
				di.blocked = false;
				return di;
			};

			inline IOController_i::AnalogIOInfo
				AnalogIOInfo(long v, UniversalIO::IOTypes t, const IOController_i::SensorInfo& si, 
								UniSetTypes::Message::Priority p = UniSetTypes::Message::Medium,
								long defval=0, IOController_i::CalibrateInfo* ci=0 )
			{
				IOController_i::AnalogIOInfo ai;
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
		DIOStateList::iterator mydioBegin();
		DIOStateList::iterator mydioEnd();
		AIOStateList::iterator myaioBegin();
		AIOStateList::iterator myaioEnd();
		AIOStateList::iterator myafind(UniSetTypes::KeyType k);
		DIOStateList::iterator mydfind(UniSetTypes::KeyType k);

		// --------------------------
		// ФИЛЬТРОВАНИЕ
		// 
		typedef sigc::slot<bool,const UniAnalogIOInfo&, CORBA::Long, UniSetTypes::ObjectId> AFilterSlot;
		typedef sigc::slot<bool,const UniDigitalIOInfo&, CORBA::Boolean, UniSetTypes::ObjectId> DFilterSlot;
		typedef std::list<AFilterSlot> AFilterSlotList;
		typedef std::list<DFilterSlot> DFilterSlotList;

		/*
			Фильтрующая функция должна возвращать:
			TRUE - если значение 'нормальное'
			FALSE - если значение не подходит (отбрасывается)
		
			Пример использования: 
				addAFilter( sigc::mem_fun(my,&MyClass::my_filter) );
		*/
		AFilterSlotList::iterator addAFilter( AFilterSlot sl, bool push_front=false );
		DFilterSlotList::iterator addDFilter( DFilterSlot sl, bool push_front=false );
		void eraseAFilter(AFilterSlotList::iterator& it);
		void eraseDFilter(DFilterSlotList::iterator& it);

		// функии проверки текущего значения
		bool checkDFilters( const UniDigitalIOInfo& ai, CORBA::Boolean newstate, UniSetTypes::ObjectId sup_id );
		bool checkAFilters( const UniAnalogIOInfo& ai, CORBA::Long& newvalue, UniSetTypes::ObjectId sup_id );

		inline bool afiltersEmpty(){ return afilters.empty(); }
		inline bool dfiltersEmpty(){ return dfilters.empty(); }
		inline int afiltersSize(){ return afilters.size(); }
		inline int dfiltersSize(){ return dfilters.size(); }

		// ---------------------------
		// note: функция вызывается рекурсивно!!!
		void updateDepends( IOController::DependsList& lst, bool undefined, bool& lock );
		void updateBlockDepends( IOController::DependsList& lst, bool blk_state, bool& lock );

		void setCheckLockValuePause( int msec );
		inline int getCheckLockValuePause(){ return checkLockValuePause; }
	
	private:		
		friend class AskDumper;
	
		DIOStateList dioList;	/*!< список с текущим состоянием дискретных входов/выходов */
		AIOStateList aioList;	/*!< список с текущим состоянием аналоговых входов/выходов */
		
		UniSetTypes::uniset_mutex dioMutex;	/*!< замок для блокирования совместного доступа к dioList */
		UniSetTypes::uniset_mutex aioMutex; /*!< замок для блокирования совместного доступа к aioList */
		
		bool isPingDBServer;	// флаг связи с DBServer-ом 

		AFilterSlotList afilters; /*!< список фильтров для аналоговых значений */
		DFilterSlotList dfilters; /*!< список фильтров для дискретных значений */

		DependsSlot dslot; /*!< undefined depends slot */
		DependsSlot bslot; /*!< block depends slot */
		int checkLockValuePause;

		UniSetTypes::uniset_mutex loggingMutex; /*!< logging info mutex */
};

#endif
