/* This file is part of the UniSet project
 * Copyright (c) 2002 Free Software Foundation, Inc.
 * Copyright (c) 2002 Pavel Vainerman <pv>
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
//!  \version $Id: SandClock.h,v 1.1 2008/10/05 19:00:53 vpashka Exp $
// --------------------------------------------------------------------------
#ifndef SandClock_H_
#define SandClock_H_
// --------------------------------------------------------------------------
#include <uniset/PassiveTimer.h>
// --------------------------------------------------------------------------
class SandClock
{
	public:
		SandClock(): state(false),sand(0),size(0){}
		~SandClock(){}
	
		// запустить часы (заново)
		inline void run( int msec )
		{
			t.setTiming(msec);
			state 	= true;
			sand	= msec;
			size	= msec;
		}
		
		inline void reset ()
		{
			run(size);
		}

		inline int duration()
		{
			return size;
		}
		// перевернуть часы
		// true - засечь время
		// false - перевернуть часы (обратный ход)
		// возвращает аргумент (т.е. идёт ли отсчёт времени)
		inline bool rotate( bool st )
		{
			if( st == state )
				return st;
				
			state = st;
			if( !state )
			{
				int cur = t.getCurrent();
				sand -= cur;

				if( sand < 0 )
					sand = 0;

//				std::cout << "перевернули: прошло " << cur
//							<< " осталось " << sand 
//							<< " засекаем " << cur << endl;

				t.setTiming(cur);
			}
			else
			{
				sand += t.getCurrent();
				if( sand > size )
					sand = size;

//				std::cout << "вернули: прошло " << t.getCurrent()
//							<< " осталось " << sand 
//							<< " засекам " << sand << endl;
	
				t.setTiming(sand);
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
			if( !state )
				return false;

			return t.checkTime();
		}

		inline bool state(){ return state; }
		
	protected:
		PassiveTimer t;
		bool state;
		int sand;
		int size;
};
// --------------------------------------------------------------------------
#endif
// --------------------------------------------------------------------------
