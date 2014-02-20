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

#include <chrono>
#include <thread>
#include <unistd.h>
#include "UniSetTypes.h"
#include "Mutex.h"
#include "Configuration.h"

// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
// -----------------------------------------------------------------------------
#define MUTEX_DEBUG(m) {}

uniset_mutex::uniset_mutex():
    nm("")
{
}
// -----------------------------------------------------------------------------
uniset_mutex::uniset_mutex( const string& name ):
    nm(name)
{
}
// -----------------------------------------------------------------------------
uniset_mutex::~uniset_mutex()
{
}
// -----------------------------------------------------------------------------
std::ostream& UniSetTypes::operator<<(std::ostream& os, uniset_mutex& m )
{
    return os << m.name();
}
// -----------------------------------------------------------------------------
void uniset_mutex::lock()
{
    m_lock.lock();
    MUTEX_DEBUG(cerr << nm << " Locked.." << endl;)
}
// -----------------------------------------------------------------------------
void uniset_mutex::unlock()
{
    m_lock.unlock();
    MUTEX_DEBUG(cerr << nm << " Unlocked.." << endl;)
}
// -----------------------------------------------------------------------------
bool uniset_mutex::try_lock_for( const time_t& msec )
{
     return m_lock.try_lock_for( std::chrono::milliseconds(msec) );
}
// -----------------------------------------------------------------------------
uniset_mutex_lock::uniset_mutex_lock( uniset_mutex& m, const time_t timeMS ):
    mutex(&m),
    locked(false)
{

    if( timeMS == 0 )
    {
        mutex->lock();
        locked = true;
        return;
    }

    if( !mutex->try_lock_for(timeMS) )
    {
        if( !mutex->name().empty() )
        {
            ulog9 << "(mutex_lock): вышло заданное время ожидания " 
                << timeMS << " msec для " << mutex->name() << endl;
        }
        return;
    }

    // здесь мы уже под защитой mutex..
    locked = true;
}
// -----------------------------------------------------------------------------
bool uniset_mutex_lock::lock_ok()
{
    return locked.load();
}
// -----------------------------------------------------------------------------
uniset_mutex_lock::~uniset_mutex_lock()
{
    if( locked)
    {
        mutex->unlock();
        locked = false;
    }
}
// -----------------------------------------------------------------------------
uniset_rwmutex::uniset_rwmutex( const std::string& name ):
nm(name)
{
}

uniset_rwmutex::uniset_rwmutex():
nm("")
{
}

uniset_rwmutex::~uniset_rwmutex()
{
}

std::ostream& UniSetTypes::operator<<(std::ostream& os, uniset_rwmutex& m )
{
    return os << m.name();
}

std::atomic<int> uniset_rwmutex::num(0);

const uniset_rwmutex &uniset_rwmutex::operator=( const uniset_rwmutex& r )
{
    if( this != &r )
    {
        lock();
        MUTEX_DEBUG(cerr << "...copy mutex...(" << r.nm << " --> " << nm << ")" << endl;)
        ostringstream s;
        s << r.nm << "." << (++num);
        nm = s.str();
        unlock();
    }

    return *this;
}

uniset_rwmutex::uniset_rwmutex( const uniset_rwmutex& r )
{
    if( this != &r )
    {
        lock();
        MUTEX_DEBUG(cerr << "...copy constr mutex...(" << r.nm << " --> " << nm << ")" << endl;)
        ostringstream s;
        s << r.nm << "." << (++num);
        nm = s.str();
        unlock();
    }
}

void uniset_rwmutex::lock()
{
    MUTEX_DEBUG(cerr << nm << " prepare Locked.." << endl;)
    m.writeLock();
    MUTEX_DEBUG(cerr << nm << " Locked.." << endl;)
}
void uniset_rwmutex::wrlock()
{
    MUTEX_DEBUG(cerr << nm << " prepare WRLocked.." << endl;)
    m.writeLock();
    MUTEX_DEBUG(cerr << nm << " WRLocked.." << endl;)
}

void uniset_rwmutex::rlock()
{
    MUTEX_DEBUG(cerr << nm << " prepare RLocked.." << endl;)
    m.readLock();
    MUTEX_DEBUG(cerr << nm << " RLocked.." << endl;)
}

void uniset_rwmutex::unlock()
{
    m.unlock();
    MUTEX_DEBUG(cerr << nm << " Unlocked.." << endl;)
}

bool uniset_rwmutex::tryrlock()
{
    return m.tryReadLock();
}

bool uniset_rwmutex::trywrlock()
{
    return m.tryWriteLock();
}

bool uniset_rwmutex::trylock()
{
    return m.tryWriteLock();
}
// -------------------------------------------------------------------------------------------
uniset_rwmutex_wrlock::uniset_rwmutex_wrlock( uniset_rwmutex& _m ):
m(_m)
{
    m.wrlock();
}

uniset_rwmutex_wrlock::~uniset_rwmutex_wrlock()
{
    m.unlock();
}
// -------------------------------------------------------------------------------------------
uniset_rwmutex_rlock::uniset_rwmutex_rlock( uniset_rwmutex& _m ):
m(_m)
{
    m.rlock();
}

uniset_rwmutex_rlock::~uniset_rwmutex_rlock()
{
    m.unlock();
}
// -----------------------------------------------------------------------------
