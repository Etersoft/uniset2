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
        cv_working.wait_for(lk, std::chrono::milliseconds(t_msec), [&](){ return (terminated == true); } );

    terminated = true;
    return true;
}
// ------------------------------------------------------------------------------------------
