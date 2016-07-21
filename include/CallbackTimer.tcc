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
void CallbackTimer<Caller>::add(size_t id, timeout_t timeMS )throw(UniSetTypes::LimitTimers)
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
void CallbackTimer<Caller>::remove( size_t id )
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
void CallbackTimer<Caller>::reset( size_t id )
{
	typename TimersList::iterator li= find_if(lst.begin(),lst.end(),FindId_eq(id));	
	if( li!=lst.end() )
		li->pt.reset();
}
// ------------------------------------------------------------------------------------------
template <class Caller>
void CallbackTimer<Caller>::setTiming( size_t id, timeout_t timeMS )
{
	typename TimersList::iterator li= find_if(lst.begin(),lst.end(),FindId_eq(id));		
	if( li!=lst.end() )
		li->pt.setTimer(timeMS);
}
// ------------------------------------------------------------------------------------------
template <class Caller>
timeout_t CallbackTimer<Caller>::getInterval( size_t id )
{
	typename TimersList::iterator li= find_if(lst.begin(),lst.end(),FindId_eq(id));		
	if( li!=lst.end() )
		return li->pt.getInterval();

	return TIMEOUT_INF;
}
// ------------------------------------------------------------------------------------------
template <class Caller>
timeout_t CallbackTimer<Caller>::getCurrent( size_t id )
{
	typename TimersList::iterator li= find_if(lst.begin(),lst.end(),FindId_eq(id));			
	if( li!=lst.end() )
		return li->pt.getCurrent();
	
	return TIMEOUT_INF;
}
// ------------------------------------------------------------------------------------------
# endif //CallbackTimer_TCC_H_
