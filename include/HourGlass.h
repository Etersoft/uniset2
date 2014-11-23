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
    (как фильтр от кратковременных изменений) с "накоплением времени".
    Аналогия с песочными часами:
    \par
    Выставляете время 'run(msec)'.. устанавливаются в какое-то положение часы 'rotate(true)'...
    песок сыплется... если весь пересыпался - срабатывает условие 'check()==true'.
    Если во время работы условие изменилось (часы перевернули в обратную сторону), то
    уже успевший пересыпаться песок, начинает пересыпаться в обратную сторону. Если опять
    повернули часы... продолжает сыпаться опять (добвляясь к тому песку, что "не успел" высыпаться обратно).
    Т.е. до момента срабатывания уже меньше времени чем "полное время" и т.д.

    Класс является "пассивным", т.е. требует периодического вызова функции rotate и check,
    для проверки наступления условия срабатывания.

    \par Пример использования.
    Допустим у вас есть сигнал "перегрев"(in_overheating) и вам необходимо выставить какой-то
    флаг о перегреве (isOverheating), если этот сигнал устойчиво держится в течение 10 секунд,
    то check() станет "true". При этом если сигнал снимется на 5 секунд ("песок начнёт обратно пересыпаться"), 
    а потом опять выставиться, то до срабатывания check() == true уже останется 5 сек, а не 10 сек.
    Получается, что для срабатывания check()=true сигнал должен не колеблясь держаться больше заданного времени.
\code
    HourGlass hg;
    hg.run(10000); // настраиваем часы на 10 сек..

    while( ....)
    {
         hg.rotate(in_overheating); // управляем состоянием песочных часов (прямой или обратный ход часов).
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
            _state   = true;
            _sand    = msec;
            _size    = msec;
        }

        inline void reset()
        {
            run(_size);
        }

        // "ёмкость" песочных часов..
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
                timeout_t cur = t.getCurrent();
                if( cur > _size )
                    cur = _size;

                _sand -= cur;
                if( _sand < 0 )
                    _sand = 0;

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
        inline timeout_t current()
        {
            return t.getCurrent();
        }

        // получить заданное время
        inline timeout_t interval()
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

        // текущее "насыпавшееся" количество "песка"
        inline timeout_t amount()
        {
            return ( _size - remain() );
        }

        // остаток песка (времени)
        inline timeout_t remain()
        {
            timeout_t c = t.getCurrent();
            if( c > _size )
                c = _size;
            
            // _state=false - означает, что песок пересыпается обратно..
            if( !_state )
            {
                int ret = ( _sand + c );
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
        bool _state;      /*!< текущее "положение часов", true - прямое, false - обратное (перевёрнутое) */
        int _sand;        /*!< сколько песка ещё осталось.. */
        timeout_t _size;  /*!< размер часов */
};
// --------------------------------------------------------------------------
#endif
// --------------------------------------------------------------------------
