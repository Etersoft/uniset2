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
#include "TDelay.h"
// -------------------------------------------------------------------------
using namespace std;
using namespace UniSetExtensions;
// -------------------------------------------------------------------------
TDelay::TDelay(Element::ElementID id, timeout_t delayMS, size_t inCount):
	Element(id),
	myout(false),
	delay(delayMS)
{
	if( inCount != 0 )
	{
		// создаём заданное количество входов
		for( unsigned int i = 1; i <= inCount; i++ )
			ins.push_front(InputInfo(i, false)); // addInput(i,st);
	}
}

TDelay::~TDelay()
{
}
// -------------------------------------------------------------------------
void TDelay::setIn( size_t num, bool state )
{
	bool prev = myout;

	// сбрасываем сразу
	if( !state )
	{
		pt.setTiming(0); // reset timer
		myout = false;
		dinfo << this << ": set " << myout << endl;

		if( prev != myout )
			Element::setChildOut();

		return;
	}

	//    if( state )

	// выставляем без задержки
	if( delay <= 0 )
	{
		pt.setTiming(0); // reset timer
		myout = true;
		dinfo << this << ": set " << myout << endl;

		if( prev != myout )
			Element::setChildOut();

		return;
	}

	// засекаем, если ещё не установлен таймер
	if( !myout && !prev  ) // т.е. !myout && prev != myout
	{
		dinfo << this << ": set timer " << delay << " [msec]" << endl;
		pt.setTiming(delay);
	}
}
// -------------------------------------------------------------------------
void TDelay::tick()
{
	if( pt.getInterval() != 0 && pt.checkTime() )
	{
		myout = true;
		pt.setTiming(0); // reset timer
		dinfo << getType() << "(" << myid << "): TIMER!!!! myout=" << myout << endl;
		Element::setChildOut();
	}
}
// -------------------------------------------------------------------------
bool TDelay::getOut() const
{
	return myout;
}
// -------------------------------------------------------------------------
void TDelay::setDelay( timeout_t timeMS )
{
	delay = timeMS;
}
// -------------------------------------------------------------------------
