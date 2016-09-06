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
// -------------------------------------------------------------------------
using namespace std;
using namespace UniSetExtensions;
// -------------------------------------------------------------------------
TNOT::TNOT( ElementID id, bool out_default ):
	Element(id),
	myout(out_default)
{
	ins.emplace_front(1, !out_default);
}
// -------------------------------------------------------------------------
TNOT::~TNOT()
{
}
// -------------------------------------------------------------------------
void TNOT::setIn( size_t num, bool state )
{
	bool prev = myout;
	myout = !state;

	if( prev != myout )
		Element::setChildOut();
}
// -------------------------------------------------------------------------
