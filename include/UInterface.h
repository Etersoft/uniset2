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
#include <memory>
#include <string>
#include <sstream>
#include <unordered_map>
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
	const unsigned int defaultTimeOut = 3;  // [сек]
}

// -----------------------------------------------------------------------------------------
#define UI_THROW_EXCEPTIONS UniSetTypes::TimeOut,UniSetTypes::IOBadParam,UniSetTypes::ORepFailed
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

		UInterface( const UniSetTypes::ObjectId backid, CORBA::ORB_var orb = NULL, const std::shared_ptr<UniSetTypes::ObjectIndex> oind = nullptr );
		UInterface( const std::shared_ptr<UniSetTypes::Configuration>& uconf = UniSetTypes::uniset_conf() );
		~UInterface();

		// ---------------------------------------------------------------
		// Работа с датчиками

		//! Получение состояния датчика
		long getValue ( const UniSetTypes::ObjectId id, const UniSetTypes::ObjectId node ) const throw(UI_THROW_EXCEPTIONS);
		long getValue ( const UniSetTypes::ObjectId id ) const;
		long getRawValue( const IOController_i::SensorInfo& si );

		//! Выставление состояния датчика
		void setValue ( const UniSetTypes::ObjectId id, long value, const UniSetTypes::ObjectId node ) const throw(UI_THROW_EXCEPTIONS);
		void setValue ( const UniSetTypes::ObjectId id, long value ) const;
		void setValue ( const IOController_i::SensorInfo& si, long value, const UniSetTypes::ObjectId supplier );

		// fast - это удалённый вызов "без подтверждения", он быстрее, но менее надёжен
		// т.к. вызывающий никогда не узнает об ошибке, если она была (датчик такой не найдён и т.п.)
		void fastSetValue( const IOController_i::SensorInfo& si, long value, UniSetTypes::ObjectId supplier );

		//! Получение состояния для списка указанных датчиков
		IOController_i::SensorInfoSeq_var getSensorSeq( const UniSetTypes::IDList& lst );

		/*! Изменения состояния списка входов/выходов
		    \return Возвращает список не найденных идентификаторов */
		UniSetTypes::IDSeq_var setOutputSeq( const IOController_i::OutSeq& lst, UniSetTypes::ObjectId sup_id );

		// ---------------------------------------------------------------
		// Заказ датчиков

		//! Универсальный заказ информации об изменении датчика
		void askSensor( const UniSetTypes::ObjectId id, UniversalIO::UIOCommand cmd,
						UniSetTypes::ObjectId backid = UniSetTypes::DefaultObjectId ) const;

		void askRemoteSensor( const UniSetTypes::ObjectId id, UniversalIO::UIOCommand cmd, const UniSetTypes::ObjectId node,
							  UniSetTypes::ObjectId backid = UniSetTypes::DefaultObjectId ) const throw(UI_THROW_EXCEPTIONS);

		//! Заказ по списку
		UniSetTypes::IDSeq_var askSensorsSeq( const UniSetTypes::IDList& lst, UniversalIO::UIOCommand cmd,
											  UniSetTypes::ObjectId backid = UniSetTypes::DefaultObjectId );
		// ------------------------------------------------------

		// установка неопределённого состояния
		void setUndefinedState( const IOController_i::SensorInfo& si, bool undefined, UniSetTypes::ObjectId supplier );

		// ---------------------------------------------------------------
		// Калибровка... пороги...

		//! калибровка
		void calibrate(const IOController_i::SensorInfo& si,
					   const IOController_i::CalibrateInfo& ci,
					   UniSetTypes::ObjectId adminId = UniSetTypes::DefaultObjectId );

		IOController_i::CalibrateInfo getCalibrateInfo( const IOController_i::SensorInfo& si );

		//! Заказ информации об изменении порога
		void askThreshold( const UniSetTypes::ObjectId sensorId, const UniSetTypes::ThresholdId tid,
						   UniversalIO::UIOCommand cmd,
						   long lowLimit, long hiLimit, bool invert = false,
						   UniSetTypes::ObjectId backid = UniSetTypes::DefaultObjectId ) const;

		void askRemoteThreshold( const UniSetTypes::ObjectId sensorId, const UniSetTypes::ObjectId node,
								 const UniSetTypes::ThresholdId thresholdId, UniversalIO::UIOCommand cmd,
								 long lowLimit, long hiLimit, bool invert = false,
								 UniSetTypes::ObjectId backid = UniSetTypes::DefaultObjectId ) const;

		IONotifyController_i::ThresholdInfo getThresholdInfo( const IOController_i::SensorInfo& si, const UniSetTypes::ThresholdId tid ) const;
		IONotifyController_i::ThresholdInfo getThresholdInfo( const UniSetTypes::ObjectId sid, const UniSetTypes::ThresholdId tid ) const;

		// ---------------------------------------------------------------
		// Вспомогательные функции

		UniversalIO::IOType getIOType(const UniSetTypes::ObjectId id, UniSetTypes::ObjectId node) const throw(UI_THROW_EXCEPTIONS);
		UniversalIO::IOType getIOType(const UniSetTypes::ObjectId id) const;

		// read from xml (only for xml!) т.е. без удалённого запроса
		UniversalIO::IOType getConfIOType( const UniSetTypes::ObjectId id ) const;

		// Получение типа объекта..
		UniSetTypes::ObjectType getType(const UniSetTypes::ObjectId id, const UniSetTypes::ObjectId node) const throw(UI_THROW_EXCEPTIONS);
		UniSetTypes::ObjectType getType(const UniSetTypes::ObjectId id) const;


		//! Время последнего изменения датчика
		IOController_i::ShortIOInfo getChangedTime( const UniSetTypes::ObjectId id, const UniSetTypes::ObjectId node ) const;

		//! Получить список датчиков
		IOController_i::ShortMapSeq* getSensors( const UniSetTypes::ObjectId id,
				const UniSetTypes::ObjectId node = UniSetTypes::uniset_conf()->getLocalNode() );

		IOController_i::SensorInfoSeq* getSensorsMap( const UniSetTypes::ObjectId id,
				const UniSetTypes::ObjectId node = UniSetTypes::uniset_conf()->getLocalNode() );

		IONotifyController_i::ThresholdsListSeq* getThresholdsList( const UniSetTypes::ObjectId id,
				const UniSetTypes::ObjectId node = UniSetTypes::uniset_conf()->getLocalNode() );
		// ---------------------------------------------------------------
		// Работа с репозиторием

		//        /*! регистрация объекта в репозитории */
		void registered(const UniSetTypes::ObjectId id, const UniSetTypes::ObjectPtr oRef, bool force = false)const throw(UniSetTypes::ORepFailed);

		//        /*! разрегистрация объекта */
		void unregister(const UniSetTypes::ObjectId id)throw(UniSetTypes::ORepFailed);

		/*! получение ссылки на объект */
		inline UniSetTypes::ObjectPtr resolve( const std::string& name ) const
		{
			return rep.resolve(name);
		}

		inline UniSetTypes::ObjectPtr resolve( const UniSetTypes::ObjectId id ) const
		{
			return rep.resolve( oind->getNameById(id) );
		}

		UniSetTypes::ObjectPtr resolve( const UniSetTypes::ObjectId id, const UniSetTypes::ObjectId nodeName, int timeoutMS = UniversalIO::defaultTimeOut) const
		throw(UniSetTypes::ResolveNameError, UniSetTypes::TimeOut);


		// Проверка доступности объекта или датчика
		bool isExist( const UniSetTypes::ObjectId id ) const;
		bool isExist( const UniSetTypes::ObjectId id, const UniSetTypes::ObjectId node ) const;

		// used for check 'isExist'
		bool waitReady( const UniSetTypes::ObjectId id, int msec, int pause = 5000,
						const UniSetTypes::ObjectId node = UniSetTypes::uniset_conf()->getLocalNode() );

		// used for check 'getValue'
		bool waitWorking( const UniSetTypes::ObjectId id, int msec, int pause = 3000,
						  const UniSetTypes::ObjectId node = UniSetTypes::uniset_conf()->getLocalNode() );

		// ---------------------------------------------------------------
		// Работа с ID, Name

		/*! получение идентификатора объекта по имени */
		inline UniSetTypes::ObjectId getIdByName( const std::string& name ) const
		{
			return oind->getIdByName(name);
		}

		/*! получение имени по идентификатору объекта */
		inline std::string getNameById( const UniSetTypes::ObjectId id ) const
		{
			return oind->getNameById(id);
		}

		inline UniSetTypes::ObjectId getNodeId( const std::string& fullname ) const
		{
			return oind->getNodeId(fullname);
		}

		inline std::string getTextName( const UniSetTypes::ObjectId id ) const
		{
			return oind->getTextName(id);
		}

		// ---------------------------------------------------------------
		// Получение указателей на вспомогательные классы.
		inline const std::shared_ptr<UniSetTypes::ObjectIndex> getObjectIndex()
		{
			return oind;
		}
		inline const std::shared_ptr<UniSetTypes::Configuration> getConf()
		{
			return uconf;
		}
		// ---------------------------------------------------------------
		// Посылка сообщений

		/*! посылка сообщения msg объекту name на узел node */
		void send( const UniSetTypes::ObjectId name, const UniSetTypes::TransportMessage& msg, UniSetTypes::ObjectId node ) throw(UI_THROW_EXCEPTIONS);
		void send( const UniSetTypes::ObjectId name, const UniSetTypes::TransportMessage& msg);

		// ---------------------------------------------------------------
		// Вспомогательный класс для кэширования ссылок на удалённые объекты

		inline void setCacheMaxSize( unsigned int newsize )
		{
			rcache.setMaxSize(newsize);
		}

		/*! Кэш ссылок на объекты */
		class CacheOfResolve
		{
			public:
				CacheOfResolve( unsigned int maxsize, int cleancount = 20 ):
					MaxSize(maxsize), minCallCount(cleancount) {};
				~CacheOfResolve() {};

				UniSetTypes::ObjectPtr resolve( const UniSetTypes::ObjectId id, const UniSetTypes::ObjectId node ) const throw(UniSetTypes::NameNotFound);
				void cache( const UniSetTypes::ObjectId id, const UniSetTypes::ObjectId node, UniSetTypes::ObjectVar ptr ) const;
				void erase( const UniSetTypes::ObjectId id, const UniSetTypes::ObjectId node ) const;

				inline void setMaxSize( unsigned int ms )
				{
					MaxSize = ms;
				};

			protected:
				CacheOfResolve() {};

			private:

				bool clean();       /*!< функция очистки кэш-а от старых ссылок */
				inline void clear() /*!< удаление всей информации */
				{
					UniSetTypes::uniset_rwmutex_wrlock l(cmutex);
					mcache.clear();
				};

				struct Info
				{
					Info( const UniSetTypes::ObjectVar& ptr ): ptr(ptr), ncall(0) {}
					Info(): ptr(NULL), ncall(0) {}

					UniSetTypes::ObjectVar ptr;
					unsigned long ncall; // счётчик обращений

					bool operator<( const CacheOfResolve::Info& rhs ) const
					{
						return this->ncall > rhs.ncall;
					}
				};

				typedef std::unordered_map<int, Info> CacheMap;
				mutable CacheMap mcache;
				mutable UniSetTypes::uniset_rwmutex cmutex;
				unsigned int MaxSize;      /*!< максимальный размер кэша */
				unsigned int minCallCount; /*!< минимальное количество вызовов, меньше которого ссылка считается устаревшей */
		};

		void initBackId( UniSetTypes::ObjectId backid );
	protected:
		std::string set_err(const std::string& pre, const UniSetTypes::ObjectId id, const UniSetTypes::ObjectId node) const;

	private:
		void init();

		ObjectRepository rep;
		UniSetTypes::ObjectId myid;
		mutable CosNaming::NamingContext_var localctx;
		mutable CORBA::ORB_var orb;
		CacheOfResolve rcache;
		std::shared_ptr<UniSetTypes::ObjectIndex> oind;
		std::shared_ptr<UniSetTypes::Configuration> uconf;
};
// --------------------------------------------------------------------------
#endif
// --------------------------------------------------------------------------
