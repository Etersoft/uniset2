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
 * \brief Реализация базового(фундаментального) класса для объектов системы
 * (процессов управления, элементов графического интерфейса и т.п.)
 * \author Pavel Vainerman
 */
//---------------------------------------------------------------------------
#ifndef UniSetObject_H_
#define UniSetObject_H_
//--------------------------------------------------------------------------
#include <condition_variable>
#include <thread>
#include <mutex>
#include <atomic>
#include <unistd.h>
#include <sys/time.h>
#include <ostream>
#include <memory>
#include <string>
#include <list>

#include "UniSetTypes.h"
#include "MessageType.h"
#include "PassiveTimer.h"
#include "Exceptions.h"
#include "UInterface.h"
#include "UniSetObject_i.hh"
#include "ThreadCreator.h"
#include "LT_Object.h"
#include "UMessageQueue.h"

//---------------------------------------------------------------------------
//#include <omnithread.h>
//---------------------------------------------------------------------------
class UniSetActivator;
class UniSetManager;

//---------------------------------------------------------------------------
class UniSetObject;
typedef std::list< std::shared_ptr<UniSetObject> > ObjectsList;     /*!< Список подчиненных объектов */
//---------------------------------------------------------------------------
/*! \class UniSetObject
 *    Класс реализует работу uniset-объекта: работа с очередью сообщений, регистрация объекта, инициализация и т.п.
 *    Для ожидания сообщений используется функция waitMessage(msec), основанная на таймере.
 *    Ожидание прерывается либо по истечении указанного времени, либо по приходу сообщения, при помощи функциии
 *    termWaiting() вызываемой из push().
 *     \note Если не будет задан ObjectId(-1), то поток обработки запущен не будет.
 *    Также создание потока можно принудительно отключить при помощи функции void thread(false). Ее необходимо вызвать до активации объекта
 *    (например в конструкторе). При этом ответственность за вызов receiveMessage() и processingMessage() возлагается
 *    на разработчика.
 *
*/
class UniSetObject:
	public std::enable_shared_from_this<UniSetObject>,
	public POA_UniSetObject_i,
	public LT_Object
{
	public:
		UniSetObject(const std::string& name, const std::string& section);
		UniSetObject(UniSetTypes::ObjectId id);
		UniSetObject();
		virtual ~UniSetObject();

		std::shared_ptr<UniSetObject> get_ptr()
		{
			return shared_from_this();
		}

		// Функции объявленные в IDL
		virtual CORBA::Boolean exist() override;

		virtual UniSetTypes::ObjectId getId() override
		{
			return myid;
		}
		inline const UniSetTypes::ObjectId getId() const
		{
			return myid;
		}
		inline std::string getName()
		{
			return myname;
		}

		virtual UniSetTypes::ObjectType getType() override
		{
			return UniSetTypes::ObjectType("UniSetObject");
		}
		virtual UniSetTypes::SimpleInfo* getInfo( ::CORBA::Long userparam = 0 ) override;
		friend std::ostream& operator<<(std::ostream& os, UniSetObject& obj );

		//! поместить сообщение в очередь
		virtual void push( const UniSetTypes::TransportMessage& msg ) override;

		/*! получить ссылку (на себя) */
		inline UniSetTypes::ObjectPtr getRef() const
		{
			UniSetTypes::uniset_rwmutex_rlock lock(refmutex);
			return (UniSetTypes::ObjectPtr)CORBA::Object::_duplicate(oref);
		}

		virtual timeout_t askTimer( UniSetTypes::TimerId timerid, timeout_t timeMS, clock_t ticks = -1,
									UniSetTypes::Message::Priority p = UniSetTypes::Message::High ) override;

	protected:
		/*! обработка приходящих сообщений */
		virtual void processingMessage( const UniSetTypes::VoidMessage* msg );
		virtual void sysCommand( const UniSetTypes::SystemMessage* sm ) {}
		virtual void sensorInfo( const UniSetTypes::SensorMessage* sm ) {}
		virtual void timerInfo( const UniSetTypes::TimerMessage* tm ) {}

		/*! Получить сообщение */
		VoidMessagePtr receiveMessage();

		/*! текущее количесво сообщений в очереди */
		size_t countMessages();
		size_t getCountOfQueueFull();

		/*! прервать ожидание сообщений */
		void termWaiting();

		std::shared_ptr<UInterface> ui; /*!< универсальный интерфейс для работы с другими процессами */
		std::string myname;
		std::string section;

		//! Дизактивизация объекта (переопределяется для необходимых действий перед деактивацией)
		virtual bool deactivateObject()
		{
			return true;
		}
		//! Активизация объекта (переопределяется для необходимых действий после активизации)
		virtual bool activateObject()
		{
			return true;
		}

		/*! запрет(разрешение) создания потока для обработки сообщений */
		inline void thread(bool create)
		{
			threadcreate = create;
		}
		/*! отключение потока обработки сообщений */
		inline void offThread()
		{
			threadcreate = false;
		}
		/*! включение потока обработки сообщений */
		inline void onThread()
		{
			threadcreate = true;
		}

		/*! функция вызываемая из потока */
		virtual void callback();

		/*! Функция вызываемая при приходе сигнала завершения или прерывания процесса. Переопределив ее можно
		 *    выполнять специфичные для процесса действия по обработке сигнала.
		 *    Например переход в безопасное состояние.
		 *  \warning В обработчике сигналов \b ЗАПРЕЩЕНО вызывать функции подобные exit(..), abort()!!!!
		*/
		virtual void sigterm( int signo );

		inline void terminate()
		{
			deactivate();
		}

		/*! Ожидать сообщения timeMS */
		virtual VoidMessagePtr waitMessage( timeout_t timeMS = UniSetTimer::WaitUpTime );

		void setID(UniSetTypes::ObjectId id);

		void setMaxSizeOfMessageQueue( size_t s )
		{
			mqueue.setMaxSizeOfMessageQueue(s);
		}

		inline size_t getMaxSizeOfMessageQueue()
		{
			return mqueue.getMaxSizeOfMessageQueue();
		}

		inline bool isActive()
		{
			return active;
		}
		inline void setActive( bool set )
		{
			active = set;
		}

		std::weak_ptr<UniSetManager> mymngr;

		void setThreadPriority( int p );

	private:

		friend class UniSetManager;
		friend class UniSetActivator;

		inline pid_t getMsgPID()
		{
			return msgpid;
		}

		/*! функция потока */
		void work();
		//! Инициализация параметров объекта
		bool init( const std::weak_ptr<UniSetManager>& om );
		//! Прямая деактивизация объекта
		bool deactivate();
		//! Непосредственная активизация объекта
		bool activate();
		/* регистрация в репозитории объектов */
		void registered();
		/* удаление ссылки из репозитория объектов     */
		void unregister();

		void initObject();

		pid_t msgpid = { 0 }; // pid потока обработки сообщений
		bool regOK = { false };
		std::atomic_bool active;

		bool threadcreate;
		std::shared_ptr<UniSetTimer> tmr;
		UniSetTypes::ObjectId myid;
		CORBA::Object_var oref;

		std::shared_ptr< ThreadCreator<UniSetObject> > thr;

		/*! очередь сообщений для объекта */
		UMessageQueue mqueue;

		/*! замок для блокирования совместного доступа к oRef */
		mutable UniSetTypes::uniset_rwmutex refmutex;

		std::atomic_bool a_working;
		std::mutex    m_working;
		std::condition_variable cv_working;
		//            timeout_t workingTerminateTimeout; /*!< время ожидания завершения потока */
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
