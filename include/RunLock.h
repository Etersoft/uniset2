/*
 * Copyright (c) 2021 Pavel Vainerman.
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
#ifndef RunLock_H_
#define RunLock_H_
// ---------------------------------------------------------------------------
#include <string>
// ---------------------------------------------------------------------------
namespace uniset {
// ---------------------------------------------------------------------------
/*!
 * RunLock неявно использует pid процесса который вызывает lock();
 */
class RunLock
{
    public:
        RunLock( const std::string& lockfile );
		~RunLock();
		
        bool isLocked() const;
        bool lock() const;
        bool unlock() const;
        bool isLockOwner() const;

    protected:
        const std::string lockfile;
	
};
// ----------------------------------------------------------------------------
} // end of namespace uniset
// ----------------------------------------------------------------------------
#endif

