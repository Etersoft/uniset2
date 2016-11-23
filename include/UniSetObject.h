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
#include "MQMutex.h"
#include "UHttpRequestHandler.h"

//---------------------------------------------------------------------------
namespace uniset
{
//---------------------------------------------------------------------------
class UniSetActivator;
class UniSetManager;

//---------------------------------------------------------------------------
class UniSetObject;
typedef std::list< std::shared_ptr<UniSetObject> > ObjectsList;     /*!< Список подчиненных объектов */
//---------------------------------------------------------------------------
/*! \class UniSetObject
 *  Класс реализует работу uniset-объекта: работа с очередью сообщений, регистрация объекта, инициализация и т.п.
 *	Обработка сообщений ведётся в специально создаваемом потоке.
 *  Для ожидания сообщений используется функция waitMessage(msec), основанная на таймере.
 *  Ожидание прерывается либо по истечении указанного времени, либо по приходу сообщения, при помощи функциии
 *  termWaiting() вызываемой из push().
 *  \note Если не будет задан ObjectId(-1), то поток обработки запущен не будет.
 *  Также создание потока можно принудительно отключить при помощи функции void thread(false). Ее необходимо вызвать до активации объекта
 *  (например в конструкторе). При этом ответственность за вызов receiveMessage() и processingMessage() возлагается
 *  на разработчика.
 *
 *	Имеется три очереди сообщений, по приоритетам: Hi, Medium, Low.
 * Соответственно сообщения вынимаются в порядке поступления, но сперва из Hi, потом из Medium, а потом из Low очереди.
 * \warning Если сообщения будут поступать в Hi или Medium очередь быстрее чем они обрабатываются, то до Low сообщений дело может и не дойти.
 *
*/
class UniSetObject:
	public std::enable_shared_from_this<UniSetObject>,
	public POA_UniSetObject_i,
	public LT_Object
#ifndef DISABLE_REST_API
	,public uniset::UHttp::IHttpRequest
#endif
{
	public:
		UniSetObject( const std::string& name, const std::string& section );
		UniSetObject( uniset::ObjectId id );
		UniSetObject();
		virtual ~UniSetObject();

		std::shared_ptr<UniSetObject> get_ptr();

		// Функции объявленные в IDL
		virtual CORBA::Boolean exist() override;

		virtual uniset::ObjectId getId() override;

		const uniset::ObjectId getId() const;
		std::string getName() const;

		virtual uniset::ObjectType getType() override
		{
			return uniset::ObjectType("UniSetObject");
		}

		virtual uniset::SimpleInfo* getInfo( const char* userparam = 0 ) override;

		//! поместить сообщение в очередь
		virtual void push( const uniset::TransportMessage& msg ) override;

#ifndef DISABLE_REST_API
		// HTTP API
		virtual nlohmann::json httpGet( const Poco::URI::QueryParameters& p ) override;
		virtual nlohmann::json httpHelp( const Poco::URI::QueryParameters& p ) override;
#endif
		// -------------- вспомогательные --------------
		/*! получить ссылку (на себя) */
		uniset::ObjectPtr getRef() const;

		/*! заказ таймера (вынесена в public, хотя должна была бы быть в protected */
		virtual timeout_t askTimer( uniset::TimerId timerid, timeout_t timeMS, clock_t ticks = -1,
									uniset::Message::Priority p = uniset::Message::High ) override;

		friend std::ostream& operator<<(std::ostream& os, UniSetObject& obj );

	protected:

		std::shared_ptr<UInterface> ui; /*!< универсальный интерфейс для работы с другими процессами */
		std::string myname;
		std::string section;
		std::weak_ptr<UniSetManager> mymngr;

		/*! обработка приходящих сообщений */
		virtual void processingMessage( const uniset::VoidMessage* msg );

		// конкретные виды сообщений
		virtual void sysCommand( const uniset::SystemMessage* sm ) {}
		virtual void sensorInfo( const uniset::SensorMessage* sm ) {}
		virtual void timerInfo( const uniset::TimerMessage* tm ) {}

		/*! Получить сообщение */
		VoidMessagePtr receiveMessage();

		/*! Ожидать сообщения заданное время */
		virtual VoidMessagePtr waitMessage( timeout_t msec = UniSetTimer::WaitUpTime );

		/*! прервать ожидание сообщений */
		void termWaiting();

		/*! текущее количесво сообщений в очереди */
		size_t countMessages();

		/*! количество потерянных сообщений */
		size_t getCountOfLostMessages() const;

		//! Активизация объекта (переопределяется для необходимых действий после активизации)
		virtual bool activateObject();

		//! Деактивиция объекта (переопределяется для необходимых действий перед деактивацией)
		virtual bool deactivateObject();

		/*! Функция вызываемая при приходе сигнала завершения или прерывания процесса. Переопределив ее можно
		 *    выполнять специфичные для процесса действия по обработке сигнала.
		 *    Например переход в безопасное состояние.
		 *  \warning В обработчике сигналов \b ЗАПРЕЩЕНО вызывать функции подобные exit(..), abort()!!!!
		*/
		virtual void sigterm( int signo );

		void terminate();

		// управление созданием потока обработки сообщений -------

		/*! запрет(разрешение) создания потока для обработки сообщений */
		void thread( bool create );

		/*! отключение потока обработки сообщений */
		void offThread();

		/*! включение потока обработки сообщений */
		void onThread();

		/*! функция вызываемая из потока */
		virtual void callback();

		// ----- конфигурирование объекта -------
		/*! установка ID объекта */
		void setID(uniset::ObjectId id);

		/*! установить приоритет для потока обработки сообщений (если позволяют права и система) */
		void setThreadPriority( Poco::Thread::Priority p );

		/*! установка размера очереди сообщений */
		void setMaxSizeOfMessageQueue( size_t s );

		/*! получить размер очереди сообщений */
		size_t getMaxSizeOfMessageQueue() const;

		/*! проверка "активности" объекта */
		bool isActive() const;

		/*! false - завершить работу потока обработки сообщений */
		void setActive( bool set );

	private:

		friend class UniSetManager;
		friend class UniSetActivator;

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
		uniset::ObjectId myid;
		CORBA::Object_var oref;

		/*! замок для блокирования совместного доступа к oRef */
		mutable uniset::uniset_rwmutex refmutex;

		std::shared_ptr< ThreadCreator<UniSetObject> > thr;

		/*! очереди сообщений в зависимости от приоритета */
		MQMutex mqueueLow;
		MQMutex mqueueMedium;
		MQMutex mqueueHi;

		std::atomic_bool a_working;
		std::mutex    m_working;
		std::condition_variable cv_working;
};
// -------------------------------------------------------------------------
} // end of uniset namespace
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
