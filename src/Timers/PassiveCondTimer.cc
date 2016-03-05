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
#include <sstream>
#include <time.h>
#include "PassiveTimer.h"
// ------------------------------------------------------------------------------------------
using namespace std;
// ------------------------------------------------------------------------------------------

PassiveCondTimer::PassiveCondTimer():
	terminated(ATOMIC_VAR_INIT(1))
{
}
// ------------------------------------------------------------------------------------------
PassiveCondTimer::~PassiveCondTimer()
{
	terminate();
}
// ------------------------------------------------------------------------------------------
void PassiveCondTimer::terminate()
{
	{
		std::unique_lock<std::mutex> lk(m_working);
		terminated = true;
	}
	cv_working.notify_all();
}
// ------------------------------------------------------------------------------------------
bool PassiveCondTimer::wait( timeout_t time_msec )
{
	std::unique_lock<std::mutex> lk(m_working);
	terminated = false;

	timeout_t t_msec = PassiveTimer::setTiming(time_msec); // вызываем для совместимости с обычным PassiveTimer-ом

	if( time_msec == WaitUpTime )
	{
		while( !terminated )
			cv_working.wait(lk);
	}
	else
		cv_working.wait_for(lk, std::chrono::milliseconds(t_msec), [&]()
	{
		return (terminated == true);
	} );

	terminated = true;
	return true;
}
// ------------------------------------------------------------------------------------------
