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
#include <cstdio>
#include <unistd.h>
#include "PassiveTimer.h"

//----------------------------------------------------------------------------------------
PassiveTimer::PassiveTimer( ):
	PassiveTimer(WaitUpTime)
{
	reset();
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
bool PassiveTimer::checkTime() const
{
	if( t_msec == WaitUpTime )
		return false;

	if( t_msec == 0 )
		return true;

	return ( std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - t_start).count() >= t_inner_msec.count() );
}

//------------------------------------------------------------------------------
// Установить время таймера
timeout_t PassiveTimer::setTiming( timeout_t msec )
{
	t_msec = msec;

	// не знаю как по другому
	// приходиться делать это через промежуточную переменную
	std::chrono::milliseconds ms(msec);
	t_inner_msec = std::move(ms);

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
timeout_t PassiveTimer::getCurrent() const
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - t_start).count();
}
//------------------------------------------------------------------------------
void PassiveTimer::terminate()
{
	t_msec = WaitUpTime;
}
//------------------------------------------------------------------------------
