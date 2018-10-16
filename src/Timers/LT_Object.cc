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
/*! \file
 *  \author Pavel Vainerman
*/
// --------------------------------------------------------------------------
#include <sstream>
#include <algorithm>
#include "Exceptions.h"
#include "UniSetObject.h"
#include "LT_Object.h"
#include "Debug.h"

// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;

// -----------------------------------------------------------------------------
LT_Object::LT_Object():
	sleepTime(UniSetTimer::WaitUpTime),
	lstMutex("LT_Object::lstMutex")
{
	tmLast.setTiming(UniSetTimer::WaitUpTime);
}

// -----------------------------------------------------------------------------
LT_Object::~LT_Object()
{
}
// -----------------------------------------------------------------------------
timeout_t LT_Object::checkTimers( UniSetObject* obj )
{
	try
	{
		{
			// lock
			uniset_rwmutex_rlock lock(lstMutex);

			if( tlst.empty() )
			{
				sleepTime = UniSetTimer::WaitUpTime;
				return sleepTime;
			}
		}

		// защита от непрерывного потока сообщений
		if( tmLast.getCurrent() < UniSetTimer::MinQuantityTime )
		{
			// корректируем сперва sleepTime
			sleepTime = tmLast.getLeft(sleepTime);

			if( sleepTime < UniSetTimer::MinQuantityTime )
			{
				sleepTime = UniSetTimer::MinQuantityTime;
				return sleepTime;
			}
		}

		{
			// lock
			uniset_rwmutex_wrlock lock(lstMutex);
			sleepTime = UniSetTimer::WaitUpTime;

			for( auto li = tlst.begin(); li != tlst.end(); ++li )
			{
				if( li->tmr.checkTime() )
				{
					// помещаем себе в очередь сообщение
					TransportMessage tm( TimerMessage(li->id, li->priority, obj->getId()).transport_msg() );
					obj->push(tm);

					// Проверка на количество заданных тактов
					if( !li->curTick )
					{
						li = tlst.erase(li);
						--li;

						if( tlst.empty() )
							sleepTime = UniSetTimer::WaitUpTime;

						continue;
					}
					else if(li->curTick > 0 )
						li->curTick--;

					li->reset();
				}
				else
				{
					li->curTimeMS = tmLast.getLeft(li->curTimeMS);
				}

				// ищем минимальное оставшееся время
				if( li->curTimeMS < sleepTime || sleepTime == UniSetTimer::WaitUpTime )
					sleepTime = li->curTimeMS;
			}

			if( sleepTime < UniSetTimer::MinQuantityTime )
				sleepTime = UniSetTimer::MinQuantityTime;
		} // unlock

		tmLast.reset();
	}
	catch( const uniset::Exception& ex )
	{
		ucrit << "(checkTimers): " << ex << endl;
	}

	return sleepTime;
}
// ------------------------------------------------------------------------------------------
timeout_t LT_Object::getTimeInterval( TimerId timerid ) const
{
	// lock
	uniset_rwmutex_rlock lock(lstMutex);

	for( const auto& li : tlst )
	{
		if( li.id == timerid )
			return li.tmr.getInterval();
	}

	return 0;
}
// ------------------------------------------------------------------------------------------
timeout_t LT_Object::getTimeLeft( TimerId timerid ) const
{
	// lock
	uniset_rwmutex_rlock lock(lstMutex);

	for( const auto& li : tlst )
	{
		if( li.id == timerid )
			return li.curTimeMS;
	}

	return 0;
}
// ------------------------------------------------------------------------------------------
LT_Object::TimersList LT_Object::getTimersList() const
{
	uniset_rwmutex_rlock l(lstMutex);
	TimersList lst(tlst);
	return lst;
}
// ------------------------------------------------------------------------------------------
string LT_Object::getTimerName( int id ) const
{
	return "";
}
// ------------------------------------------------------------------------------------------

timeout_t LT_Object::askTimer( uniset::TimerId timerid, timeout_t timeMS, clock_t ticks, uniset::Message::Priority p )
{
	if( timeMS > 0 ) // заказ
	{
		if( timeMS < UniSetTimer::MinQuantityTime )
		{
			ucrit << "(LT_askTimer): [мс] попытка заказать таймер " << getTimerName(timerid)
				  << " со временем срабатывания "
				  << " меньше разрешённого " << UniSetTimer::MinQuantityTime << endl;
			timeMS = UniSetTimer::MinQuantityTime;
		}

		{
			// lock
			uniset_rwmutex_wrlock lock(lstMutex);

			// поищем а может уж такой есть
			if( !tlst.empty() )
			{
				for( auto li = tlst.begin(); li != tlst.end(); ++li )
				{
					if( li->id == timerid )
					{
						li->curTick = ticks;
						li->tmr.setTiming(timeMS);

						if( ulog()->debugging(loglevel) )
							ulog()->debug(loglevel) << "(LT_askTimer): заказ на таймер ["
													<< timerid << "]" << getTimerName(timerid) << " " << timeMS << " [мс] уже есть..." << endl;

						return sleepTime;
					}
				}
			}

			// TimerInfo newti(timerid, timeMS, ticks, p);
			tlst.emplace_back(timerid, timeMS, ticks, p);
		}    // unlock

		if( ulog()->debugging(loglevel) )
			ulog()->debug(loglevel) << "(LT_askTimer): поступил заказ на таймер [" << timerid << "]"
									<< getTimerName(timerid) << " " << timeMS << " [мс]\n";
	}
	else // отказ (при timeMS == 0)
	{
		if( ulog()->debugging(loglevel) )
			ulog()->debug(loglevel) << "(LT_askTimer): поступил отказ по таймеру [" << timerid << "]"
									<< getTimerName(timerid) << endl;

		{
			// lock
			uniset_rwmutex_wrlock lock(lstMutex);
			tlst.erase( std::remove_if(tlst.begin(), tlst.end(), Timer_eq(timerid)), tlst.end() );
		}    // unlock
	}

	{
		// lock
		uniset_rwmutex_rlock lock(lstMutex);

		if( tlst.empty() )
			sleepTime = UniSetTimer::WaitUpTime;
		else
			sleepTime = UniSetTimer::MinQuantityTime;
	}

	return sleepTime;
}
// -----------------------------------------------------------------------------
