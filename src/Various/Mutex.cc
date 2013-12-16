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
	cnd(0),
	nm(""),
	locked(0)
{
	cnd = new omni_condition(&mtx);
}
// -----------------------------------------------------------------------------
uniset_mutex::uniset_mutex( string name ):
	cnd(0),
	nm(name),
	locked(0)
{
	cnd = new omni_condition(&mtx);
}
// -----------------------------------------------------------------------------
uniset_mutex::~uniset_mutex()
{
	delete cnd;
}
// -----------------------------------------------------------------------------
std::ostream& UniSetTypes::operator<<(std::ostream& os, uniset_mutex& m )
{
	return os << m.name();
}
// -----------------------------------------------------------------------------
void uniset_mutex::lock()
{
	sem.wait();
	locked = 1;

	MUTEX_DEBUG(cerr << nm << " Locked.." << endl;)
}
// -----------------------------------------------------------------------------
void uniset_mutex::unlock()
{
	locked = 0;
	MUTEX_DEBUG(cerr << nm << " Unlocked.." << endl;)
	sem.post();
	cnd->signal();
}
// -----------------------------------------------------------------------------
bool uniset_mutex::isRelease()
{
	return !locked;
}
// -----------------------------------------------------------------------------
const uniset_mutex &uniset_mutex::operator=(const uniset_mutex& r)
{
//	if( this != &r )
//		locked = r.locked;

	return *this;
}
// -----------------------------------------------------------------------------
uniset_mutex::uniset_mutex( const uniset_mutex& r ):
	cnd(0),
	nm(r.nm),
	locked(r.locked)
{
	cnd = new omni_condition(&mtx);
}

// -----------------------------------------------------------------------------
uniset_mutex_lock::uniset_mutex_lock( uniset_mutex& m, int timeMS ):
	mutex(&m)
{
	if( timeMS <= 0 || mutex->isRelease() )
	{
		mutex->lock();
		mlock = 1;
		return;
	}

	unsigned long sec, msec;
	omni_thread::get_time(&sec,&msec, timeMS/1000, (timeMS%1000)*1000000 );
	mutex->mtx.lock();
	if( !mutex->cnd->timedwait(sec, msec) )
	{
		if( !mutex->name().empty() && unideb.debugging( Debug::type(Debug::LEVEL9|Debug::WARN)) )
		{
			unideb[Debug::type(Debug::LEVEL9|Debug::WARN)] 
				<< "(mutex_lock): вышло заданное время ожидания " 
				<< timeMS << " msec для " << mutex->name() << endl;
		}

		mlock = 0;
		mutex->mtx.unlock();
		return;	//	ресурс не захватываем
	}

	mlock = 1;
	mutex->lock();
	mutex->mtx.unlock();
}
// -----------------------------------------------------------------------------
bool uniset_mutex_lock::lock_ok()
{ 
	return mlock;
}

uniset_mutex_lock::~uniset_mutex_lock()
{	
	if( mlock )
	{
		mlock = 0;
		mutex->unlock();
	}
}
// -----------------------------------------------------------------------------
uniset_mutex_lock& uniset_mutex_lock::operator=(const uniset_mutex_lock &r)
{
	return *this;
}
// -----------------------------------------------------------------------------
uniset_rwmutex::uniset_rwmutex( const std::string& name ):
nm(name),
wr_wait(0)
{

}

int uniset_rwmutex::num  = 0;

uniset_rwmutex::uniset_rwmutex():
nm(""),
wr_wait(0)
{
}

uniset_rwmutex::~uniset_rwmutex()
{
}

std::ostream& UniSetTypes::operator<<(std::ostream& os, uniset_rwmutex& m )
{
	return os << m.name();
}

const uniset_rwmutex &uniset_rwmutex::operator=( const uniset_rwmutex& r )
{
	if( this != &r )
	{
		lock();
		ostringstream s;
		s << r.nm << "." << (++num);
		nm = s.str();
		unlock();
	}

	return *this;
}

uniset_rwmutex::uniset_rwmutex( const uniset_rwmutex& r ):
nm(r.nm)
{
}

void uniset_rwmutex::lock()
{
	MUTEX_DEBUG(cerr << nm << " prepare Locked.." << endl;)
	wr_wait += 1;
	m.writeLock();
	wr_wait -= 1;
	MUTEX_DEBUG(cerr << nm << " Locked.." << endl;)
}
void uniset_rwmutex::wrlock()
{
	MUTEX_DEBUG(cerr << nm << " prepare WRLocked.." << endl;)
	wr_wait += 1;
	m.writeLock();
	wr_wait -= 1;
	MUTEX_DEBUG(cerr << nm << " WRLocked.." << endl;)
}
void uniset_rwmutex::rlock()
{
	MUTEX_DEBUG(cerr << nm << " prepare RLocked.." << endl;)

	while( wr_wait > 0 )
		msleep(2);

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

uniset_rwmutex_wrlock::uniset_rwmutex_wrlock( const uniset_rwmutex_wrlock& r ):
 m(r.m)
{

}

uniset_rwmutex_wrlock& uniset_rwmutex_wrlock::operator=(const uniset_rwmutex_wrlock& r)
{
	if( this != &r )
		m = r.m;

	return *this;
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

uniset_rwmutex_rlock::uniset_rwmutex_rlock( const uniset_rwmutex_rlock& r ):
m(r.m)
{

}

uniset_rwmutex_rlock& uniset_rwmutex_rlock::operator=(const uniset_rwmutex_rlock& r)
{
	if( this != &r )
		m = r.m;

	return *this;
}
// -----------------------------------------------------------------------------
