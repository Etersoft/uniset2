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
#ifndef TRIGGER_OR_H_
#define TRIGGER_OR_H_
//---------------------------------------------------------------------------
#include <unordered_map>
//---------------------------------------------------------------------------
namespace uniset
{
	/*!
	    Триггер \b "ИЛИ", со множеством входов.
	    Логика включения следующая:
	    - "1" на любом входе даёт на выходе "1"
	    - "0" на \b ВСЕХ входах даёт на выходе "0"

	    В конструкторе указывается функция, которая будет вызываться при \b ИЗМЕНЕНИИ состояния выхода.

	    \warning Нет блокирования совместного доступа(не рассчитан на работу в многопоточной среде).

	    \par Пример использования
	    \code
	    #include "TriggerOR.h"
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
	    TriggerOR<MyClass> tr(&rec, &MyClass::out);

	    // Добавление 'входов'
	    tr.add(1,true);
	    tr.add(2,false);
	    tr.add(3,false);
	    tr.add(4,false);
	    ...

	    // Использование:
	    // подаём на вход N1 "0"
	    // после чего, при изменении состояния 'выхода' будет вызвана функция MyClass::out, в которой производится
	    // фактическая обработка 'изменения состояния'

	    tr.commit(1,false);

	    \endcode
	*/
	template<class Caller, typename InputType = int>
	class TriggerOR
	{
		public:

			/*!
			    прототип функции вызова
			    \param newstate - новое состояние 'выхода'
			*/
			typedef void(Caller::* Action)(bool newstate);

			TriggerOR(Caller* r, Action a) noexcept;
			~TriggerOR() noexcept;

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
			bool out = { false };
			Caller* cal;
			Action act;
	};
	// -------------------------------------------------------------------------
#include "TriggerOR.tcc"
	//---------------------------------------------------------------------------
} // end of uniset namespace
//---------------------------------------------------------------------------
#endif
