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
# ifndef CallbackTimer_TCC_H_
# define CallbackTimer_TCC_H_
// -------------------------------------------------------------------------- 
#include <unistd.h>
#include <sstream>
#include "CallbackTimer.h"

// ------------------------------------------------------------------------------------------
template <class Caller> class CallbackTimer;

// ------------------------------------------------------------------------------------------
/*! Создание таймера
	\param r - указатель на заказчика
*/
template <class Caller>
CallbackTimer<Caller>::CallbackTimer( Caller* r, Action a ):
	cal(r),
	act(a),
	terminated(false)
{
	thr = new ThreadCreator<CallbackTimer>(this, &CallbackTimer<Caller>::work);
}

// ------------------------------------------------------------------------------------------

template <class Caller>
CallbackTimer<Caller>::CallbackTimer():
	cal(nullptr),
	terminated(false)
{
	thr = new ThreadCreator<CallbackTimer>(this, &CallbackTimer<Caller>::work);
}
// ------------------------------------------------------------------------------------------
template <class Caller>
CallbackTimer<Caller>::~CallbackTimer()
{
	terminate();
	clearTimers();
	delete thr;
}

// ------------------------------------------------------------------------------------------
template <class Caller>
void CallbackTimer<Caller>::work()
{
	terminated = false;
	while( !terminated )
	{
		usleep(UniSetTimer::MinQuantityTime);

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
void CallbackTimer<Caller>::run()
{
	if( !terminated )
		terminate();

	startTimers();
//	PosixThread::start(static_cast<PosixThread*>(this));
	thr->start();
}
// ------------------------------------------------------------------------------------------
template <class Caller>
void CallbackTimer<Caller>::terminate()
{
//	timeAct = 0;
	terminated = true;
	usleep(1000);
}
// ------------------------------------------------------------------------------------------

template <class Caller>
void CallbackTimer<Caller>::add( int id, int timeMS )throw(UniSetTypes::LimitTimers)
{
	if( lst.size() >= MAXCallbackTimer )
	{
		std::ostringstream err;
		err << "CallbackTimers: exceeded the maximum number of timers (" << MAXCallbackTimer << ")";
		throw UniSetTypes::LimitTimers(err.str()); 
	}
	
	PassiveTimer pt(timeMS);
	TimerInfo ti(id, pt);
	lst.push_back(ti);
//	lst[id] = ti;
}
// ------------------------------------------------------------------------------------------

template <class Caller>
void CallbackTimer<Caller>::remove( int id )
{
	// STL - способ поиска
	typename TimersList::iterator li= find_if(lst.begin(),lst.end(),FindId_eq(id));	
	if( li!=lst.end() )
		lst.erase(li);
}
// ------------------------------------------------------------------------------------------
template <class Caller>
void CallbackTimer<Caller>::startTimers()
{
	for( typename TimersList::iterator li=lst.begin(); li!=lst.end(); ++li)
	{
		li->pt.reset();
	}
}
// ------------------------------------------------------------------------------------------
template <class Caller>
void CallbackTimer<Caller>::clearTimers()
{
	lst.clear();
}
// ------------------------------------------------------------------------------------------
template <class Caller>
void CallbackTimer<Caller>::reset( int id )
{
	typename TimersList::iterator li= find_if(lst.begin(),lst.end(),FindId_eq(id));	
	if( li!=lst.end() )
		li->pt.reset();
}
// ------------------------------------------------------------------------------------------
template <class Caller>
void CallbackTimer<Caller>::setTiming( int id, int timeMS )
{
	typename TimersList::iterator li= find_if(lst.begin(),lst.end(),FindId_eq(id));		
	if( li!=lst.end() )
		li->pt.setTimer(timeMS);
}
// ------------------------------------------------------------------------------------------
template <class Caller>
int CallbackTimer<Caller>::getInterval( int id )
{
	typename TimersList::iterator li= find_if(lst.begin(),lst.end(),FindId_eq(id));		
	if( li!=lst.end() )
		return li->pt.getInterval();
	return -1;
}
// ------------------------------------------------------------------------------------------
template <class Caller>
int CallbackTimer<Caller>::getCurrent( int id )
{
	typename TimersList::iterator li= find_if(lst.begin(),lst.end(),FindId_eq(id));			
	if( li!=lst.end() )
		return li->pt.getCurrent();
	
	return -1;
}
// ------------------------------------------------------------------------------------------

# endif //CallbackTimer_TCC_H_
