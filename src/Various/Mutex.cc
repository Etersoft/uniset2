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
#ifndef HAVE_LINUX_LIBC_HEADERS_INCLUDE_LINUX_FUTEX_H
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
#else // HAVE_LINUX_FUTEX_H
// -----------------------------------------------------------------------------
// mutex на основе futex
// Идея и реализация взята с http://kerneldump.110mb.com/dokuwiki/doku.php?id=wiki:futexes_are_tricky_p3
// Оригинальная статья: http://people.redhat.com/drepper/futex.pdf

uniset_mutex::uniset_mutex():
	val(0),nm("")
{
}

uniset_mutex::uiset_mutex( std::string name )
	val(0),
	nm(name)
{

}

uniset_mutex::~uniset_mutex()
{
	unlock();	
}

void uniset_mutex::lock() 
{
	int c;
	if( (c = cmpxchg(val, 0, 1))!= 0 )
	{
		do 
		{
			if( c==2 || cmpxchg(val, 1, 2)!=0 )
				futex_wait(&val, 2);
		}
		while( (c = cmpxchg(val, 0, 2))!=0 );
	}
}

void uniset_mutex::unlock()
{
	if( atomic_dec(val)!=1 )
	{
		val = 0;
		futex_wake(&val, 1);
	}
}

bool uniset_mutex::isRelease()
{
	return (bool)cmpxchg(val, 1, 2);
}

// -----------------------------------------------------------------------------
#endif // HAVE_LINUX_FUTEX_H
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
#ifndef HAVE_LINUX_LIBC_HEADERS_INCLUDE_LINUX_FUTEX_H

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

#else // HAVE_FUTEX

// mutex на основе futex
// Идея и реализация взята с http://kerneldump.110mb.com/dokuwiki/doku.php?id=wiki:futexes_are_tricky_p3
// Оригинальная статья: http://people.redhat.com/drepper/futex.pdf
void uniset_spin_mutex::lock( int check_pause_msec )
{
	struct timespec tm;
	tm.tv_sec 	= check_pause_msec / 1000;
	tm.tv_nsec 	= check_pause_msec%1000;

	int c;
	if( (c = cmpxchg(val, 0, 1))!= 0 )
	{
		do 
		{
			if( c==2 || cmpxchg(val, 1, 2)!=0 )
			{
				if( futex_wait(&val, 2,tm) == ETIMEDOUT )
					return;
			}
		}
		while( (c = cmpxchg(val, 0, 2))!=0 );
	}
}

void uniset_spin_mutex::unlock()
{
	if( atomic_dec(val)!=1 )
	{
		val = 0;
		futex_wake(&val, 1);
	}
}
#endif // HAVE_FUTEX

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
