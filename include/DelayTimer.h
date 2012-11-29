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
#ifndef DelayTimer_H_
#define DelayTimer_H_
// --------------------------------------------------------------------------
#include "PassiveTimer.h"
// --------------------------------------------------------------------------
/*! Таймер реализующий задержку на срабатывание и отпускание сигнала..
*/
class DelayTimer
{
	public:
		DelayTimer():realState(false),state(false),
				ondelay(0),offdelay(0),waiting(false){}

		DelayTimer( timeout_t on_msec, timeout_t off_msec ):realState(false),state(false),
				ondelay(on_msec),offdelay(off_msec),waiting(false)
		{
		}
		
		~DelayTimer(){}
	
		// запустить часы (заново)
		inline void set( timeout_t on_msec, timeout_t off_msec )
		{
			ondelay = on_msec;
			offdelay = off_msec;
		}
		
		inline void reset()
		{
			pt.reset();
		}

		inline bool check( bool st )
		{
			if( realState != st )
			{
				if( st )
					pt.setTiming(ondelay);
				else
					pt.setTiming(offdelay);

				waiting = true;
				realState = st;
			}
			
			if( !waiting )
				return state;
			
			if( pt.checkTime() )
			{
				state = realState;
				waiting = false;
			}

			return state;
		}

	protected:
		PassiveTimer pt;
		bool realState;
		bool state;
		timeout_t ondelay;
		timeout_t offdelay;
		bool waiting;
};
// --------------------------------------------------------------------------
#endif
// --------------------------------------------------------------------------
