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
 * \author Pavel Vainerman
 */
//---------------------------------------------------------------------------
#ifndef Object_LT_H_
#define Object_LT_H_
//--------------------------------------------------------------------------
#include <deque>
#include "Debug.h"
#include "UniSetTypes.h"
#include "MessageType.h"
#include "PassiveTimer.h"
#include "Exceptions.h"
//---------------------------------------------------------------------------
namespace uniset
{
	class UniSetObject;

	//---------------------------------------------------------------------------
	/*! \class LT_Object

	    \note '_LT' - это "local timers".
		 Класс реализующий механизм работы с локальными таймерами. Обеспечивает более надёжную работу
	    т.к. позволяет обходится без удалённого заказа таймеров у TimеService-а.
	    Но следует помнить, что при этом объект использующий такие таймеры становится более ресурсоёмким,
	    т.к. во время работы поток обработки сообщений не "спит", как у обычного UniSetObject-а, а тратит
	    время на проверку таймеров (правда при условии, что в списке есть хотя бы один заказ)

	    \par Основной принцип
	        Проверяет список таймеров и при срабатывании формирует стандартное уведомление uniset::TimerMessage,
		которое помещается в очередь указанному объекту. При проверке таймеров, определяется минимальное время оставшееся
	    до очередного срабатывания. Если в списке не остаётся ни одного таймера - возвращает UniSetTimers::WaitUpTime.

	    Примерный код использования выглядит так:

	    \code
	        class MyClass:
	            public UniSetObject
	        {
	            ...
	            int sleepTime;
	            UniSetObject_LT lt;
	            void callback();
	        }

	        void callback()
	        {
	            // При реализации с использованием waitMessage() каждый раз при вызове askTimer() необходимо
	            // проверять возвращаемое значение на UniSetTimers::WaitUpTime и вызывать termWaiting(),
	            // чтобы избежать ситуации, когда процесс до заказа таймера 'спал'(в функции waitMessage()) и после
	            // заказа продолжит спать (т.е. обработчик вызван не будет)...

	            try
	            {
	                if( waitMessage(msg, sleepTime) )
	                    processingMessage(&msg);

	                sleepTime=lt.checkTimers(this);
	            }
				catch( uniset::Exception& ex)
	            {
	                cout << myname << "(callback): " << ex << endl;
	            }
	        }

	        void askTimers()
	        {
	            // проверяйте возвращаемое значение
	            if( lt.askTimer(Timer1, 1000) != UniSetTimer::WaitUpTime )
	                termWaiting();
	        }

	    \endcode


	    \warning Точность работы определяется периодичностью вызова обработчика.
	    \sa TimerService

		\todo Подумать.. может перейти на unordered_map
	*/
	class LT_Object
	{
		public:
			LT_Object();
			virtual ~LT_Object();


			/*! заказ таймера
			    \param timerid - идентификатор таймера
			    \param timeMS - период. 0 - означает отказ от таймера
			    \param ticks - количество уведомлений. "-1" - постоянно
			    \param p - приоритет присылаемого сообщения
			    \return Возвращает время [мсек] оставшееся до срабатывания очередного таймера
			*/
			virtual timeout_t askTimer( uniset::TimerId timerid, timeout_t timeMS, clock_t ticks = -1,
										uniset::Message::Priority p = uniset::Message::High );


			/*!
			    основная функция обработки.
			    \param obj - указатель на объект, которому посылается уведомление
			    \return Возвращает время [мсек] оставшееся до срабатывания очередного таймера
			*/
			timeout_t checkTimers( UniSetObject* obj );

			/*! получить время на которое установлен таймер timerid
			 * \param timerid - идентификатор таймера
			 * \return 0 - если таймер не найден, время (мсек) если таймер есть.
			 */
			timeout_t getTimeInterval( uniset::TimerId timerid ) const;

			/*! получить оставшееся время для таймера timerid
			 * \param timerid - идентификатор таймера
			 * \return 0 - если таймер не найден, время (мсек) если таймер есть.
			 */
			timeout_t getTimeLeft( uniset::TimerId timerid ) const;

		protected:

			/*! пользовательская функция для вывода названия таймера */
			virtual std::string getTimerName( int id ) const;

			/*! Информация о таймере */
			struct TimerInfo
			{
				TimerInfo() {};
				TimerInfo( uniset::TimerId id, timeout_t timeMS, clock_t cnt, uniset::Message::Priority p ):
					id(id),
					curTimeMS(timeMS),
					priority(p),
					curTick(cnt - 1)
				{
					tmr.setTiming(timeMS);
				};

				inline void reset()
				{
					curTimeMS = tmr.getInterval();
					tmr.reset();
				}

				uniset::TimerId id = { 0 };    /*!<  идентификатор таймера */
				timeout_t curTimeMS = { 0 };   /*!<  остаток времени */
				uniset::Message::Priority priority = { uniset::Message::High }; /*!<  приоритет посылаемого сообщения */

				/*!
				 * текущий такт
				 * \note Если задано количество -1 то сообщения будут посылаться постоянно
				*/
				clock_t curTick = { -1 };

				// таймер с меньшим временем ожидания имеет больший приоритет
				bool operator < ( const TimerInfo& ti ) const
				{
					return curTimeMS > ti.curTimeMS;
				}

				PassiveTimer tmr;
			};

			class Timer_eq: public std::unary_function<TimerInfo, bool>
			{
				public:
					Timer_eq(uniset::TimerId t): tid(t) {}

					inline bool operator()(const TimerInfo& ti) const
					{
						return ( ti.id == tid );
					}

				protected:
					uniset::TimerId tid;
			};

			typedef std::deque<TimerInfo> TimersList;

			timeout_t sleepTime; /*!< текущее время ожидания */

			TimersList getTimersList() const;

		private:
			TimersList tlst;

			/*! замок для блокирования совместного доступа к списку таймеров */
			mutable uniset::uniset_rwmutex lstMutex;
			PassiveTimer tmLast;

			Debug::type loglevel = { Debug::LEVEL3 };
	};
	// -------------------------------------------------------------------------
} // end of uniset namespace
//--------------------------------------------------------------------------
#endif
