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
    TSEL_R::TSEL_R( ElementID id, bool st, long _sel_false, long _sel_true ):
        Element(id, true),
        myout(0),
        control_inp(st),
        true_inp(_sel_true),
        false_inp(_sel_false)
    {
        myout = control_inp ? true_inp : false_inp;
    }
    // -------------------------------------------------------------------------
    TSEL_R::~TSEL_R()
    {
    }
    // -------------------------------------------------------------------------
    void TSEL_R::setIn( size_t num, long value )
    {
        bool prev = myout;

        //обновление входов
        switch(num)
        {
          case 1:
            control_inp = (bool)value;
            break;
          case 2:
            true_inp = value;
            break;
          case 3:
            false_inp = value;
            break;
          default:
            break;
        };

        //обновление выхода
        myout = control_inp ? true_inp : false_inp;

        if( prev != myout || init_out )
        {
            init_out = false;
            Element::setChildOut();
        }
    }
    // -------------------------------------------------------------------------
} // end of namespace uniset
