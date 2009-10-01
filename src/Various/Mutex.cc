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
 *  \date   $Date: 2007/11/15 23:53:36 $
 *  \version $Id: Mutex.cc,v 1.14 2007/11/15 23:53:36 vpashka Exp $
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
static mutex_atomic_t mutex_atomic_read( mutex_atomic_t* m ){ return (*m); }
static mutex_atomic_t mutex_atomic_set( mutex_atomic_t* m, int val ){ return (*m) = val; }
//static void mutex_atomic_inc( mutex_atomic_t* m ){ (*m)++; }
//static void mutex_atomic_dec( mutex_atomic_t* m ){ (*m)--; }
// -----------------------------------------------------------------------------
uniset_mutex::uniset_mutex():
	cnd(0),
	nm("")
{
	mutex_atomic_set(&locked,0);
	cnd = new omni_condition(&mtx);
}

uniset_mutex::uniset_mutex( string name ):
	nm(name)
{
	mutex_atomic_set(&locked,0);
	cnd = new omni_condition(&mtx);
}

uniset_mutex::~uniset_mutex()
{
	unlock();
	mutex_atomic_set(&locked,0);
	delete cnd;
}

void uniset_mutex::lock()
{
	sem.wait();
	mutex_atomic_set(&locked,1);
}

void uniset_mutex::unlock()
{
	mutex_atomic_set(&locked,0);
	sem.post();
	cnd->signal();
}

bool uniset_mutex::isRelease()
{
	return (bool)!mutex_atomic_read(&locked);
}
// -----------------------------------------------------------------------------
const uniset_mutex &uniset_mutex::operator=(const uniset_mutex& r)
{
	if( this != &r )
		locked = r.locked;

	return *this;
}

uniset_mutex::uniset_mutex( const uniset_mutex& r ):
	cnd(0),
	nm(r.nm)
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
		mutex_atomic_set(&mlock,1);
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

		mutex_atomic_set(&mlock,0);
		mutex->mtx.unlock();
		return;	//	ресурс не захватываем
	}

	mutex_atomic_set(&mlock,1);
	mutex->lock();
	mutex->mtx.unlock();
}

// -----------------------------------------------------------------------------
bool uniset_mutex_lock::lock_ok()
{ 
	return (bool)mutex_atomic_read(&mlock);
}

uniset_mutex_lock::~uniset_mutex_lock()
{	
	if( mutex_atomic_read(&mlock) )
	{
		mutex_atomic_set(&mlock,0);
		mutex->unlock();
	}
}
// -----------------------------------------------------------------------------
uniset_mutex_lock& uniset_mutex_lock::operator=(const uniset_mutex_lock &r)
{
	return *this;
}
// -----------------------------------------------------------------------------
uniset_spin_mutex::uniset_spin_mutex()
{
	unlock();	
}

uniset_spin_mutex::~uniset_spin_mutex()
{
	unlock();
}

const uniset_spin_mutex &uniset_spin_mutex::operator=( const uniset_spin_mutex& r )
{
	if( this != &r )
		unlock();

	return *this;
}

uniset_spin_mutex::uniset_spin_mutex( const uniset_spin_mutex& r )
{
	unlock();
}

void uniset_spin_mutex::lock( int check_pause_msec )
{
	while( mutex_atomic_read(&m) != 0 )
	{
		if( check_pause_msec > 0 )
			msleep(check_pause_msec);
	}
	mutex_atomic_set(&m,1);
}

void uniset_spin_mutex::unlock()
{
	m = 0;
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

uniset_spin_lock::uniset_spin_lock( const uniset_spin_lock& r ):
m(r.m)
{

}

uniset_spin_lock& uniset_spin_lock::operator=(const uniset_spin_lock& r)
{
	if( this != &r )
		m = r.m;

	return *this;
}
// -----------------------------------------------------------------------------
#undef MUTEX_LOCK_SLEEP_MS
// -----------------------------------------------------------------------------
