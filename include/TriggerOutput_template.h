/* This file is part of the UniSet project
 * Copyright (c) 2002 Free Software Foundation, Inc.
 * Copyright (c) 2002 Pavel Vainerman <pv>
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
 *  \author Pavel Vainerman <pv>
 *  \date   $Date: 2008/06/01 21:36:19 $
 *  \version $Id: TriggerOutput_template.h,v 1.4 2008/06/01 21:36:19 vpashka Exp $
*/
// --------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "TriggerOutput.h"
//---------------------------------------------------------------------------

template<class Caller, typename OutIdType, typename ValueType>
TriggerOutput<Caller,OutIdType,ValueType>::TriggerOutput( Caller* r, Action a):
	cal(r),
	act(a)
{
}

template <class Caller, typename OutIdType, typename ValueType>
TriggerOutput<Caller,OutIdType,ValueType>::~TriggerOutput()
{
}

//---------------------------------------------------------------------------
template <class Caller, typename OutIdType, typename ValueType>
void TriggerOutput<Caller,OutIdType,ValueType>::add(OutIdType num, ValueType val)
{
	outs[num] = val; 
	set(num,val);
	try
	{
		(cal->*act)(num,val);
	}
	catch(...){}
}

//---------------------------------------------------------------------------
template <class Caller, typename OutIdType, typename ValueType>
void TriggerOutput<Caller,OutIdType,ValueType>::remove(OutIdType num)
{
	typename OutList::iterator it=outs.find(num);
	if( it!=outs.end() )
		outs.erase(it);
}

//---------------------------------------------------------------------------
template <class Caller, typename OutIdType, typename ValueType>
bool TriggerOutput<Caller,OutIdType,ValueType>::getState(OutIdType out)
{
	typename OutList::iterator it=outs.find(out);
	if( it!=outs.end() )
		return it->second;

	return false;
}
//---------------------------------------------------------------------------
template <class Caller, typename OutIdType, typename ValueType>
void TriggerOutput<Caller,OutIdType,ValueType>::set(OutIdType out, ValueType val)
{
	typename OutList::iterator it=outs.find(out);
	if( it==outs.end() )
		return;

	// потом val
	ValueType prev(it->second);
	it->second = val;
	if( prev != val )
	{
		check(out);	 // выставляем сперва все нули
		try
		{
			(cal->*act)(it->first, it->second);
		}
		catch(...){}
	}
}
//---------------------------------------------------------------------------
template <class Caller, typename OutIdType, typename ValueType>
void TriggerOutput<Caller,OutIdType,ValueType>::check(OutIdType newout)
{
	for( typename OutList::iterator it=outs.begin(); it!=outs.end(); ++it )
	{
		if( it->first != newout && it->second )
		{
			it->second = 0;
//			try
//			{
				(cal->*act)(it->first, it->second);
//			}
//			catch(...){}
		}
	}

}
//---------------------------------------------------------------------------
template <class Caller, typename OutIdType, typename ValueType>
void TriggerOutput<Caller,OutIdType,ValueType>::update()
{
	for( typename OutList::iterator it=outs.begin(); it!=outs.end(); ++it )
		(cal->*act)(it->first, it->second);
}
//---------------------------------------------------------------------------
template <class Caller, typename OutIdType, typename ValueType>
void TriggerOutput<Caller,OutIdType,ValueType>::reset()
{
	for( typename OutList::iterator it=outs.begin(); it!=outs.end(); ++it )
	{
		it->second = 0;
		(cal->*act)(it->first, it->second);
	}
}
//---------------------------------------------------------------------------
