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
#define MUTEX_LOCK_SLEEP_MS 20
// -------------------------------------------------------------------------------------------
uniset_mutex::uniset_mutex(): 
	release(1),
	nm("")
{
	ocond = new omni_condition(&omutex);
}

uniset_mutex::uniset_mutex( const string name ):
	release(1),
	nm(name)
{
	ocond = new omni_condition(&omutex);
}

uniset_mutex::~uniset_mutex()
{
	delete ocond;
}

void uniset_mutex::lock()
{
	release = 0;
	omutex.lock();
}

void uniset_mutex::unlock()
{
	omutex.unlock();
	release = 1;
}

const uniset_mutex &uniset_mutex::operator=(const uniset_mutex& r)
{
	if( this != &r )
	{
		release = r.release;
		if( release )
			unlock();
		else
			lock();
	}
	return *this;
}

uniset_mutex::uniset_mutex (const uniset_mutex& r):
	release(r.release),
	nm(r.nm)
{
}

// -------------------------------------------------------------------------------------------
uniset_mutex_lock::uniset_mutex_lock( uniset_mutex& m, int t_msec ):
	mutex(&m)
{
	if( m.isRelease() )
	{
		m.lock();
//		cerr << "....locked.." << endl; 
		return;
	}
	
	if( t_msec > 0 )
	{
		m.lock();
		unsigned long sec, msec;
		omni_thread::get_time(&sec,&msec,t_msec/1000,(t_msec%1000)*1000000);
//		cerr << "....wait mutex msec=" << t_msec << endl; 
//		m.ocond->timedwait(sec, msec);
		if( !m.ocond->timedwait(sec, msec) )
		{
			m.unlock();
			return;
		}
	}
//	m.lock();
}

// -------------------------------------------------------------------------------------------
uniset_mutex_lock::~uniset_mutex_lock()
{	
	mutex->unlock();
	mutex->ocond->signal();
}
// -------------------------------------------------------------------------------------------
uniset_mutex_lock& uniset_mutex_lock::operator=(const uniset_mutex_lock &r)
{
	return *this;
}
// -------------------------------------------------------------------------------------------
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
	lock();
}

void uniset_spin_mutex::unlock()
{
	unlock();
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
