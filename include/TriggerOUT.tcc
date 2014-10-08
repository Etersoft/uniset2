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
#include "TriggerOUT.h"
//---------------------------------------------------------------------------

template<class Caller, typename OutIdType, typename ValueType>
TriggerOUT<Caller,OutIdType,ValueType>::TriggerOUT( Caller* r, Action a):
	cal(r),
	act(a)
{
}

template <class Caller, typename OutIdType, typename ValueType>
TriggerOUT<Caller,OutIdType,ValueType>::~TriggerOUT()
{
}

//---------------------------------------------------------------------------
template <class Caller, typename OutIdType, typename ValueType>
void TriggerOUT<Caller,OutIdType,ValueType>::add(OutIdType num, ValueType val)
{
	outs[num] = val;
	set(num,val);

	resetOuts(num);	 // выставляем сперва все нули
	(cal->*act)(num,val); // потом выставляем указанный выход
}

//---------------------------------------------------------------------------
template <class Caller, typename OutIdType, typename ValueType>
void TriggerOUT<Caller,OutIdType,ValueType>::remove(OutIdType num)
{
	auto it=outs.find(num);
	if( it!=outs.end() )
		outs.erase(it);
}

//---------------------------------------------------------------------------
template <class Caller, typename OutIdType, typename ValueType>
bool TriggerOUT<Caller,OutIdType,ValueType>::getState(OutIdType out)
{
	auto it=outs.find(out);
	if( it!=outs.end() )
		return it->second;

	return false;
}
//---------------------------------------------------------------------------
template <class Caller, typename OutIdType, typename ValueType>
void TriggerOUT<Caller,OutIdType,ValueType>::set(OutIdType out, ValueType val)
{
	auto it=outs.find(out);
	if( it==outs.end() )
		return;

	// сперва сбрасываем все остальные выходы
	resetOuts(out);	 // выставляем сперва все нули

	// потом выставляем заданный выход
	ValueType prev = it->second;
	it->second = val;
	if( prev != val )
		(cal->*act)(it->first, it->second); 
}
//---------------------------------------------------------------------------
template <class Caller, typename OutIdType, typename ValueType>
void TriggerOUT<Caller,OutIdType,ValueType>::resetOuts( OutIdType outIgnore )
{
	for( auto it=outs.begin(); it!=outs.end(); ++it )
	{
		if( it->first != outIgnore && it->second )
		{
			it->second = 0;
			(cal->*act)(it->first, it->second);
		}
	}

}
//---------------------------------------------------------------------------
template <class Caller, typename OutIdType, typename ValueType>
void TriggerOUT<Caller,OutIdType,ValueType>::update()
{
	for( auto it=outs.begin(); it!=outs.end(); ++it )
		(cal->*act)(it->first, it->second);
}
//---------------------------------------------------------------------------
template <class Caller, typename OutIdType, typename ValueType>
void TriggerOUT<Caller,OutIdType,ValueType>::reset()
{
	for( auto it=outs.begin(); it!=outs.end(); ++it )
	{
		it->second = 0;
		(cal->*act)(it->first, it->second);
	}
}
//---------------------------------------------------------------------------
