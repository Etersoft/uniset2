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
#ifndef Pulse_H_
#define Pulse_H_
// --------------------------------------------------------------------------
#include <iostream>
#include <algorithm>
#include "PassiveTimer.h"
// --------------------------------------------------------------------------
namespace uniset
{
	/*! Класс реализующий формирование импульсов заданной длительности(t1) и заданных пауз между ними(t0).
	    Класс пассивный, для работы требует постоянного вызова функции step().
	    Для получения текущего состояния "выхода" использовать out().
	    Формирование импульсов включается функцией run() либо функцией set(true).
	    Вызов reset() тоже включает формирование импульсов.
	    Выключается формирование вызовом set(false).

		\warning Точность поддержания "импульсов" зависит от частоты вызова step() или out()
	*/
	class Pulse
	{
		public:

			// t1_msec - интервал "вкл"
			// t0_msec - интерфал "откл"
			inline void run( timeout_t _t1_msec, timeout_t _t0_msec ) noexcept
			{
				setTiming(_t1_msec, _t0_msec, true);
			}

			inline void setTiming( timeout_t _t1_msec, timeout_t _t0_msec, bool run = false ) noexcept
			{
				t1_msec = _t1_msec;
				t0_msec = _t0_msec;
				t1.setTiming(t1_msec);
				t0.setTiming(t0_msec);
				set(run);
			}

			inline void reset() noexcept
			{
				set(true);
			}

			inline bool step() noexcept
			{
				if( !enabled )
				{
					ostate = false;
					return false;
				}

				if( ostate && t1.checkTime() )
				{
					ostate = false;
					t0.setTiming(t0_msec);
				}

				if( !ostate && t0.checkTime() )
				{
					ostate = true;
					t1.setTiming(t1_msec);
				}

				return ostate;
			}

			inline bool out() noexcept
			{
				return step(); // ostate;
			}

			inline bool out() const noexcept
			{
				return ostate;
			}

			inline void set( bool state ) noexcept
			{
				enabled = state;

				if( !enabled )
					ostate = false;
				else
				{
					t1.reset();
					t0.reset();
					ostate     = true;
				}
			}

			friend std::ostream& operator<<(std::ostream& os, Pulse& p )
			{
				return os << " idOn=" << p.enabled
					   << " t1=" << p.t1.getInterval()
					   << " t0=" << p.t0.getInterval()
					   << " out=" << p.out();
			}

			friend std::ostream& operator<<(std::ostream& os, Pulse* p )
			{
				return os << (*p);
			}

			inline timeout_t getT1() const noexcept
			{
				return t1_msec;
			}
			inline timeout_t getT0() const noexcept
			{
				return t0_msec;
			}

			bool isOn() const noexcept
			{
				return enabled;
			}

		protected:
			PassiveTimer t1;    // таймер "1"
			PassiveTimer t0;    // таймер "0"
			bool ostate = { false };
			bool enabled = { false };
			timeout_t t1_msec = { 0 };
			timeout_t t0_msec = { 0 };

	};
	// -------------------------------------------------------------------------
} // end of uniset namespace
// --------------------------------------------------------------------------
#endif
// --------------------------------------------------------------------------
