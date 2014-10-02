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
#include <time.h>
#include <sys/times.h>
#include <stdio.h>
#include <unistd.h>
#include "PassiveTimer.h"

//----------------------------------------------------------------------------------------
// CLK_TCK устарела по новому стандарту
#ifndef CLK_TCK
#define CLK_TCK sysconf(_SC_CLK_TCK)
#endif
//----------------------------------------------------------------------------------------

//------------------------------------------------------------------------------

PassiveTimer::PassiveTimer( ):
timeAct(0),
timeSS(0),
timeStart(0),
clock_ticks(sysconf(_SC_CLK_TCK))
{
    setTiming(WaitUpTime);
}

//------------------------------------------------------------------------------

PassiveTimer::PassiveTimer( timeout_t timeMS ):
timeAct(0),
timeSS(0),
timeStart(0),
clock_ticks(sysconf(_SC_CLK_TCK))
{
//    printf("const =%d\n",timeMS);
    setTiming( timeMS );
}

//------------------------------------------------------------------------------
PassiveTimer::~PassiveTimer()
{

}
//------------------------------------------------------------------------------
bool PassiveTimer::checkTime()
{
//    printf("times=%d, act=%d\n",times(0),timeAct);
//    printf("%d\n",timeSS); msleep(10);

    if( timeSS == WaitUpTime )
        return false;

    if( times() >= timeAct )
        return true;
    return false;
}

//------------------------------------------------------------------------------
// Установить время таймера
timeout_t PassiveTimer::setTiming( timeout_t timeMS )
{
    if( timeMS == WaitUpTime )
        timeSS = WaitUpTime;
    else
    {
        timeSS=timeMS/10; // задержка в сантисекундах
        if (timeMS%10)
            timeSS++; // Округляем в большую сторону
    }
    PassiveTimer::reset();
    return getInterval();
}

//------------------------------------------------------------------------------
// Запустить таймер
void PassiveTimer::reset(void)
{
    timeStart = times();
    if( timeSS == WaitUpTime)
        return;
    timeAct = (timeSS*clock_ticks)/100 + timeStart;
}
//------------------------------------------------------------------------------
// получить текущее значение таймера
timeout_t PassiveTimer::getCurrent()
{
    return (times()-timeStart)*1000/clock_ticks;
}
//------------------------------------------------------------------------------
void PassiveTimer::terminate()
{
    timeAct = 0;
}

//------------------------------------------------------------------------------
clock_t PassiveTimer::times()
{
    // Не в Linux запрещено вызывать с NULL
    struct tms tm;
    return ::times(&tm);
}
