/*
 * Copyright (c) 2023 Ilya Polshchikov.
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
    TRS::TRS( ElementID id, bool st, bool _dominantReset):
        Element(id, true),
        myout(st),
        dominantReset(_dominantReset),
        set_inp(false),
        reset_inp(false)
    {
    }
    // -------------------------------------------------------------------------
    TRS::~TRS()
    {
    }
    // -------------------------------------------------------------------------
    void TRS::setIn( size_t num, long value )
    {
        bool prev = myout;

        //обновление входов
        //обновление входов
        switch(num)
        {
          case 1:
            set_inp = (bool)value;
            break;
          case 2:
            reset_inp = (bool)value;
            break;
          default:
            break;
        };

        //обновление выхода
        if(dominantReset)
        {
            myout = set_inp ? true : myout;
            myout = reset_inp ? false : myout;
        }
        else
        {
            myout = reset_inp ? false : myout;
            myout = set_inp ? true : myout;
        }

        if( prev != myout || init_out )
        {
            init_out = false;
            Element::setChildOut();
        }
    }
    // -------------------------------------------------------------------------
} // end of namespace uniset
