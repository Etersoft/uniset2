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
 *  \author Pavel Vainerman
*/
//----------------------------------------------------------------------------
# ifndef CallbackTimer_H_
# define CallbackTimer_H_
//----------------------------------------------------------------------------
#include <list>
#include "Exceptions.h"
#include "ThreadCreator.h"
#include "PassiveTimer.h"
//-----------------------------------------------------------------------------
namespace uniset
{
	class LimitTimers:
		public uniset::Exception
	{
		public:
			LimitTimers(): Exception("LimitTimers") {}

			/*! Конструктор позволяющий вывести в сообщении об ошибке дополнительную информацию err */
			LimitTimers(const std::string& err): Exception(err) {}
	};
	//----------------------------------------------------------------------------------------

	/*!
	 * \brief Таймер
	 * \author Pavel Vainerman
	 * \par
	 * Создает поток, в котором происходит отсчет тактов (10ms). Позволяет заказывать до CallbackTimer::MAXCallbackTimer таймеров.
	 * При срабатывании будет вызвана указанная функция с указанием \b Id таймера, который сработал.
	 * Функция обратного вызова должна удовлетворять шаблону CallbackTimer::Action.
	 * Пример создания таймера:
	 *
	    \code
	        class MyClass
	        {
	            public:
					void Time(size_t id){ cout << "Timer id: "<< id << endl;}
	        };

	        MyClass* rec = new MyClass();
	         ...
	         CallbackTimer<MyClass> *timer1 = new CallbackTimer<MyClass>(rec);
	        timer1->add(1, &MyClass::Time, 1000);
	        timer1->add(5, &MyClass::Time, 1200);
	        timer1->run();
	    \endcode
	 *
	 * \note Каждый экземпляр класса CallbackTimer создает поток, поэтому \b желательно не создавать больше одного экземпляра,
	 * для одного процесса (чтобы не порождать много потоков).
	*/
	template <class Caller>
	class CallbackTimer
	//    public PassiveTimer
	{
		public:

			/*! Максимальное количество таймеров */
			static const size_t MAXCallbackTimer = 20;

			/*! прототип функции вызова */
			typedef void(Caller::* Action)( size_t id );

			CallbackTimer(Caller* r, Action a);
			~CallbackTimer();

			// Управление таймером
			void run();        /*!< запуск таймера */
			void terminate();  /*!< остановка    */

			// Работа с таймерами (на основе интерфейса PassiveTimer)
			void reset(size_t id);                 /*!< перезапустить таймер */
			void setTiming(size_t id, timeout_t timrMS); /*!< установить таймер и запустить */
			timeout_t getInterval(size_t id);            /*!< получить интервал, на который установлен таймер, в мс */
			timeout_t getCurrent(size_t id);             /*!< получить текущее значение таймера */


			void add( size_t id, timeout_t timeMS )throw(uniset::LimitTimers); /*!< добавление нового таймера */
			void remove( size_t id ); /*!< удаление таймера */

		protected:

			CallbackTimer();
			void work();

			void startTimers();
			void clearTimers();

		private:

			typedef CallbackTimer<Caller> CBT;
			friend class ThreadCreator<CBT>;
			Caller* cal;
			Action act;
			ThreadCreator<CBT>* thr;

			bool terminated;

			struct TimerInfo
			{
				TimerInfo(size_t id, PassiveTimer& pt):
					id(id), pt(pt) {};

				size_t id;
				PassiveTimer pt;
			};

			typedef std::list<TimerInfo> TimersList;
			TimersList lst;

			// функция-объект для поиска по id
			struct FindId_eq: public std::unary_function<TimerInfo, bool>
			{
				FindId_eq(const size_t id): id(id) {}
				inline bool operator()(const TimerInfo& ti) const
				{
					return ti.id == id;
				}
				size_t id;
			};
	};
	//----------------------------------------------------------------------------------------
#include "CallbackTimer.tcc"
	//----------------------------------------------------------------------------------------
} // end of uniset namespace
//----------------------------------------------------------------------------------------
# endif //CallbackTimer_H_
