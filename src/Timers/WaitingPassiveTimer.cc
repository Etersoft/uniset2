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

#include <unistd.h>
#include <stream.h>
#include <stdio.h>
// #include "WaitingPassiveTimer.h"
#include "PassiveTimer.h"
// ------------------------------------------------------------------------------------------
int WaitingPassiveTimer::countTimers = 0;
// ------------------------------------------------------------------------------------------

void WaitingPassiveTimer::checkCount()
{
	if ( countTimers >= MAX_COUNT_THRPASSIVE_TIMERS )
	{
		char err[200];
		sprintf(err, "LimitThrPassiveTimers: превышено максимальное количество таймеров %d", MAX_COUNT_THRPASSIVE_TIMERS);
		throw LimitWaitingPTimers(err);
	}

	countTimers++;
}

WaitingPassiveTimer::WaitingPassiveTimer()throw(LimitWaitingPTimers):
	terminated(true),
	//    pCall(NULL),
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
WaitingPassiveTimer::WaitingPassiveTimer(bool* value )throw(LimitWaitingPTimers):
	pValue(value),
	terminated(true)
	//    pCall(NULL)
{
	checkCount();
}

// ------------------------------------------------------------------------------------------
WaitingPassiveTimer::~WaitingPassiveTimer()
{
	//    cout << "Timer: destructor.."<< endl;
	//    pCall = NULL;
	pValue = NULL;
	terminate();
	countTimers--;
}
// ------------------------------------------------------------------------------------------
void WaitingPassiveTimer::work()
{
	timeout_t sleepMKS = MIN_QUANTITY_TIME_MS * 1000;
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
	//    cout << "Timer: завершил поток..."<< endl;
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
