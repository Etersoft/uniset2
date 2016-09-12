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
 *  \brief
 *  \author Pavel Vainerman
*/
// --------------------------------------------------------------------------
//---------------------------------------------------------------------------
#ifndef TRIGGER_AND_H_
#define TRIGGER_AND_H_
//---------------------------------------------------------------------------
#include <unordered_map>
//---------------------------------------------------------------------------
/*!
    Триггер \b "И", со множеством входов.
    Логика включения следующая: только "1" на \b ВСЕХ входах даёт на выходе "1", иначе "0".

    В конструкторе указывается функция, которая будет вызываться при \b ИЗМЕНЕНИИ состояния выхода.

    \warning Нет блокирования совместного доступа(не рассчитан на работу в многопоточной среде).

    \par Пример использования
    \code
    #include "TriggerAND.h"

    class MyClass
    {
        public:
            MyClass(){};
            ~MyClass(){};
            void out( bool newstate){ cout << "OR out state="<< newstate <<endl;}
        ...
    };

    ...
    MyClass rec;
    // Создание
    TriggerAND<MyClass, int> tr(&rec, &MyClass::out);

    // Добавление 'входов' (формирование списка входов)
    tr.add(1,false);
    tr.add(2,true);
    tr.add(3,true);
    tr.add(4,true);
    ...

    // Использование:
    // подаём на вход N1 единицу
    // после чего, при изменении состояния 'выхода' будет вызвана функция MyClass::out, в которой производится
    // фактическая обработка 'изменения состояния'

    tr.commit(1,true);

    \endcode

*/
template<class Caller, typename InputType = int>
class TriggerAND
{
	public:

		/*!
		    прототип функции вызова
		    \param newstate - новое состояние 'выхода'
		*/
		typedef void(Caller::* Action)(bool newstate);

		TriggerAND(Caller* r, Action a) noexcept;
		~TriggerAND() noexcept;

		inline bool state() const noexcept
		{
			return out;
		}


		bool getState(InputType in) const noexcept;
		bool commit(InputType in, bool state);

		void add(InputType in, bool state);
		void remove(InputType in);

		typedef std::unordered_map<InputType, bool> InputMap;

		inline typename InputMap::const_iterator begin() noexcept
		{
			return inputs.begin();
		}

		inline typename InputMap::const_iterator end() noexcept
		{
			return inputs.end();
		}

		void update();
		void reset();

	protected:
		void check();
		InputMap inputs; // список входов
		bool out;
		Caller* cal;
		Action act;
};

//---------------------------------------------------------------------------
#include "TriggerAND.tcc"
//---------------------------------------------------------------------------
#endif
