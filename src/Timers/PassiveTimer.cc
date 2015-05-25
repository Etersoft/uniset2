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
#include <cstdio>
#include <unistd.h>
#include "PassiveTimer.h"

//----------------------------------------------------------------------------------------
PassiveTimer::PassiveTimer( ):
	PassiveTimer(WaitUpTime)
{

}
//------------------------------------------------------------------------------

PassiveTimer::PassiveTimer( timeout_t msec ):
	t_msec(msec)
{
	setTiming(msec);
}

//------------------------------------------------------------------------------
PassiveTimer::~PassiveTimer()
{

}
//------------------------------------------------------------------------------
bool PassiveTimer::checkTime()
{
	if( t_msec == WaitUpTime )
		return false;

	if( t_msec == 0 )
		return true;

	return ( std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - t_start).count() >= t_msec );
}

//------------------------------------------------------------------------------
// Установить время таймера
timeout_t PassiveTimer::setTiming( timeout_t msec )
{
	t_msec = msec;
	PassiveTimer::reset();
	return getInterval();
}
//------------------------------------------------------------------------------
// Запустить таймер
void PassiveTimer::reset(void)
{
	t_start = std::chrono::high_resolution_clock::now();
}
//------------------------------------------------------------------------------
// получить текущее значение таймера
timeout_t PassiveTimer::getCurrent()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - t_start).count();
}
//------------------------------------------------------------------------------
void PassiveTimer::terminate()
{
	t_msec = WaitUpTime;
}
//------------------------------------------------------------------------------
