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
#include <sstream>
#include <algorithm>
#include "Exceptions.h"
#include "UniSetObject.h"
#include "LT_Object.h"
#include "Debug.h"

// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;

// -----------------------------------------------------------------------------
LT_Object::LT_Object():
lstMutex("LT_Object::lstMutex"),
sleepTime(UniSetTimer::WaitUpTime)
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
        {    // lock
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
                sleepTime=UniSetTimer::MinQuantityTime;
                return sleepTime;
            }
        }

        {    // lock
            uniset_rwmutex_wrlock lock(lstMutex);
            sleepTime = UniSetTimer::WaitUpTime;
            for( auto li=tlst.begin(); li!=tlst.end(); ++li )
            {
                if( li->tmr.checkTime() )
                {
                    // помещаем себе в очередь сообщение
                    TransportMessage tm( std::move(TimerMessage(li->id, li->priority, obj->getId()).transport_msg()) );
                    obj->push(tm);

                    // Проверка на количество заданных тактов
                    if( !li->curTick )
                    {
                        li = tlst.erase(li);
                        if( li == tlst.end() ) --li;

                        if( tlst.empty() )
                            sleepTime = UniSetTimer::WaitUpTime;
                        continue;
                    }
                    else if(li->curTick>0 )
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
                sleepTime=UniSetTimer::MinQuantityTime;
        } // unlock

        tmLast.reset();
    }
    catch( Exception& ex )
    {
        ucrit << "(checkTimers): " << ex << endl;
    }

    return sleepTime;
}
// ------------------------------------------------------------------------------------------

timeout_t LT_Object::askTimer( UniSetTypes::TimerId timerid, timeout_t timeMS, clock_t ticks, UniSetTypes::Message::Priority p )
{
    if( timeMS > 0 ) // заказ
    {
        if( timeMS < UniSetTimer::MinQuantityTime )
        {
            ucrit << "(LT_askTimer): [мс] попытка заказть таймер со временем срабатыания "
                        << " меньше разрешённого " << UniSetTimer::MinQuantityTime << endl;
            timeMS = UniSetTimer::MinQuantityTime;
        }

        {    // lock
            uniset_rwmutex_wrlock lock(lstMutex);
            // поищем а может уж такой есть
            if( !tlst.empty() )
            {
                for( auto li=tlst.begin(); li!=tlst.end(); ++li )
                {
                    if( li->id == timerid )
                    {
                        li->curTick = ticks;
                        li->tmr.setTiming(timeMS);
                        uinfo << "(LT_askTimer): заказ на таймер(id="
                                << timerid << ") " << timeMS << " [мс] уже есть..." << endl;
                        return sleepTime;
                    }
                }
            }

            // TimerInfo newti(timerid, timeMS, ticks, p);
            tlst.push_back( std::move(TimerInfo(timerid, timeMS, ticks, p)) );
        }    // unlock

        uinfo << "(LT_askTimer): поступил заказ на таймер(id="<< timerid << ") " << timeMS << " [мс]\n";
    }
    else // отказ (при timeMS == 0)
    {
        uinfo << "(LT_askTimer): поступил отказ по таймеру id="<< timerid << endl;    
        {    // lock
            uniset_rwmutex_wrlock lock(lstMutex);
            tlst.erase( std::remove_if(tlst.begin(),tlst.end(),Timer_eq(timerid)), tlst.end() );
        }    // unlock
    }

    {    // lock
        uniset_rwmutex_rlock lock(lstMutex);

        if( tlst.empty() )
            sleepTime = UniSetTimer::WaitUpTime;
        else
            sleepTime = UniSetTimer::MinQuantityTime;
    }

    return sleepTime;
}
// -----------------------------------------------------------------------------
