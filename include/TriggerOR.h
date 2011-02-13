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
#ifndef TRIGGER_OR_H_
#define TRIGGER_OR_H_
//---------------------------------------------------------------------------
#include <map>
//---------------------------------------------------------------------------
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
	TriggerOR<MyClass, int> tr_or(&rec, &MyClass::out);
	
	// Добавление 'входов'
	tr_or.add(1,true);
	tr_or.add(2,false);
	tr_or.add(3,false);
	tr_or.add(4,false);
	...
	// Использование
	// подаёт на вход N1 "0"
	// после чего, при изменении состояния 'выхода' будет вызвана функция MyClass::out, в которой производится 
	// фактическая обработка 'изменения состояния'
	tr_or.commit(1,false);
	\endcode
*/
template<class Caller, typename InputType>
class TriggerOR
{
	public:

		/*! 
			прототип функции вызова 
			\param newstate - новое состояние 'выхода'
		*/
		typedef void(Caller::* Action)(bool newstate);	
	
		TriggerOR(Caller* r, Action a);
		~TriggerOR();
		
		inline bool state(){ return out; }
		

		bool getState(InputType in);
		bool commit(InputType in, bool state);

		void add(InputType in, bool state);
		void remove(InputType in);

		typedef std::map<InputType, bool> InputMap;

		inline typename InputMap::const_iterator begin()
		{
			return inputs.begin();
		}

		inline typename InputMap::const_iterator end()
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
#include "TriggerOR_template.h"
//---------------------------------------------------------------------------
#endif
