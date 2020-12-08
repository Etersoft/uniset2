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
#include <sstream>
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
    TOR::TOR(ElementID id, size_t num, bool st):
        Element(id),
        myout(false)
    {
        if( num != 0 )
        {
            // создаём заданное количество входов
            for( size_t i = 1; i <= num; i++ )
            {
                ins.emplace_front(i, st); // addInput(i,st);

                if( st == true )
                    myout = true;
            }
        }
    }

    TOR::~TOR()
    {
    }
    // -------------------------------------------------------------------------
    void TOR::setIn( size_t num, long value )
    {
        //    cout << getType() << "(" << myid << "):  input " << num << " set " << state << endl;
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
        // для тригера 'OR' проверка до первой единицы
        for( auto&& it : ins )
        {
            if( it.value )
            {
                myout = true;
                brk = true;
                break;
            }
        }

        if( !brk )
            myout = false;

        dinfo << this << ": myout " << myout << endl;

        if( prev != myout )
            Element::setChildOut();
    }
    // -------------------------------------------------------------------------
} // end of namespace uniset
