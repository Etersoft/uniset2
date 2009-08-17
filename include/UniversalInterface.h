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
 * \brief Универсальный интерфейс для взаимодействия с объектами системы
 * \author Pavel Vainerman
 * \version $Id: UniversalInterface.h,v 1.24 2008/12/14 21:57:51 vpashka Exp $
 * \date   $Date: 2008/12/14 21:57:51 $
 */
// --------------------------------------------------------------------------
#ifndef UniversalInterface_H_
#define UniversalInterface_H_
// ---------------------------------------------------------------------------
#include <string>
#include <sstream>
#include <map>
#include <functional>
#include <omniORB4/CORBA.h>
#include "Exceptions.h"
#include "UniSetTypes.h"
#include "ObjectIndex.h"
#include "ObjectRepository.h"
#include "IOController_i.hh"
#include "MessageType.h"
#include "Configuration.h"

// -----------------------------------------------------------------------------------------
/*! \namespace UniversalIO
 * Пространство имен содержащее классы, функции и т.п. для работы с вводом/выводом 
*/
namespace UniversalIO
{
    /*! Время ожидания ответа */
    const unsigned int defaultTimeOut=3;	// [сек]
}

// -----------------------------------------------------------------------------------------
//#define REPEAT_TIMEOUT	100 // [мс] пауза между попытками вызвать удаленную функцию объекта
//#define REPEAT_COUNT	5 	// количество попыток, после которого вырабатывается TimeOut

#define IO_THROW_EXCEPTIONS UniSetTypes::TimeOut,UniSetTypes::IOBadParam,UniSetTypes::ORepFailed

// -----------------------------------------------------------------------------------------
/*!
 * \class UniversalInterface
 * ... а здесь идет кратенькое описание... (коротенько минут на 40!...)
 * Для увеличения производительности в функции встроен cache обращений...
 *
 * См. также \ref UniversalIOControllerPage
*/ 
class UniversalInterface
{
	public:
	
		UniversalInterface( UniSetTypes::ObjectId backid, CORBA::ORB_var orb=NULL, UniSetTypes::ObjectIndex* oind=NULL );
		UniversalInterface( UniSetTypes::Configuration* uconf=UniSetTypes::conf );
		~UniversalInterface();
		
		inline UniSetTypes::ObjectIndex* getObjectIndex() { return oind; }

		// -------- Функции работы с группой датчиков -----------

		// Группа должна принадлежать одному процессу!
		
		//! Получение состояния для списка указанных датчиков
		IOController_i::ASensorInfoSeq_var getSensorSeq( UniSetTypes::IDList& lst );

		// Изменения состояния списка входов/выходов
		// Возвращает список не найденных идентификаторов
		UniSetTypes::IDSeq_var setOutputSeq( const IOController_i::OutSeq& lst, UniSetTypes::ObjectId sup_id );

		//! Заказ по списку
		UniSetTypes::IDSeq_var askSensorsSeq( UniSetTypes::IDList& lst, UniversalIO::UIOCommand cmd,
												UniSetTypes::ObjectId backid = UniSetTypes::DefaultObjectId );

		
		// ------------------------------------------------------


		//! Получение состояния дискретного датчика
		bool getState ( UniSetTypes::ObjectId id, UniSetTypes::ObjectId node ) throw(IO_THROW_EXCEPTIONS);
		bool getState ( UniSetTypes::ObjectId id );

		//! Получение состояния аналогового датчика
		long getValue ( UniSetTypes::ObjectId id, UniSetTypes::ObjectId node )throw(IO_THROW_EXCEPTIONS);
		long getValue ( UniSetTypes::ObjectId id );
		
		//! Вывод для дискретного датчика
		void setState ( UniSetTypes::ObjectId id, bool state, UniSetTypes::ObjectId node ) throw(IO_THROW_EXCEPTIONS);
		void setState ( UniSetTypes::ObjectId id, bool state );
		void setState ( IOController_i::SensorInfo& si, bool state, UniSetTypes::ObjectId supplier );

		//! Вывод для аналогового датчика
		void setValue ( UniSetTypes::ObjectId id, long value, UniSetTypes::ObjectId node ) throw(IO_THROW_EXCEPTIONS);
		void setValue ( UniSetTypes::ObjectId id, long value);
		void setValue ( IOController_i::SensorInfo& si, long value, UniSetTypes::ObjectId supplier );

