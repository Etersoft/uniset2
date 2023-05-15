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
#include "TA2D.h"
// -----------------------------------------------------------------------------
namespace uniset
{
    // -------------------------------------------------------------------------
    using namespace std;
    using namespace uniset::extensions;
    // -------------------------------------------------------------------------
    TA2D::TA2D(Element::ElementID id, long filterValue ):
        Element(id, true),
        myout(false),
        fvalue(filterValue)
    {
    }

    TA2D::~TA2D()
    {
    }
    // -------------------------------------------------------------------------
    void TA2D::setIn( size_t num, long value )
    {
        // num игнорируем, т.к у нас всего один вход..

        bool prev = myout;
        myout = ( fvalue == value );

        if( prev != myout || init_out )
        {
            init_out = false;
            Element::setChildOut();
        }
    }
    // -------------------------------------------------------------------------
    long TA2D::getOut() const
    {
        return ( myout ? 1 : 0 );
    }
    // -------------------------------------------------------------------------
    void TA2D::setFilterValue( long value )
    {
        if( fvalue != value && myout )
            myout = false;

        fvalue = value;
    }
    // -------------------------------------------------------------------------
} // end of namespace uniset
