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
/*! Таймер реализующий задержку на срабатывание и отпускание сигнала.
	Для проверки вызывается функция check( state ), где state - это текущее состояние
	сигнала, а функция check() возвращает сигнал с задержкой.
	Чтобы состояние переключилось, оно должно продержаться не менее заданного времени.
*/
class DelayTimer
{
	public:
		DelayTimer():prevState(false),state(false),
				onDelay(0),offDelay(0),waiting_on(false),waiting_off(false){}

		DelayTimer( timeout_t on_msec, timeout_t off_msec ):prevState(false),state(false),
				onDelay(on_msec),offDelay(off_msec),waiting_on(false),waiting_off(false)
		{
		}

		~DelayTimer(){}

		inline void set( timeout_t on_msec, timeout_t off_msec )
		{
			onDelay = on_msec;
			offDelay = off_msec;
		}

		// запустить часы (заново)
		inline void reset()
		{
			pt.reset();
		}

		inline bool check( bool st )
		{
			if( waiting_off )
			{
				if( !st && pt.checkTime() )
				{
					waiting_off = false;
					state = false;
					return state;
				}
				else if( st != prevState )
					pt.reset();

				prevState = st;
				return state;
			}

			if( waiting_on )
			{
				if( st && pt.checkTime() )
				{
					waiting_on = false;
					state = true;
					return state;
				}
				else if( st != prevState )
					pt.reset();

				prevState = st;
				return state;
			}

			if( state != st )
			{
				prevState = st;
				if( st )
				{
					pt.setTiming(onDelay);
					waiting_on = true;
				}
				else
				{
					pt.setTiming(offDelay);
					waiting_off = true;
				}

				if( pt.checkTime() )
					return st;
			}

			return state;
		}

	protected:
		PassiveTimer pt;
		bool prevState;
		bool state;
		timeout_t onDelay;
		timeout_t offDelay;
		bool waiting_on;
		bool waiting_off;
};
// --------------------------------------------------------------------------
#endif
// --------------------------------------------------------------------------