		//! Запись состояния дискретного датчика на удаленный контроллер
		bool saveState ( UniSetTypes::ObjectId id, bool state, UniversalIO::IOTypes type, UniSetTypes::ObjectId node ) throw(IO_THROW_EXCEPTIONS);
		bool saveState ( UniSetTypes::ObjectId id, bool state, UniversalIO::IOTypes type );
		bool saveState ( IOController_i::SensorInfo& si, bool state, UniversalIO::IOTypes type, UniSetTypes::ObjectId supplier );
		
		//! Запись состояния аналогового датчика на удаленный контроллер
		bool saveValue ( UniSetTypes::ObjectId id, long value, UniversalIO::IOTypes type, UniSetTypes::ObjectId node ) throw(IO_THROW_EXCEPTIONS);
		bool saveValue ( UniSetTypes::ObjectId id, long value, UniversalIO::IOTypes type );
		bool saveValue ( IOController_i::SensorInfo& si, long value, UniversalIO::IOTypes type, UniSetTypes::ObjectId supplier );
		
		// функции не вырабатывающие исключений...
		void fastSaveValue( IOController_i::SensorInfo& si, long value, UniversalIO::IOTypes type, UniSetTypes::ObjectId supplier );
		void fastSaveState( IOController_i::SensorInfo& si, bool state, UniversalIO::IOTypes type, UniSetTypes::ObjectId supplier );
		void fastSetState( IOController_i::SensorInfo& si, bool state, UniSetTypes::ObjectId supplier );
		void fastSetValue( IOController_i::SensorInfo& si, long value, UniSetTypes::ObjectId supplier );

		// установка неопределённого состояния
		void setUndefinedState( IOController_i::SensorInfo& si, bool undefined, UniSetTypes::ObjectId supplier );


		CORBA::Long getRawValue( const IOController_i::SensorInfo& si );

		//! калибровка
		void calibrate(const IOController_i::SensorInfo& si, 
					   const IOController_i::CalibrateInfo& ci,
					   UniSetTypes::ObjectId adminId = UniSetTypes::DefaultObjectId );

		IOController_i::CalibrateInfo getCalibrateInfo( const IOController_i::SensorInfo& si );


		//! Заказ информации об изменении дискретного датчика
		void askRemoteState( UniSetTypes::ObjectId id, UniversalIO::UIOCommand cmd, UniSetTypes::ObjectId node,
							UniSetTypes::ObjectId backid = UniSetTypes::DefaultObjectId )throw(IO_THROW_EXCEPTIONS);		

		void askState( UniSetTypes::ObjectId id, UniversalIO::UIOCommand cmd, 
						UniSetTypes::ObjectId backid = UniSetTypes::DefaultObjectId );

		//! Заказ информации об изменении аналогового датчика						
		void askRemoteValue ( UniSetTypes::ObjectId id, UniversalIO::UIOCommand cmd, UniSetTypes::ObjectId node,
						UniSetTypes::ObjectId backid = UniSetTypes::DefaultObjectId ) throw(IO_THROW_EXCEPTIONS);
		void askValue ( UniSetTypes::ObjectId id, UniversalIO::UIOCommand cmd,
								UniSetTypes::ObjectId backid = UniSetTypes::DefaultObjectId );

		void askThreshold ( UniSetTypes::ObjectId sensorId, UniSetTypes::ThresholdId tid,
							UniversalIO::UIOCommand cmd,
							CORBA::Long lowLimit=0, CORBA::Long hiLimit=0, CORBA::Long sensibility=0, 
							UniSetTypes::ObjectId backid = UniSetTypes::DefaultObjectId );

		void askRemoteThreshold( UniSetTypes::ObjectId sensorId, UniSetTypes::ObjectId node,
								 UniSetTypes::ThresholdId thresholdId, UniversalIO::UIOCommand cmd,
								 CORBA::Long lowLimit=0, CORBA::Long hiLimit=0, CORBA::Long sensibility=0, 
								 UniSetTypes::ObjectId backid = UniSetTypes::DefaultObjectId );

		//! Универсальный заказ информации об изменении датчика
		void askSensor( UniSetTypes::ObjectId id, UniversalIO::UIOCommand cmd,
							UniSetTypes::ObjectId backid = UniSetTypes::DefaultObjectId );

		void askRemoteSensor( UniSetTypes::ObjectId id, UniversalIO::UIOCommand cmd, UniSetTypes::ObjectId node,
							UniSetTypes::ObjectId backid = UniSetTypes::DefaultObjectId )throw(IO_THROW_EXCEPTIONS);		


		void askOutput( UniSetTypes::ObjectId id, UniversalIO::UIOCommand cmd, 
						UniSetTypes::ObjectId backid = UniSetTypes::DefaultObjectId );

