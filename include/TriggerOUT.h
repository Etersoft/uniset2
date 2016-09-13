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
// --------------------------------------------------------------------------
//---------------------------------------------------------------------------
#ifndef TriggerOUT_H_
#define TriggerOUT_H_
//---------------------------------------------------------------------------
#include <unordered_map>
//---------------------------------------------------------------------------
/*!
    \par Описание
        Этот триггер гарантирует, что значению "val" будет равен всегда только один выход (или ни одного).
    При помощи TriggerOUT::set(), на определённый выход подаётся "val"(val также может быть = "0"),
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
    #include "TriggerOUT.h"

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
    TriggerOUT<MyClass,int,int> tr_out(&rec, &MyClass::out);

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
template<class Caller, typename OutIdType = int, typename ValueType = bool>
class TriggerOUT
{
	public:

		/*! прототип функции вызова
		    \param out - идентификатор 'выхода'
		    \param val - новое значение
		*/
		typedef void(Caller::* Action)(OutIdType out, ValueType val);

		TriggerOUT(Caller* r, Action a) noexcept;
		~TriggerOUT() noexcept;


		/*! получить текущее значение указанного 'выхода' */
		bool getState(OutIdType out) const noexcept;

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
		void remove(OutIdType out) noexcept;

		void update();
		void reset();

	protected:
		void resetOuts( OutIdType outIgnore );

		typedef std::unordered_map<OutIdType, ValueType> OutList;
		OutList outs; // список выходов

		Caller* cal;
		Action act;

};

//---------------------------------------------------------------------------
#include "TriggerOUT.tcc"
//---------------------------------------------------------------------------
#endif
