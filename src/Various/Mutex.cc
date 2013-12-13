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
void uniset_mutex::lock()
{
	sem.wait();
	locked = 1;
}
// -----------------------------------------------------------------------------
void uniset_mutex::unlock()
{
	locked = 0;
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
uniset_spin_mutex::uniset_spin_mutex():
wr_wait(0)
{
}

uniset_spin_mutex::~uniset_spin_mutex()
{
}

const uniset_spin_mutex &uniset_spin_mutex::operator=( const uniset_spin_mutex& r )
{
	if( this != &r )
		unlock();

	return *this;
}

uniset_spin_mutex::uniset_spin_mutex( const uniset_spin_mutex& r )
{
	//unlock();
}

void uniset_spin_mutex::lock( int check_pause_msec )
{
	wr_wait += 1;
	while( !m.tryWriteLock() )
	{
		if( check_pause_msec > 0 )
			msleep(check_pause_msec);
	}
	wr_wait -= 1;
}
void uniset_spin_mutex::wrlock( int check_pause_msec )
{
	wr_wait += 1;
	while( !m.tryWriteLock() )
	{
		if( check_pause_msec > 0 )
			msleep(check_pause_msec);
	}
	wr_wait -= 1;
}
void uniset_spin_mutex::rlock( int check_pause_msec )
{
	while( wr_wait > 0 )
		msleep(check_pause_msec);

	while( !m.tryReadLock() )
	{
		if( check_pause_msec > 0 )
			msleep(check_pause_msec);
	}
}

void uniset_spin_mutex::unlock()
{
	m.unlock();
}
// -------------------------------------------------------------------------------------------
uniset_spin_lock::uniset_spin_lock( uniset_spin_mutex& _m, int check_pause_msec ):
	m(_m)
{
	m.lock(check_pause_msec);
}

uniset_spin_lock::~uniset_spin_lock()
{
	m.unlock();
}

uniset_spin_wrlock::uniset_spin_wrlock( uniset_spin_mutex& _m, int check_pause_msec ):
	uniset_spin_lock(_m)
{
	m.wrlock(check_pause_msec);
}

uniset_spin_wrlock::~uniset_spin_wrlock()
{
	// unlocked in uniset_spin_lock destructor
}

uniset_spin_wrlock::uniset_spin_wrlock( const uniset_spin_wrlock& r ):
uniset_spin_lock(r.m)
{

}

uniset_spin_wrlock& uniset_spin_wrlock::operator=(const uniset_spin_wrlock& r)
{
	if( this != &r )
		m = r.m;

	return *this;
}
// -------------------------------------------------------------------------------------------
uniset_spin_rlock::uniset_spin_rlock( uniset_spin_mutex& _m, int check_pause_msec ):
uniset_spin_lock(_m)
{
	m.rlock(check_pause_msec);
}

uniset_spin_rlock::~uniset_spin_rlock()
{
	// unlocked in uniset_spin_lock destructor
}

uniset_spin_rlock::uniset_spin_rlock( const uniset_spin_rlock& r ):
uniset_spin_lock(r.m)
{

}

uniset_spin_rlock& uniset_spin_rlock::operator=(const uniset_spin_rlock& r)
{
	if( this != &r )
		m = r.m;

	return *this;
}

// -----------------------------------------------------------------------------
#undef MUTEX_LOCK_SLEEP_MS
// -----------------------------------------------------------------------------