		void askRemoteOutput( UniSetTypes::ObjectId id, UniversalIO::UIOCommand cmd, UniSetTypes::ObjectId node,
							UniSetTypes::ObjectId backid = UniSetTypes::DefaultObjectId )throw(IO_THROW_EXCEPTIONS);

		//! Заказ таймера
		void askTimer( UniSetTypes::TimerId timerid, CORBA::Long timeMS, CORBA::Short ticks=-1,
						UniSetTypes::Message::Priority piority=UniSetTypes::Message::High,
						UniSetTypes::ObjectId backid = UniSetTypes::DefaultObjectId) throw(IO_THROW_EXCEPTIONS);

		//! Заказ сообщения
		void askMessage( UniSetTypes::MessageCode mid, UniversalIO::UIOCommand cmd, bool ack = true,
							UniSetTypes::ObjectId backid = UniSetTypes::DefaultObjectId ) throw(IO_THROW_EXCEPTIONS);
		void askMessageRange( UniSetTypes::MessageCode from, UniSetTypes::MessageCode to,
								UniversalIO::UIOCommand cmd, bool ack = true,
								UniSetTypes::ObjectId backid = UniSetTypes::DefaultObjectId ) throw(IO_THROW_EXCEPTIONS);
		

		UniversalIO::IOTypes getIOType(UniSetTypes::ObjectId id, UniSetTypes::ObjectId node) throw(IO_THROW_EXCEPTIONS);
		UniversalIO::IOTypes getIOType(UniSetTypes::ObjectId id);
		UniSetTypes::ObjectType getType(UniSetTypes::ObjectId id, UniSetTypes::ObjectId node) throw(IO_THROW_EXCEPTIONS);
		UniSetTypes::ObjectType getType(UniSetTypes::ObjectId id);

		IOController_i::ShortIOInfo getChangedTime( UniSetTypes::ObjectId id, UniSetTypes::ObjectId node );


//		/*! регистрация объекта в репозитории */
		void registered(UniSetTypes::ObjectId id, const UniSetTypes::ObjectPtr oRef, bool force=false)throw(UniSetTypes::ORepFailed);
		void registered(UniSetTypes::ObjectId id, UniSetTypes::ObjectId node, const UniSetTypes::ObjectPtr oRef, bool force=false)throw(UniSetTypes::ORepFailed);

//		/*! разрегистрация объекта */
		void unregister(UniSetTypes::ObjectId id)throw(UniSetTypes::ORepFailed);
		void unregister(UniSetTypes::ObjectId id, UniSetTypes::ObjectId node)throw(UniSetTypes::ORepFailed);

		/*! получение ссылки на объект */
		inline UniSetTypes::ObjectPtr resolve(const char* name)
		{
		    return rep.resolve(name);
		}

		inline UniSetTypes::ObjectPtr resolve( UniSetTypes::ObjectId id )
		{
			std::string nm = oind->getNameById(id);
			return rep.resolve(nm);
		}

		UniSetTypes::ObjectPtr resolve( UniSetTypes::ObjectId id, UniSetTypes::ObjectId nodeName, int timeoutMS=UniversalIO::defaultTimeOut)
			throw(UniSetTypes::ResolveNameError, UniSetTypes::TimeOut);

		
		bool isExist( UniSetTypes::ObjectId id );
		bool isExist( UniSetTypes::ObjectId id, UniSetTypes::ObjectId node );

		/*! получение идентификатора объекта по имени */
		inline UniSetTypes::ObjectId getIdByName(const char* name)
		{
		    return oind->getIdByName(name);
		}

		/*! получение имени по идентификатору объекта */
		inline std::string getNameById( UniSetTypes::ObjectId id )
		{
			return oind->getNameById(id);
		}

		inline std::string getNameById( UniSetTypes::ObjectId id, UniSetTypes::ObjectId node )
		{
			return oind->getNameById(id, node);
		}

		inline UniSetTypes::ObjectId getNodeId(const std::string& fullname)
		{
			return oind->getNodeId(fullname);
		}

		inline std::string getName(const std::string& fullname)
		{
			return oind->getName(fullname);
		}

		inline std::string getTextName( UniSetTypes::ObjectId id )
		{
		    return oind->getTextName(id);
		}

		static std::string timeToString(time_t tm=time(0), const std::string brk=":"); /*!< Преобразование времени в строку HH:MM:SS */
		static std::string dateToString(time_t tm=time(0), const std::string brk="/"); /*!< Преобразование даты в строку DD/MM/YYYY */

		/*! посылка сообщения msg объекту name на узел node */
		void send( UniSetTypes::ObjectId name, UniSetTypes::TransportMessage& msg, UniSetTypes::ObjectId node) throw(IO_THROW_EXCEPTIONS);
		void send( UniSetTypes::ObjectId name, UniSetTypes::TransportMessage& msg);

