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
#include "TriggerAND.h"
//---------------------------------------------------------------------------
template<class Caller, typename InputType>
TriggerAND<Caller,InputType>::TriggerAND(Caller* c, Action a) noexcept:
out(false),
cal(c),
act(a)
{
}

template<class Caller, typename InputType>
TriggerAND<Caller,InputType>::~TriggerAND() noexcept
{
}

//---------------------------------------------------------------------------
template<class Caller, typename InputType>
bool TriggerAND<Caller,InputType>::commit(InputType num, bool state)
{
	typename InputMap::iterator it=inputs.find(num);
	if( it!=inputs.end() )
	{
		inputs[num] = state;
		check();
		return true;
	}

	return false;
}

//---------------------------------------------------------------------------
template<class Caller, typename InputType>
void TriggerAND<Caller,InputType>::add(InputType num, bool state)
{
	inputs[num] = state;
	check();
}

//---------------------------------------------------------------------------
template<class Caller, typename InputType>
void TriggerAND<Caller,InputType>::remove(InputType num)
{
	typename InputMap::iterator it=inputs.find(num);
	if( it!=inputs.end() )
		inputs.erase(it);

	check();
}

//---------------------------------------------------------------------------
template<class Caller, typename InputType>
bool TriggerAND<Caller,InputType>::getState(InputType num) const noexcept
{
	try
	{
		auto it=inputs.find(num);
		if( it!=inputs.end() )
			return it->second;
	}
	catch(...){}

	return false; // throw NotFound
}
//---------------------------------------------------------------------------
template<class Caller, typename InputType>
void TriggerAND<Caller,InputType>::check()
{
	bool old = out;
	for( auto it=inputs.begin(); it!=inputs.end(); ++it )
	{
		if( !it->second )
		{
			// если хоть один вход "0" выставляем на выходе "0"
			// и прекращаем дальнейший поиск
			out = false;
			if( old != out )
				(cal->*act)(out);
				
			return;
		}
	}

	out = true;
	if( old != out )
		(cal->*act)(out);
}
//---------------------------------------------------------------------------
template<class Caller, typename InputType>
void TriggerAND<Caller,InputType>::update()
{
	(cal->*act)(out);
}
//---------------------------------------------------------------------------
template<class Caller, typename InputType>
void TriggerAND<Caller,InputType>::reset()
{
	out = false;
	(cal->*act)(out);
}
//---------------------------------------------------------------------------
