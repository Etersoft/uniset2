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
#include "TriggerOUT.h"
//---------------------------------------------------------------------------

template<class Caller, typename OutIdType, typename ValueType>
TriggerOUT<Caller,OutIdType,ValueType>::TriggerOUT( Caller* r, Action a) noexcept:
	cal(r),
	act(a)
{
}

template <class Caller, typename OutIdType, typename ValueType>
TriggerOUT<Caller,OutIdType,ValueType>::~TriggerOUT() noexcept
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
void TriggerOUT<Caller,OutIdType,ValueType>::remove(OutIdType num) noexcept
{
	try
	{
		auto it=outs.find(num);
		if( it!=outs.end() )
			outs.erase(it);
	}
	catch(...){}
}

//---------------------------------------------------------------------------
template <class Caller, typename OutIdType, typename ValueType>
bool TriggerOUT<Caller,OutIdType,ValueType>::getState(OutIdType out) const noexcept
{
	try
	{
		auto it=outs.find(out);
		if( it!=outs.end() )
			return it->second;
	}
	catch(...){}
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
