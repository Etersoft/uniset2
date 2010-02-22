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
 *  \date   $Date: 2005/01/28 21:12:49 $
 *  \version $Id: WaitingPassiveTimer.cc,v 1.6 2005/01/28 21:12:49 vitlav Exp $
*/
// -------------------------------------------------------------------------- 

#include <unistd.h>
#include <stream.h>
#include <stdio.h>
// #include "WaitingPassiveTimer.h"
#include "PassiveTimer.h"
// ------------------------------------------------------------------------------------------
int WaitingPassiveTimer::countTimers=0;
// ------------------------------------------------------------------------------------------

void WaitingPassiveTimer::checkCount()
{
	if ( countTimers >= MAX_COUNT_THRPASSIVE_TIMERS )
	{
		char err[200];
		sprintf(err,"LimitThrPassiveTimers: превышено максимальное количество таймеров %d", MAX_COUNT_THRPASSIVE_TIMERS);
		throw LimitWaitingPTimers(err); 
	}
	
	countTimers++;	
}

WaitingPassiveTimer::WaitingPassiveTimer()throw(LimitWaitingPTimers):
	terminated(true),
//	pCall(NULL),
	pValue(NULL)
{
	checkCount();

}
// ------------------------------------------------------------------------------------------
/*
WaitingPassiveTimer::WaitingPassiveTimer( void(*fp)(void) ):
	pCall(fp)
{
}
*/
// ------------------------------------------------------------------------------------------
/*!
 * \param  *value - указатель на объект подлежащий изменению
*/
WaitingPassiveTimer::WaitingPassiveTimer(bool *value )throw(LimitWaitingPTimers):
	pValue(value),
	terminated(true)
//	pCall(NULL)
{
	checkCount();
}

// ------------------------------------------------------------------------------------------
WaitingPassiveTimer::~WaitingPassiveTimer()
{
//	cout << "Timer: destructor.."<< endl;
//	pCall = NULL;
	pValue = NULL;
	terminate();
	countTimers--;
}
// ------------------------------------------------------------------------------------------
void WaitingPassiveTimer::work()
{
	timeout_t sleepMKS = MIN_QUANTITY_TIME_MS*1000;
	terminated = false;
	while( !terminated )
	{
		usleep(sleepMKS); 
		if ( checkTime() )
			break;
	}
	
	terminated = true;
	if(pValue != NULL)
		*pValue ^= true;
/*	
	if(pCall!=NULL)
	{
		pCall();
	}

*/	
/*
	check = false;
	pause();
	check = true;
*/	
	stop();
//	cout << "Timer: завершил поток..."<< endl;
}
// ------------------------------------------------------------------------------------------
void WaitingPassiveTimer::terminate()
{
	timeAct = 0;
	terminated = true;
	usleep(1000);
}
// ------------------------------------------------------------------------------------------
void WaitingPassiveTimer::wait(timeout_t timeMS)
{
	if ( !terminated )
		terminate();

	setTiming(timeMS);
	start((PosixThread*)this);
	pthread_join(getTID(), NULL);
}

// ------------------------------------------------------------------------------------------
