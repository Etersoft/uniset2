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
 *  \date   $Date: 2007/11/29 22:19:54 $
 *  \version $Id: PassiveTimer.cc,v 1.13 2007/11/29 22:19:54 vpashka Exp $
*/
// -------------------------------------------------------------------------- 
#include <time.h>
#include <sys/times.h>
#include <stdio.h>
#include <unistd.h>
#include "PassiveTimer.h"

//------------------------------------------------------------------------------

PassiveTimer::PassiveTimer( ):
timeAct(0),
timeSS(0),
timeStart(0)
{
	clock_ticks = sysconf(_SC_CLK_TCK);
	setTiming(-1);
}

//------------------------------------------------------------------------------

PassiveTimer::PassiveTimer( int timeMS ):
timeAct(0),
timeSS(0),
timeStart(0)
{
//	printf("const =%d\n",timeMS);
	clock_ticks = sysconf(_SC_CLK_TCK);
	setTiming( timeMS );
}

//------------------------------------------------------------------------------
/*! \note  Если задано timeMS<0, не сработает никогда */
bool PassiveTimer::checkTime()
{
//	printf("times=%d, act=%d\n",times(0),timeAct);
//	printf("%d\n",timeSS); msleep(10);

	if( timeSS<0 )	// == WaitUpTime;
		return false; 

	if( times(0) >= timeAct )
		return true;
	return false;
}

//------------------------------------------------------------------------------
// Установить время таймера
void PassiveTimer::setTiming( int timeMS )
{
	if( timeMS<0 )
	{
		// для корректной работы getCurrent()
		// время всё-равно надо засечь
		timeSS=WaitUpTime;
		timeStart=times(0);
//		PassiveTimer::reset();
	}
	else
	{
		timeSS=timeMS/10; // задержка в сантисекундах
		if (timeMS%10)
			timeSS++; // Округляем в большую сторону
		PassiveTimer::reset();
	}
}

//------------------------------------------------------------------------------
// Запустить таймер
void PassiveTimer::reset(void)
{
	timeStart=times(0);
	timeAct = (timeSS*clock_ticks)/100 + timeStart;
}
//------------------------------------------------------------------------------
// получить текущее значение таймера
int PassiveTimer::getCurrent()
{
	return (times(0)-timeStart)*1000/clock_ticks;
}
//------------------------------------------------------------------------------
void PassiveTimer::terminate()
{
	timeAct = 0;
}
//------------------------------------------------------------------------------
