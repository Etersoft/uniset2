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
 */
// --------------------------------------------------------------------------
#ifndef UInterface_H_
#define UInterface_H_
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
#define IO_THROW_EXCEPTIONS UniSetTypes::TimeOut,UniSetTypes::IOBadParam,UniSetTypes::ORepFailed
// -----------------------------------------------------------------------------------------
/*!
 * \class UInterface
 * Универсальный интерфейс для взаимодействия между объектами (процессами).
 * По сути является "фасадом" к реализации механизма взамиодействия
  * в libuniset (основанном на CORBA) Хотя до конца скрыть CORBA-у пока не удалось.
 * Для увеличения производительности в функции встроен cache обращений...
 *
 * См. также \ref UniversalIOControllerPage
*/
class UInterface
{
	public:

		UInterface( UniSetTypes::ObjectId backid, CORBA::ORB_var orb=NULL, UniSetTypes::ObjectIndex* oind=NULL );
		UInterface( UniSetTypes::Configuration* uconf=UniSetTypes::conf );
		~UInterface();

		// ---------------------------------------------------------------
		// Работа с датчиками

		//! Получение состояния датчика
		long getValue ( UniSetTypes::ObjectId id, UniSetTypes::ObjectId node )throw(IO_THROW_EXCEPTIONS);
		long getValue ( UniSetTypes::ObjectId id );
		long getRawValue( const IOController_i::SensorInfo& si );

		//! Выставление состояния датчика
		void setValue ( UniSetTypes::ObjectId id, long value, UniSetTypes::ObjectId node ) throw(IO_THROW_EXCEPTIONS);
		void setValue ( UniSetTypes::ObjectId id, long value );
		void setValue ( IOController_i::SensorInfo& si, long value, UniSetTypes::ObjectId supplier );

		// fast - это удалённый вызов "без подтверждения", он быстрее, но менее надёжен
		// т.к. вызывающий никогда не узнает об ошибке, если она была (датчик такой не найдён и т.п.)
		void fastSetValue( IOController_i::SensorInfo& si, long value, UniSetTypes::ObjectId supplier );

		//! Получение состояния для списка указанных датчиков
		IOController_i::SensorInfoSeq_var getSensorSeq( UniSetTypes::IDList& lst );

		/*! Изменения состояния списка входов/выходов
			\return Возвращает список не найденных идентификаторов */
		UniSetTypes::IDSeq_var setOutputSeq( const IOController_i::OutSeq& lst, UniSetTypes::ObjectId sup_id );

		// ---------------------------------------------------------------
		// Заказ датчиков

		//! Универсальный заказ информации об изменении датчика
		void askSensor( UniSetTypes::ObjectId id, UniversalIO::UIOCommand cmd,
							UniSetTypes::ObjectId backid = UniSetTypes::DefaultObjectId );

		void askRemoteSensor( UniSetTypes::ObjectId id, UniversalIO::UIOCommand cmd, UniSetTypes::ObjectId node,
							UniSetTypes::ObjectId backid = UniSetTypes::DefaultObjectId )throw(IO_THROW_EXCEPTIONS);

		//! Заказ по списку
		UniSetTypes::IDSeq_var askSensorsSeq( UniSetTypes::IDList& lst, UniversalIO::UIOCommand cmd,
												UniSetTypes::ObjectId backid = UniSetTypes::DefaultObjectId );
		// ------------------------------------------------------

		// установка неопределённого состояния
		void setUndefinedState( IOController_i::SensorInfo& si, bool undefined, UniSetTypes::ObjectId supplier );

		// ---------------------------------------------------------------
		// Калибровка... пороги...

		//! калибровка
		void calibrate(const IOController_i::SensorInfo& si,
					   const IOController_i::CalibrateInfo& ci,
					   UniSetTypes::ObjectId adminId = UniSetTypes::DefaultObjectId );

		IOController_i::CalibrateInfo getCalibrateInfo( const IOController_i::SensorInfo& si );


		//! Заказ информации об изменении дискретного датчика
		void askThreshold( UniSetTypes::ObjectId sensorId, UniSetTypes::ThresholdId tid,
							UniversalIO::UIOCommand cmd,
							CORBA::Long lowLimit=0, CORBA::Long hiLimit=0,
							UniSetTypes::ObjectId backid = UniSetTypes::DefaultObjectId );