		bool info( std::string msg, UniSetTypes::ObjectId messenger,  
					UniSetTypes::ObjectId fromNode = UniSetTypes::conf->getLocalNode(),
					UniSetTypes::InfoMessage::Character ch=UniSetTypes::InfoMessage::Normal,
					UniSetTypes::ObjectId from=UniSetTypes::DefaultObjectId );

		bool alarm( std::string msg, UniSetTypes::ObjectId messenger, 
					UniSetTypes::ObjectId fromNode = UniSetTypes::conf->getLocalNode(),
					UniSetTypes::AlarmMessage::Character ch=UniSetTypes::AlarmMessage::Alarm, 
					UniSetTypes::ObjectId from=UniSetTypes::DefaultObjectId );

		bool info( UniSetTypes::InfoMessage& msg,  UniSetTypes::ObjectId messenger);
		bool alarm( UniSetTypes::AlarmMessage& msg,  UniSetTypes::ObjectId messenger);
		
		bool waitReady( UniSetTypes::ObjectId id, int msec, int pause=5000,
						UniSetTypes::ObjectId node = UniSetTypes::conf->getLocalNode() ); 	// used exist

		bool waitWorking( UniSetTypes::ObjectId id, int msec, int pause=3000,
							UniSetTypes::ObjectId node = UniSetTypes::conf->getLocalNode() ); 	// used getState

		inline void setCacheMaxSize( unsigned int newsize)
		{
			rcache.setMaxSize(newsize);
		}

		/*!
			\todo Убедится в уникальности возвращаемого результата	hash(...)	
		*/
		class CacheOfResolve
		{
			public:
					CacheOfResolve(unsigned int maxsize, int cleantime):
						MaxSize(maxsize), CleanTime(cleantime){};
					~CacheOfResolve(){};
			
					UniSetTypes::ObjectPtr resolve( UniSetTypes::ObjectId id, UniSetTypes::ObjectId node )throw(UniSetTypes::NameNotFound);
					void cache(UniSetTypes::ObjectId id, UniSetTypes::ObjectId node, UniSetTypes::ObjectVar ptr);
					void erase(UniSetTypes::ObjectId id, UniSetTypes::ObjectId node);
					
					inline void setMaxSize( unsigned int ms)
					{
						MaxSize = ms;
					};

//					void setCleanTime();

			protected:
					CacheOfResolve(){};
			private:
			
				bool clean(); 		/*!< функция очистки кэш-а от старых ссылок */
				inline void clear() /*!< удаление всей информации */
				{
					mcache.clear();	
				}; 
								
				/*!
					\todo можно добавить поле CleanTime для каждой ссылки отдельно...
				*/
				struct Info
				{
					Info( UniSetTypes::ObjectVar ptr, time_t tm=0):
							ptr(ptr)
					{
						if(!tm)
							 timestamp = time(NULL);
					}
					
					Info():
						ptr(NULL), timestamp(0){};

					UniSetTypes::ObjectVar ptr;
					time_t timestamp; // время последнего обращения 

					bool operator<( const CacheOfResolve::Info& rhs ) const
					{
						return this->timestamp < rhs.timestamp;
					}

				};
				
				typedef std::map<int, Info> CacheMap;
				CacheMap mcache;
				unsigned int MaxSize;	/*!< максимальный размер кэша */
				unsigned int CleanTime;	/*!< период устаревания ссылок [мин] */

/*
				// В последствии написать функцию для использования
				// remove_if
				typedef std::pair<int, Info> CacheItem;
				// функция-объект для поиска устаревших(по времени) ссылок
				struct OldRef_eq: public unary_function<CacheItem, bool>
				{		
					OldRef_eq(time_t tm):tm(tm){}
					bool operator()( const CacheItem& inf ) const
					{
						return inf.timestamp < tm;
					}
					time_t tm;
				};
*/				
		};

		void initBackId( UniSetTypes::ObjectId backid );
	protected:
		std::string set_err(const std::string& pre, UniSetTypes::ObjectId id, UniSetTypes::ObjectId node); 
	private:
		void init();

		ObjectRepository rep;
		UniSetTypes::ObjectId myid;
		CosNaming::NamingContext_var localctx;
		CORBA::ORB_var orb;
		CacheOfResolve rcache;
		UniSetTypes::ObjectIndex* oind;
		UniSetTypes::Configuration* uconf;
};
// --------------------------------------------------------------------------
#endif
// --------------------------------------------------------------------------
