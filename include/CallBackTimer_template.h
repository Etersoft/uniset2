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
 *  \date   $Date: 2008/02/07 21:04:56 $
 *  \version $Id: CallBackTimer_template.h,v 1.6 2008/02/07 21:04:56 vpashka Exp $
*/
// -------------------------------------------------------------------------- 
# ifndef CallBackTimer_TEMPLATE_H_
# define CallBackTimer_TEMPLATE_H_
// -------------------------------------------------------------------------- 
#include <unistd.h>
#include <sstream>
#include "CallBackTimer.h"

// ------------------------------------------------------------------------------------------
template <class Caller> class CallBackTimer;

// ------------------------------------------------------------------------------------------
/*! Создание таймера
	\param r - указатель на заказчика
*/
template <class Caller>
CallBackTimer<Caller>::CallBackTimer( Caller* r, Action a ):
	cal(r),
	act(a),
	terminated(false)
{
	thr = new ThreadCreator<CallBackTimer>(this, &CallBackTimer<Caller>::work);
}

// ------------------------------------------------------------------------------------------

template <class Caller>
CallBackTimer<Caller>::CallBackTimer():
	cal(null),
	terminated(false)
{
	thr = new ThreadCreator<CallBackTimer>(this, &CallBackTimer<Caller>::work);
}
// ------------------------------------------------------------------------------------------
template <class Caller>
CallBackTimer<Caller>::~CallBackTimer()
{
	terminate();
	clearTimers();
	delete thr;
}

// ------------------------------------------------------------------------------------------
template <class Caller>
void CallBackTimer<Caller>::work()
{
	terminated = false;
	while( !terminated )
	{
		usleep(UniSetTimer::MIN_QUANTITY_TIME_MKS); 

		for( typename TimersList::iterator li=lst.begin(); li!=lst.end(); ++li )
		{
			if( li->pt.checkTime() )
			{
				(cal->*act)( li->id );
				li->pt.reset();
			}
		}

	}
}
// ------------------------------------------------------------------------------------------
template <class Caller>
void CallBackTimer<Caller>::run()
{
	if( !terminated )
		terminate();

	startTimers();
//	PosixThread::start(static_cast<PosixThread*>(this));
	thr->start();
}
// ------------------------------------------------------------------------------------------
template <class Caller>
void CallBackTimer<Caller>::terminate()
{
//	timeAct = 0;
	terminated = true;
	usleep(1000);
}
// ------------------------------------------------------------------------------------------

template <class Caller>
void CallBackTimer<Caller>::add( int id, int timeMS )throw(UniSetTypes::LimitTimers)
{
	if( lst.size() >= MAXCallBackTimer )
	{
		ostringstream err;
		err << "CallBackTimers: превышено максимальное количество таймеров" << MAXCallBackTimer;
		throw UniSetTypes::LimitTimers(err.str()); 
	}
	
	PassiveTimer pt(timeMS);
	TimerInfo ti(id, pt);
	lst.push_back(ti);
//	lst[id] = ti;
}
// ------------------------------------------------------------------------------------------

template <class Caller>
void CallBackTimer<Caller>::remove( int id )
{
	// STL - способ поиска
	typename TimersList::iterator li= find_if(lst.begin(),lst.end(),FindId_eq(id));	
	if( li!=lst.end() )
		lst.erase(li);
}
// ------------------------------------------------------------------------------------------
template <class Caller>
void CallBackTimer<Caller>::startTimers()
{
	for( typename TimersList::iterator li=lst.begin(); li!=lst.end(); ++li)
	{
		li->pt.reset();
	}
}
// ------------------------------------------------------------------------------------------
template <class Caller>
void CallBackTimer<Caller>::clearTimers()
{
	lst.clear();
}
// ------------------------------------------------------------------------------------------
template <class Caller>
void CallBackTimer<Caller>::reset( int id )
{
	typename TimersList::iterator li= find_if(lst.begin(),lst.end(),FindId_eq(id));	
	if( li!=lst.end() )
		li->pt.reset();
}
// ------------------------------------------------------------------------------------------
template <class Caller>
void CallBackTimer<Caller>::setTiming( int id, int timeMS )
{
	typename TimersList::iterator li= find_if(lst.begin(),lst.end(),FindId_eq(id));		
	if( li!=lst.end() )
		li->pt.setTimer(timeMS);
}
// ------------------------------------------------------------------------------------------
template <class Caller>
int CallBackTimer<Caller>::getInterval( int id )
{
	typename TimersList::iterator li= find_if(lst.begin(),lst.end(),FindId_eq(id));		
	if( li!=lst.end() )
		return li->pt.getInterval();
	return -1;
}
// ------------------------------------------------------------------------------------------
template <class Caller>
int CallBackTimer<Caller>::getCurrent( int id )
{
	typename TimersList::iterator li= find_if(lst.begin(),lst.end(),FindId_eq(id));			
	if( li!=lst.end() )
		return li->pt.getCurrent();
	
	return -1;
}
// ------------------------------------------------------------------------------------------

# endif //CallBackTimer_H_
