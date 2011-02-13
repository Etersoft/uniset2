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
 * \brief Реализация базового(фундаментального) класса для объектов системы 
 * (процессов управления, элементов графического интерфейса и т.п.)
 * \author Pavel Vainerman
 */
//---------------------------------------------------------------------------
#ifndef UniSetObject_H_
#define UniSetObject_H_
//--------------------------------------------------------------------------
#include <unistd.h>
#include <sys/time.h>
#include <queue>
#include <ostream>
#include <string>
#include <list>

#include "UniSetTypes.h"
#include "MessageType.h"
#include "PassiveTimer.h"
#include "Exceptions.h"
#include "UniversalInterface.h"
#include "UniSetObject_i.hh"
#include "ThreadCreator.h"

//---------------------------------------------------------------------------
//#include <omnithread.h>
//---------------------------------------------------------------------------
class ObjectsActivator;
class ObjectsManager;

//---------------------------------------------------------------------------
class UniSetObject;
typedef std::list<UniSetObject *> ObjectsList; 	/*!< Список подчиненных объектов */
//---------------------------------------------------------------------------
/*! \class UniSetObject
 *	Класс задает такие свойства объекта как: получение сообщений, помещение сообщения в очередь и т.п.
 *	Для ожидания сообщений используется функция waitMessage(), основанная на таймере.
 *	Ожидание прерывается либо по истечении указанного времени, либо по приходу сообщения, при помощи функциии
 *	termWaiting() вызываемой из push(). 
 * 	\note Если не будет задан ObjectId(-1), то поток обработки запущен не будет.
 *	Также создание потока можно принудительно отключить при помощи функции void thread(). Ее необходимо вызвать до активации объекта
 *	(например в конструкторе). При этом ответственность за вызов receiveMessage() и processingMessage() возлагается 
 *	на разработчика.
*/ 
class UniSetObject:
	public POA_UniSetObject_i
{
	public:
		UniSetObject(const std::string name, const std::string section); 
		UniSetObject(UniSetTypes::ObjectId id);
		UniSetObject();
		virtual ~UniSetObject();

		// Функции объявленные в IDL
		virtual CORBA::Boolean exist();
		virtual char* getName(){return (char*)myname.c_str();}
		virtual UniSetTypes::ObjectId getId(){ return myid; }
		virtual UniSetTypes::ObjectType getType() { return UniSetTypes::getObjectType("UniSetObject"); }
		virtual UniSetTypes::SimpleInfo* getInfo();
		friend std::ostream& operator<<(std::ostream& os, UniSetObject& obj );

		//! поместить сообщение в очередь
		virtual void push(const UniSetTypes::TransportMessage& msg);

		/*! получить ссылку (на себя) */
		inline UniSetTypes::ObjectPtr getRef()
		{
			UniSetTypes::uniset_mutex_lock lock(refmutex, 300);
			return (UniSetTypes::ObjectPtr)CORBA::Object::_duplicate(oref);
		}

	protected:
			/*! обработка приходящих сообщений */
			virtual void processingMessage(UniSetTypes::VoidMessage *msg);

			/*! Получить сообщение */
			bool receiveMessage(UniSetTypes::VoidMessage& vm);
	
			/*! текущее количесво сообщений в очереди */
			unsigned int countMessages();
			 	
			/*! прервать ожидание сообщений */
			void termWaiting();

			UniversalInterface ui; /*!< универсальный интерфейс для работы с другими процессами */			
			std::string myname;
			std::string section;

			//! Дизактивизация объекта (переопределяется для необходимых действий перед деактивацией)	 
			virtual bool disactivateObject(){return true;}
			//! Активизация объекта (переопределяется для необходимых действий после активизации)	 
			virtual bool activateObject(){return true;}

			/*! запрет(разрешение) создания потока для обработки сообщений */
			inline void thread(bool create){ threadcreate = create; }
			/*! отключение потока обработки сообщений */
			inline void offThread(){ threadcreate = false; }
			/*! включение потока обработки сообщений */
			inline void onThread(){ threadcreate = true; }

			/*! функция вызываемая из потока */
			virtual void callback();

			/*! Функция вызываемая при приходе сигнала завершения или прерывания процесса. Переопределив ее можно
			 *	выполнять специфичные для процесса действия по обработке сигнала.
			 *	Например переход в безопасное состояние.
			 *  \warning В обработчике сигналов \b ЗАПРЕЩЕНО вызывать функции подобные exit(..), abort()!!!! 
			*/
			virtual void sigterm( int signo ){};

			inline void terminate(){ disactivate(); }

			/*! Ожидать сообщения timeMS */
			virtual bool waitMessage(UniSetTypes::VoidMessage& msg, timeout_t timeMS=UniSetTimer::WaitUpTime);

			void setID(UniSetTypes::ObjectId id);


			void setMaxSizeOfMessageQueue( unsigned int s )
			{
				if( s>=0 )
					SizeOfMessageQueue = s;
			}

			inline unsigned int getMaxSizeOfMessageQueue()
			{ return SizeOfMessageQueue; }
			
			void setMaxCountRemoveOfMessage( unsigned int m )
			{
				if( m >=0 )
					MaxCountRemoveOfMessage = m;
			}

			inline unsigned int getMaxCountRemoveOfMessage()
			{ return MaxCountRemoveOfMessage; }


			// функция определения приоритетного сообщения для обработки
			struct PriorVMsgCompare: 
				public std::binary_function<UniSetTypes::VoidMessage, UniSetTypes::VoidMessage, bool>
			{
				bool operator()(const UniSetTypes::VoidMessage& lhs, 
								const UniSetTypes::VoidMessage& rhs) const;
			};
			typedef std::priority_queue<UniSetTypes::VoidMessage,std::vector<UniSetTypes::VoidMessage>,PriorVMsgCompare> MessagesQueue;


			/*! Вызывается при переполнеии очереди сообщений (в двух местах push и receive)
				для очитски очереди.
				\warning По умолчанию удаляет из очереди все повторяющиеся 
				 - SensorMessage
				 - TimerMessage
				 - SystemMessage
			 Если не помогло удаляет из очереди UniSetObject::MaxCountRemoveOfMessage
			\note Для специфичной обработки может быть переопределена
			\warning Т.к. при фильтровании SensorMessage не смотрится значение, то
			при удалении сообщений об изменении аналоговых датчиков очистка может привести
			к некорректной работе фильрующих алгоритмов работающих с "выборкой" последних N значений.
			(потому-что останется одно последнее)
			*/
			virtual void cleanMsgQueue( MessagesQueue& q );


			void setRecvMutexTimeout( unsigned long msec );
			inline unsigned long getRecvMutexTimeout(){ return recvMutexTimeout; }
			
			void setPushMutexTimeout( unsigned long msec );
			unsigned long getPushMutexTimeout(){ return pushMutexTimeout; }

			bool isActive();
			void setActive( bool set );

			UniSetTypes::VoidMessage msg;	
			ObjectsManager* mymngr; 

			void setThreadPriority( int p );
			
	private:

			friend class ObjectsManager;
			friend class ObjectsActivator;
			friend class ThreadCreator<UniSetObject>;
			inline pid_t getMsgPID()
			{
				return msgpid;
			}

			/*! функция потока */
			void work();	
			//! Инициализация параметров объекта
			bool init(ObjectsManager* om);
			//! Прямая деактивизация объекта	 
			bool disactivate();
			//! Непосредственная активизация объекта
			bool activate();
			/* регистрация в репозитории объектов */
			void registered();
			/* удаление ссылки из репозитория объектов 	*/
			void unregister();

			void init_object();

			pid_t msgpid; // pid потока обработки сообщений
			bool reg;
			bool active;
			UniSetTypes::uniset_mutex act_mutex;
			bool threadcreate;
			UniSetTimer* tmr;
			UniSetTypes::ObjectId myid;
			CORBA::Object_var oref;
			ThreadCreator<UniSetObject>* thr;

			/*! очередь сообщений для объекта */
			MessagesQueue queueMsg;

		 	/*! замок для блокирования совместного доступа к очереди */
			UniSetTypes::uniset_mutex qmutex;

		 	/*! замок для блокирования совместного доступа к очереди */
			UniSetTypes::uniset_mutex refmutex;

			/*! размер очереди сообщений (при превышении происходит очистка) */
			unsigned int SizeOfMessageQueue;
			/*! сколько сообщений удалять при очисте*/
			unsigned int MaxCountRemoveOfMessage;
			unsigned long recvMutexTimeout; /*!< таймаут на ожидание освобождения mutex-а при receiveMessage */
			unsigned long pushMutexTimeout; /*!< таймаут на ожидание освобождения mutex-а при pushMessage */
			
			// статистическая информация 
			unsigned long stMaxQueueMessages;	/*<! Максимальное число сообщений хранившихся в очереди */
			unsigned long stCountOfQueueFull; 	/*! количество переполнений очереди сообщений */
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