		void askRemoteThreshold( UniSetTypes::ObjectId sensorId, UniSetTypes::ObjectId node,
								 UniSetTypes::ThresholdId thresholdId, UniversalIO::UIOCommand cmd,
								 CORBA::Long lowLimit=0, CORBA::Long hiLimit=0,
								 UniSetTypes::ObjectId backid = UniSetTypes::DefaultObjectId );

		// ---------------------------------------------------------------
		// Вспомогательные функции

		UniversalIO::IOType getIOType(UniSetTypes::ObjectId id, UniSetTypes::ObjectId node) throw(IO_THROW_EXCEPTIONS);
		UniversalIO::IOType getIOType(UniSetTypes::ObjectId id);

		// read from xml (only for xml!) т.е. без удалённого запроса
		UniversalIO::IOType getConfIOType( UniSetTypes::ObjectId id );

		// Получение типа объекта..
		UniSetTypes::ObjectType getType(UniSetTypes::ObjectId id, UniSetTypes::ObjectId node) throw(IO_THROW_EXCEPTIONS);
		UniSetTypes::ObjectType getType(UniSetTypes::ObjectId id);


		//! Время последнего изменения датчика
		IOController_i::ShortIOInfo getChangedTime( UniSetTypes::ObjectId id, UniSetTypes::ObjectId node );

		//! Получить список датчиков
		IOController_i::ShortMapSeq* getSensors( UniSetTypes::ObjectId id,
													UniSetTypes::ObjectId node=UniSetTypes::conf->getLocalNode() );

		// ---------------------------------------------------------------
		// Работа с репозиторием

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


		// Проверка доступности объекта или датчика
		bool isExist( UniSetTypes::ObjectId id );
		bool isExist( UniSetTypes::ObjectId id, UniSetTypes::ObjectId node );

		bool waitReady( UniSetTypes::ObjectId id, int msec, int pause=5000,
						UniSetTypes::ObjectId node = UniSetTypes::conf->getLocalNode() ); 	// used exist

		bool waitWorking( UniSetTypes::ObjectId id, int msec, int pause=3000,
							UniSetTypes::ObjectId node = UniSetTypes::conf->getLocalNode() ); 	// used getValue

		// ---------------------------------------------------------------
		// Работа с ID, Name

		/*! получение идентификатора объекта по имени */
		inline UniSetTypes::ObjectId getIdByName(const char* name)
		{
		    return oind->getIdByName(name);
		}
		inline UniSetTypes::ObjectId getIdByName(const string name)
		{
		    return getIdByName(name.c_str());
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


		// ---------------------------------------------------------------
		// Получение указателей на вспомогательные классы.
		inline UniSetTypes::ObjectIndex* getObjectIndex() { return oind; }
		inline UniSetTypes::Configuration* getConf() { return uconf; }

		// ---------------------------------------------------------------
		// Посылка сообщений

		/*! посылка сообщения msg объекту name на узел node */
		void send( UniSetTypes::ObjectId name, UniSetTypes::TransportMessage& msg, UniSetTypes::ObjectId node) throw(IO_THROW_EXCEPTIONS);
		void send( UniSetTypes::ObjectId name, UniSetTypes::TransportMessage& msg);

		// ---------------------------------------------------------------
		// Вспомогательный класс для кэширования ссылок на удалённые объекты

		inline void setCacheMaxSize( unsigned int newsize)
		{
			rcache.setMaxSize(newsize);
		}

		/*! Кэш ссылок на объекты */
		class CacheOfResolve
		{
			public:
					CacheOfResolve(unsigned int maxsize, int cleantime):
						MaxSize(maxsize), CleanTime(cleantime){};
					~CacheOfResolve(){};

					UniSetTypes::ObjectPtr resolve( UniSetTypes::ObjectId id, UniSetTypes::ObjectId node )throw(UniSetTypes::NameNotFound);
					void cache(UniSetTypes::ObjectId id, UniSetTypes::ObjectId node, UniSetTypes::ObjectVar ptr);
					void erase(UniSetTypes::ObjectId id, UniSetTypes::ObjectId node);

					inline void setMaxSize( unsigned int ms )
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
					UniSetTypes::uniset_rwmutex_wrlock l(cmutex);
					mcache.clear();
				};

				/*!
					\todo можно добавить поле CleanTime для каждой ссылки отдельно...
				*/
				struct Info
				{
					Info( UniSetTypes::ObjectVar ptr, time_t tm=0 ):
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
				UniSetTypes::uniset_rwmutex cmutex;
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
