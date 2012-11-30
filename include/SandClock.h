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
// idea: lav@etersoft.ru
// realisation: pv@etersoft.ru, lav@etersoft.ru
// --------------------------------------------------------------------------
#ifndef SandClock_H_
#define SandClock_H_
// --------------------------------------------------------------------------
/*! WARNING! This class is DEPRECATED! Use HourGlass.. */
// --------------------------------------------------------------------------
#include "PassiveTimer.h"
// --------------------------------------------------------------------------
/*! WARNING! This class is DEPRECATED! Use HourGlass.. */
class SandClock
{
	public:
		SandClock(): _state(false),_sand(0),_size(0){}
		~SandClock(){}

		// запустить часы (заново)
		inline void run( int msec )
		{
			t.setTiming(msec);
			_state 	= true;
			_sand	= msec;
			_size	= msec;
		}

		inline void reset ()
		{
			run(_size);
		}

		inline int duration()
		{
			return _size;
		}
		// перевернуть часы
		// true - засечь время
		// false - перевернуть часы (обратный ход)
		// возвращает аргумент (т.е. идёт ли отсчёт времени)
		inline bool rotate( bool st )
		{
			if( st == _state )
				return st;

			_state = st;
			if( !_state )
			{
				int cur = t.getCurrent();
				_sand -= cur;

				if( _sand < 0 )
					_sand = 0;

//				std::cout << "перевернули: прошло " << cur
//							<< " осталось " << sand
//							<< " засекаем " << cur << endl;

				t.setTiming(cur);
			}
			else
			{
				_sand += t.getCurrent();
				if( _sand > _size )
					_sand = _size;

//				std::cout << "вернули: прошло " << t.getCurrent()
//							<< " осталось " << sand
//							<< " засекам " << sand << endl;

				t.setTiming(_sand);
			}
			return st;
		}

		// получить прошедшее время
		// для положения st
		inline int current( bool st )
		{
			return t.getCurrent();
		}

		// получить заданное время
		// для положения st
		inline int interval( bool st )
		{
			return t.getInterval();
		}

		// проверить наступление
		inline bool check()
		{
			// пока часы не "стоят"
			// всегда false
			if( !_state )
				return false;

			return t.checkTime();
		}

		inline bool state(){ return _state; }

	protected:
		PassiveTimer t;
		bool _state;
		int _sand;
		int _size;
};
// --------------------------------------------------------------------------
#endif
// --------------------------------------------------------------------------
