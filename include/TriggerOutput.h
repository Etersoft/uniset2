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
// --------------------------------------------------------------------------
//---------------------------------------------------------------------------
#ifndef TriggerOutput_H_
#define TriggerOutput_H_
//---------------------------------------------------------------------------
#include <map>
//---------------------------------------------------------------------------
/*!
    \par Описание
        Этот триггер гарантирует, что значению "val" будет равен всегда только один выход (или ни одного). 
    При помощи TriggerOutput::set(), на определённый выход подаётся "val"(val также может быть = "0"), 
    при этом все остальные выходы выставляются в "0". 
        Работает в следующей последовательности: сперва выставляются все выходы в "0", а потом 
    уже нужный (единственный) выход выставляется в "val".

    Полезен для случаев, когда, например, команда "включить" и "отключить" подается на разные выходы.
    В этом случае тригер не позволяет иметь на выходах противоречивое состояние. 
    
    В конструкторе указывается функция, которая будет вызываться при ИЗМЕНЕНИИ состояния того, 
    или иного выхода.
    
    \warning Нет блокирования совместного доступа(не рассчитан на работу в многопоточной среде).

    \par Пример использования
    \code
    #include "TriggerOutput.h"
    
    class MyClass
    {
        public:
            MyClass(){};
            ~MyClass(){};
        
            void out(int out_id, int val){ cout << "TriggerOUT out="<< out_id << " val=" << val <<endl;}            
        ...
    };

    ...
    MyClass rec;
    // Создание:
    TriggerOutput<MyClass,int,int> tr_out(&rec, &MyClass::out);

    // формируем OUT триггер с двумя 'выходами'
    tr_out.add(1,0);
    tr_out.add(2,0);

    ...
    // Использование:
    // подаём сперва на первый 'выход' значение, второй должен стать в "0",
    // потом на другой...

    tr_out.set(1,4);

    cout << ( tr_out.getState(2) !=0 ? "FAIL" : "OK" ) << endl;

    tr_out.set(2,3);

    cout << ( tr_out.getState(1) !=0 ? "FAIL" : "OK" ) << endl;

    \endcode
*/
template<class Caller, typename OutIdType, typename ValueType>
class TriggerOutput
{
    public:

        /*! прототип функции вызова 
            \param out - идентификатор 'выхода'
            \param val - новое значение
        */
        typedef void(Caller::* Action)(OutIdType out, ValueType val);
    
        TriggerOutput(Caller* r, Action a);
        ~TriggerOutput();


        /*! получить текущее значение указанного 'выхода' */
        bool getState(OutIdType out);

        /*! установить значение одного из 'выходов'
            \param out - идентификатор 'выхода'
            \param val - новое значение
        */
        void set(OutIdType out, ValueType val);

        /*! добавить новый 'выход' и установить начальное значение.
            \param out - идентификатор 'выхода'
            \param val - новое значение
        */
        void add(OutIdType out, ValueType val);

        /*! удалить указанный 'выход' */
        void remove(OutIdType out);

        void update();
        void reset();

    protected:
        void check(OutIdType newout);
    
        typedef std::map<OutIdType, ValueType> OutList;
        OutList outs; // список выходов
        
        Caller* cal;
        Action act;
        
};

//---------------------------------------------------------------------------
#include "TriggerOutput.tcc"
//---------------------------------------------------------------------------
#endif
