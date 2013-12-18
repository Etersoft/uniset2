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
 * \author Pavel Vainerman
 */
//---------------------------------------------------------------------------
#ifndef Object_LT_H_
#define Object_LT_H_
//--------------------------------------------------------------------------
#include <list>
#include "UniSetTypes.h"
#include "MessageType.h"
#include "PassiveTimer.h"
#include "Exceptions.h"

//---------------------------------------------------------------------------
class UniSetObject;
//---------------------------------------------------------------------------
/*! \class LT_Object

    \note '_LT' - это "local timers".
     Класс реализующий механиз локальных таймеров. Обеспечивает более надёжную работу
    т.к. позволяет обходится без удалённого заказа таймеров у TimеServic-а.
    Но следует помнить, что при этом объект использующий такие таймеры становится более ресурсоёмким,
    т.к. во время работы поток обработки сообщений не "спит", как у обычного UniSetObject-а, а тратит
    время на проверку таймеров (правда при условии, что в списке есть хотябы один заказ)

    \par Основной принцип
        Проверяет список таймеров у при срабатывании формирует стандартное уведомление UniSetTypes::TimerMessage,
    которое помещается в очередь указанному объекту. Каждый раз пересортировывает список и возвращает время оставшееся
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
            // заказа продолжит спать(т.е. обработчик вызван не будет)...

            try
            {
                if( waitMessage(msg, sleepTime) )
                    processingMessage(&msg);

                sleepTime=lt.checkTimers(this);
            }
            catch(Exception& ex)
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


    \warning Точность работы определяется переодичснотью вызова обработичика.
    \sa TimerService
*/ 
class LT_Object
{
    public:
        LT_Object();
        virtual ~LT_Object();


        /*! заказ таймера
            \param timerid - идентификатор таймера
            \param timeMS - период. 0 - означает отказ от таймера
            \param ticks - количество уведомлений. "-1"- постоянно
            \param p - приоритет присылаемого сообщения
            \return Возвращает время [мсек] оставшееся до срабатывания
            очередного таймера
        */
        timeout_t askTimer( UniSetTypes::TimerId timerid, timeout_t timeMS, clock_t ticks=-1,
                        UniSetTypes::Message::Priority p=UniSetTypes::Message::High );


        /*!
            основная функция обработки.
            \param obj - указатель на объект, которому посылается уведомление
            \return Возвращает время [мсек] оставшееся до срабатывания
                очередного таймера
        */
        timeout_t checkTimers( UniSetObject* obj );

        /*! получить текущее время ожидания */
        //inline timeout_t getSleepTimeMS(){ return sleepTime; }

    protected:

        /*! Информация о таймере */
        struct TimerInfo
        {
            TimerInfo():id(0), curTimeMS(0), priority(UniSetTypes::Message::High){};
            TimerInfo(UniSetTypes::TimerId id, timeout_t timeMS, short cnt, UniSetTypes::Message::Priority p):
                id(id),
                curTimeMS(timeMS),
                priority(p),
                curTick(cnt-1)
            {
                tmr.setTiming(timeMS);
            };

            inline void reset()
            {
                curTimeMS = tmr.getInterval();
                tmr.reset();
            }

            UniSetTypes::TimerId id;    /*!<  идентификатор таймера */
            timeout_t curTimeMS;                /*!<  остаток времени */
            UniSetTypes::Message::Priority priority; /*!<  приоритет посылаемого сообщения */

            /*!
             * текущий такт
             * \note Если задано количество -1 то сообщения будут поылатся постоянно
            */
            clock_t curTick;

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
                Timer_eq(UniSetTypes::TimerId t):tid(t){}

            inline bool operator()(const TimerInfo& ti) const
            {
                return ( ti.id == tid );
            }

            protected:
                UniSetTypes::TimerId tid;
        };

        typedef std::list<TimerInfo> TimersList;

    private:
        TimersList tlst;
        /*! замок для блокирования совместного доступа к cписку таймеров */
        UniSetTypes::uniset_rwmutex lstMutex;
        timeout_t sleepTime; /*!< текущее время ожидания */
        PassiveTimer tmLast;
};
//--------------------------------------------------------------------------
#endif
