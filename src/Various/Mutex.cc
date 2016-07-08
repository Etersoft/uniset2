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

bool uniset_rwmutex::try_rlock()
{
	return m.tryReadLock();
}

bool uniset_rwmutex::try_wrlock()
{
	return m.tryWriteLock();
}

bool uniset_rwmutex::try_lock()
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
