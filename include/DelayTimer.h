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
#ifndef DelayTimer_H_
#define DelayTimer_H_
// --------------------------------------------------------------------------
#include "PassiveTimer.h"
// --------------------------------------------------------------------------
namespace uniset
{
	/*! Таймер реализующий задержку на срабатывание и отпускание сигнала.
	    Для проверки вызывается функция check( state ), где state - это текущее состояние
	    сигнала, а функция check() возвращает сигнал с задержкой.
	    Чтобы состояние переключилось, оно должно продержаться не менее заданного времени.
	*/
	class DelayTimer
	{
		public:
			DelayTimer() {}

			DelayTimer( timeout_t on_msec, timeout_t off_msec ) noexcept:
				onDelay(on_msec), offDelay(off_msec) {}

			~DelayTimer() noexcept {}

			inline void set( timeout_t on_msec, timeout_t off_msec ) noexcept
			{
				onDelay = on_msec;
				offDelay = off_msec;
				waiting_on = false;
				waiting_off = false;
				state = false;
			}

			// запустить часы (заново)
			inline void reset() noexcept
			{
				pt.reset();
				waiting_on = false;
				waiting_off = false;
				state = false;
			}

			inline bool check( bool st ) noexcept
			{
				prevState = st;

				if( waiting_off )
				{
					if( pt.checkTime() )
					{
						waiting_off = false;

						if( !st )
							state = false;

						return state;
					}
					else if( st )
						waiting_off = false;

					return state;
				}

				if( waiting_on )
				{
					if( pt.checkTime() )
					{
						waiting_on = false;

						if( st )
							state = true;

						return state;
					}
					else if( !st )
						waiting_on = false;

					return state;
				}

				if( state != st )
				{
					waiting_on = false;
					waiting_off = false;

					if( st )
					{
						if( onDelay <= 0 )
						{
							pt.setTiming(0);
							state = st;
							return st;
						}

						pt.setTiming(onDelay);
						waiting_on = true;
					}
					else
					{
						if( offDelay <= 0 )
						{
							pt.setTiming(0);
							state = st;
							return st;
						}

						pt.setTiming(offDelay);
						waiting_off = true;
					}
				}

				return state;
			}

			inline bool get() noexcept
			{
				return check(prevState);
			}

			inline timeout_t getOnDelay() const noexcept
			{
				return onDelay;
			}
			inline timeout_t getOffDelay() const noexcept
			{
				return offDelay;
			}

			inline timeout_t getCurrent() const noexcept
			{
				return pt.getCurrent();
			}

			inline bool isWaitingOn() noexcept
			{
				return !get() && waiting_on;
			}

			inline bool isWaitingOff() noexcept
			{
				return get() && waiting_off;
			}

			inline bool isWaiting() noexcept
			{
				check(prevState);
				return (waiting_off || waiting_on);
			}

		protected:
			PassiveTimer pt;
			bool prevState = { false };
			bool state = { false };
			timeout_t onDelay = { 0 };
			timeout_t offDelay = { 0 };
			bool waiting_on = { false };
			bool waiting_off = { false };
	};
	// -------------------------------------------------------------------------
} // end of uniset namespace
// --------------------------------------------------------------------------
#endif
// --------------------------------------------------------------------------
