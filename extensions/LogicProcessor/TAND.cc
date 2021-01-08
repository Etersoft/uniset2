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
// -------------------------------------------------------------------------
#include <iostream>
#include "Extensions.h"
#include "Element.h"
// -----------------------------------------------------------------------------
namespace uniset
{
    // -------------------------------------------------------------------------
    using namespace std;
    using namespace uniset::extensions;
    // -------------------------------------------------------------------------
    TAND::TAND(ElementID id, size_t num, bool st):
        TOR(id, num, st)
    {
    }

    TAND::~TAND()
    {
    }
    // -------------------------------------------------------------------------
    void TAND::setIn( size_t num, long value )
    {
        //    cout << this << ": input " << num << " set " << state << endl;
        for( auto&& it : ins )
        {
            if( it.num == num )
            {
                if( it.value == value )
                    return; // вход не менялся можно вообще прервать проверку

                it.value = value;
                break;
            }
        }

        bool prev = myout;
        bool brk = false; // признак досрочного завершения проверки

        // проверяем изменился ли выход
        // для тригера 'AND' проверка до первого 0
        for( auto&& it : ins )
        {
            if( !it.value )
            {
                myout = false;
                brk = true;
                break;
            }
        }

        if( !brk )
            myout = true;

        dinfo << this << ": myout " << myout << endl;

        if( prev != myout )
            Element::setChildOut();
    }
    // -------------------------------------------------------------------------
} // end of namespace uniset
