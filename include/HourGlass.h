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
#ifndef HourGlass_H_
#define HourGlass_H_
// --------------------------------------------------------------------------
#include "PassiveTimer.h"
// --------------------------------------------------------------------------
/*! Песочные часы.  Класс реализующий логику песочных часов.
	Удобен для создания задержек на срабатывание и на отпускание
	(как фильтр от кратковременных изменений). Аналогия с песочными часами:
	\par
	Выставляете время(run).. устанавливаются в какое-то положение часы (rotate)...
	песок сыплется... если весь пересыпался - срабатывает условие (check()==true).
	Если во время работы условие изменилось (часы перевернули в обратную сторону), то
	уже успевший пересыпаться песок, начинает пересыпаться в обратную сторону. Если опять
	повернули часы... продолжает сыпаться опять (добвляясь к тому песку, что "не успел" высыпаться обратно).
	и т.д. по кругу...

	Класс является "пассивным", т.е. требует периодического вызова функции check, для проверки наступления условия срабатывания.

	\par Пример использования.
	Допустим у вас есть сигнал "перегрев"(in_overheating) и вам необходимо выставить какой-то
	флаг о перегреве (isOverheating), если этот сигнал устойчиво держится в течение 10 секунд,
	и при этом если сигнал снялся, то вам необходимо как минимум те же 10 секунд,
	подождать прежде чем "снять" флаг. Для этого удобно использовать данный класс.
\code
	HourGlass hg;
	hg.run(10000); // настриваем часы на 10 сек..

	while( ....)
	{
	     hg.rotate(in_overheating); // управляем состоянием песочных часов (прямой или обратный ход).
	     isOverheating = hg.check();
	}

\endcode
*/
class HourGlass
{
	public:
		HourGlass(): _state(false),_sand(0),_size(0){}
		~HourGlass(){}

		// запустить часы (заново)
		inline void run( timeout_t msec )
		{
			t.setTiming(msec);
			_state 	= true;
			_sand	= msec;
			_size	= msec;
		}

		inline void reset()
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
		inline timeout_t current( bool st )
		{
			return t.getCurrent();
		}

		// получить заданное время
		// для положения st
		inline timeout_t interval( bool st )
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
		timeout_t _size;
};
// --------------------------------------------------------------------------
#endif
// --------------------------------------------------------------------------
