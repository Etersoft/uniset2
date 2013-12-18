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
 *  \author Pavel Vainerman
*/
//----------------------------------------------------------------------------
# ifndef CallBackTimer_H_
# define CallBackTimer_H_
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
            LimitTimers():Exception("LimitTimers"){ printException(); }
    
            /*! Конструктор позволяющий вывести в сообщении об ошибке дополнительную информацию err */
            LimitTimers(const std::string err):Exception(err){ printException(); }
    };
};
//@}
// end of UniSetException group
//----------------------------------------------------------------------------------------

/*! 
 * \brief Таймер 
 * \author Pavel Vainerman
 * \par
 * Создает поток, в котором происходит отсчет тактов (10ms). Позволяет заказывать до CallBackTimer::MAXCallBackTimer таймеров.
 * При срабатывании будет вызвана указанная функция с указанием \b Id таймера, который сработал. 
 * Функция обратного вызова должна удовлетворять шаблону CallBackTimer::Action.
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
         CallBackTimer<MyClass> *timer1 = new CallBackTimer<MyClass>(rec);
        timer1->add(1, &MyClass::Time, 1000);
        timer1->add(5, &MyClass::Time, 1200);
        timer1->run();    
    \endcode
 *
 * \note Каждый экземпляр класса CallBackTimer создает поток, поэтому \b желательно не создавать больше одного экземпляра,
 * для одного процесса (чтобы не порождать много потоков).
*/ 
template <class Caller>
class CallBackTimer
//    public PassiveTimer
{ 
    public:

        /*! Максимальное количество таймеров */
        static const int MAXCallBackTimer = 20;

        /*! прототип функции вызова */
        typedef void(Caller::* Action)( int id );    

        CallBackTimer(Caller* r, Action a);
        ~CallBackTimer();

        // Управление таймером
        void run();            /*!< запуск таймера */
        void terminate();    /*!< остановка    */
        
        // Работа с таймерами (на основе интерфейса PassiveTimer)
        void reset(int id);                    /*!< перезапустить таймер */
        void setTiming(int id, int timrMS);    /*!< установить таймер и запустить */
        int getInterval(int id);            /*!< получить интервал, на который установлен таймер, в мс */
        int getCurrent(int id);                /*!< получить текущее значение таймера */


        void add( int id, int timeMS )throw(UniSetTypes::LimitTimers); /*!< добавление нового таймера */
        void remove( int id ); /*!< удаление таймера */
                
    protected:

        CallBackTimer();
        void work();
        
        void startTimers();
        void clearTimers();

    private:

        typedef CallBackTimer<Caller> CBT;
        friend class ThreadCreator<CBT>;
        Caller* cal;
        Action act;
        ThreadCreator<CBT> *thr;
        
        bool terminated;

        struct TimerInfo
        {
            TimerInfo(int id, PassiveTimer& pt):
                id(id), pt(pt){};

            int id;
            PassiveTimer pt;
        };
        
        typedef std::list<TimerInfo> TimersList;
        TimersList lst;

        // функция-объект для поиска по id
        struct FindId_eq: public unary_function<TimerInfo, bool>
        {
            FindId_eq(const int id):id(id){}
            inline bool operator()(const TimerInfo& ti) const{return ti.id==id;}
            int id;
        };
};

#include "CallBackTimer.tcc"
# endif //CallBackTimer_H_
