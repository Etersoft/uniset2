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
/*! \file
 *  \author Pavel Vainerman
*/
// -------------------------------------------------------------------------- 
#include "Exceptions.h"
#include "Debug.h"
#include "PassiveTimer.h"
#include "UniSetObject_LT.h"
// ------------------------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;

// ------------------------------------------------------------------------------------------
UniSetObject_LT::UniSetObject_LT( UniSetTypes::ObjectId id ):
UniSetObject(id),
sleepTime(UniSetTimer::WaitUpTime)
{
}
// ------------------------------------------------------------------------------------------
UniSetObject_LT::UniSetObject_LT():
sleepTime(UniSetTimer::WaitUpTime) 
{
}

// ------------------------------------------------------------------------------------------
UniSetObject_LT::~UniSetObject_LT() 
{
}
// ------------------------------------------------------------------------------------------
void UniSetObject_LT::callback()
{
	// При реализации с использованием waitMessage() каждый раз при вызове askTimer() необходимо 
	// проверять возвращаемое значение на UniSetTimers::WaitUpTime и вызывать termWaiting(), 
	// чтобы избежать ситуации, когда процесс до заказа таймера 'спал'(в функции waitMessage()) и после 
	// заказа продолжит спать(т.е. обработчик вызван не будет)...
	try
	{	
		if( waitMessage(msg, sleepTime) )
			processingMessage(&msg);

		sleepTime=lt.checkTimers(this);
	}
	catch(Exception& ex)
	{
		ulog.crit() << myname << "(callback): " << ex << endl;
	}
}
// ------------------------------------------------------------------------------------------
void UniSetObject_LT::askTimer( UniSetTypes::TimerId timerid, timeout_t timeMS, short ticks, UniSetTypes::Message::Priority p )
{
	// проверяйте возвращаемое значение
	if( lt.askTimer(timerid, timeMS, ticks, p) != UniSetTimer::WaitUpTime )
		termWaiting();
}
// ------------------------------------------------------------------------------------------
