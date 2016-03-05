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
/**
  @defgroup UniSetExceptions Исключения
  @{
*/

namespace UniSetTypes
{
	class LimitTimers:
		public UniSetTypes::Exception
	{
		public:
			LimitTimers(): Exception("LimitTimers") {}

			/*! Конструктор позволяющий вывести в сообщении об ошибке дополнительную информацию err */
			LimitTimers(const std::string& err): Exception(err) {}
	};
}
//@}
// end of UniSetException group
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
                void Time(int id){ cout << "Timer id: "<< id << endl;}
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
		static const unsigned int MAXCallbackTimer = 20;

		/*! прототип функции вызова */
		typedef void(Caller::* Action)( int id );

		CallbackTimer(Caller* r, Action a);
		~CallbackTimer();

		// Управление таймером
		void run();        /*!< запуск таймера */
		void terminate();  /*!< остановка    */

		// Работа с таймерами (на основе интерфейса PassiveTimer)
		void reset(int id);                 /*!< перезапустить таймер */
		void setTiming(int id, int timrMS); /*!< установить таймер и запустить */
		int getInterval(int id);            /*!< получить интервал, на который установлен таймер, в мс */
		int getCurrent(int id);             /*!< получить текущее значение таймера */


		void add( int id, int timeMS )throw(UniSetTypes::LimitTimers); /*!< добавление нового таймера */
		void remove( int id ); /*!< удаление таймера */

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
			TimerInfo(int id, PassiveTimer& pt):
				id(id), pt(pt) {};

			int id;
			PassiveTimer pt;
		};

		typedef std::list<TimerInfo> TimersList;
		TimersList lst;

		// функция-объект для поиска по id
		struct FindId_eq: public std::unary_function<TimerInfo, bool>
		{
			FindId_eq(const int id): id(id) {}
			inline bool operator()(const TimerInfo& ti) const
			{
				return ti.id == id;
			}
			int id;
		};
};

#include "CallbackTimer.tcc"
# endif //CallbackTimer_H_
