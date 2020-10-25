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
// idea: lav@etersoft.ru
// implementation: pv@etersoft.ru, lav@etersoft.ru
// --------------------------------------------------------------------------
#ifndef HourGlass_H_
#define HourGlass_H_
// --------------------------------------------------------------------------
#include "PassiveTimer.h"
// --------------------------------------------------------------------------
namespace uniset
{
	// header only

	/*! Песочные часы.  Класс реализующий логику песочных часов.
	    Удобен для создания задержек на срабатывание и на отпускание
	    (как фильтр от кратковременных изменений) с "накоплением времени".
	    Аналогия с песочными часами:
	    \par
	    Выставляете время 'run(msec)'.. устанавливаются в какое-то положение часы 'rotate(true)'...
	    песок сыплется... если весь пересыпался - срабатывает условие 'check()==true'.
	    Если во время работы условие изменилось (часы перевернули в обратную сторону), то
	    уже успевший пересыпаться песок, начинает пересыпаться в обратную сторону. Если опять
	    повернули часы... продолжает сыпаться опять (добавляясь к тому песку, что "не успел" высыпаться обратно).
	    Т.е. до момента срабатывания уже меньше времени чем "полное время" и т.д.

	    Класс является "пассивным", т.е. требует периодического вызова функции rotate и check,
	    для проверки наступления условия срабатывания.

	    \par Пример использования.
		Допустим у вас есть сигнал "температура"(in_temp) и вам необходимо выставить какой-то
		флаг о перегреве (isOverheating).
		Если температура продержится выше порога в течение 10 секунд check() станет "true".
		Если температура станет меньше порога через 6 секунд ("песок начнёт обратно пересыпаться"),
		а потом опять станет выше, то до срабатывания check() == true уже останется 4 сек, а не 10 сек.
	    Получается, что для срабатывания check()=true сигнал должен не колеблясь держаться больше заданного времени.
	\code
	    HourGlass hg;
	    hg.run(10000); // настраиваем часы на 10 сек..

	    while( ....)
	    {
			 hg.rotate( in_temp > HiTemp ); // управляем состоянием песочных часов (прямой или обратный ход часов).
	         isOverheating = hg.check();
	    }

	\endcode
	*/
	class HourGlass
	{
		public:
			HourGlass() noexcept {}
			~HourGlass() noexcept {}

			// запустить часы (заново)
			inline void run( timeout_t msec ) noexcept
			{
				t.setTiming(msec);
				_state   = true;
				_sand    = msec;
				_size    = msec;
			}

			inline void reset() noexcept
			{
				run(_size);
			}

			// "ёмкость" песочных часов..
			inline timeout_t duration() const noexcept
			{
				return _size;
			}
			// перевернуть часы
			// true - засечь время
			// false - перевернуть часы (обратный ход)
			// возвращает аргумент (т.е. идёт ли отсчёт времени)
			inline bool rotate( bool st ) noexcept
			{
				if( st == _state )
					return st;

				_state = st;

				// TODO 24.10.2015 Lav: follow code is very simular to remain function
				if( !_state )
				{
					timeout_t cur = t.getCurrent();

					if( cur > _size )
						cur = _size;

					_sand = ( _sand > cur ) ? (_sand - cur) : 0;

					t.setTiming(cur);
				}
				else
				{
					timeout_t cur = t.getCurrent();

					if( cur > _size )
						cur = _size;

					_sand += cur;

					if( _sand > _size )
						_sand = _size;

					t.setTiming(_sand);
				}

				return st;
			}

			// получить прошедшее время
			inline timeout_t current() const noexcept
			{
				return t.getCurrent();
			}

			// получить заданное время
			inline timeout_t interval() const noexcept
			{
				return t.getInterval();
			}

			// проверить наступление
			inline bool check() const noexcept
			{
				// пока часы не "стоят"
				// всегда false
				if( !_state )
					return false;

				return t.checkTime();
			}

			inline bool enabled() const noexcept
			{
				return _state;
			}

			// текущее "насыпавшееся" количество "песка" (прошедшее время)
			inline timeout_t amount() const noexcept
			{
				return ( _size - remain() );
			}

			// остаток песка (времени) (оставшееся время)
			inline timeout_t remain() const noexcept
			{
				timeout_t c = t.getCurrent();

				if( c > _size )
					c = _size;

				// _state=false - означает, что песок пересыпается обратно..
				if( !_state )
				{
					timeout_t ret = ( _sand + c );

					if( ret > _size )
						return _size;

					return ret;
				}

				// _state=true  - означает, что песок пересыпается..
				int ret = ( _sand - c );

				if( ret < 0 )
					return 0;

				return ret;
			}

		protected:
			PassiveTimer t;   /*!< таймер для отсчёта времени.. */
			bool _state = { false };      /*!< текущее "положение часов", true - прямое, false - обратное (перевёрнутое) */
			timeout_t _sand = { 0 };  /*!< сколько песка ещё осталось.. */
			timeout_t _size = { 0 };  /*!< размер часов */
	};
	// -------------------------------------------------------------------------
} // end of uniset namespace
// --------------------------------------------------------------------------
#endif
// --------------------------------------------------------------------------
