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
 * \author Pavel Vainerman
 */
// --------------------------------------------------------------------------
#ifndef UniSet_MUTEX_H_
#define UniSet_MUTEX_H_
// -----------------------------------------------------------------------------------------
#include <string>
#include <memory>
#include <Poco/RWLock.h>
// -----------------------------------------------------------------------------------------
namespace uniset
{
    // rwmutex..
    class uniset_rwmutex
    {
        public:
            uniset_rwmutex( const std::string& name );
            uniset_rwmutex();
            ~uniset_rwmutex();

            void lock();
            void unlock();

            void wrlock();
            void rlock();

            bool try_lock();
            bool try_rlock();
            bool try_wrlock();

            uniset_rwmutex( const uniset_rwmutex& r ) = delete;
            uniset_rwmutex& operator=(const uniset_rwmutex& r) = delete;

            uniset_rwmutex( uniset_rwmutex&& r ) = default;
            uniset_rwmutex& operator=(uniset_rwmutex&& r) = default;

            inline std::string name() const
            {
                return nm;
            }

            inline void setName( const std::string& name )
            {
                nm = name;
            }

        private:
            std::string nm;
            friend class uniset_rwmutex_lock;
            std::unique_ptr<Poco::RWLock> m;
    };

    std::ostream& operator<<(std::ostream& os, uniset_rwmutex& m );
    // -------------------------------------------------------------------------
    class uniset_rwmutex_wrlock
    {
        public:
            uniset_rwmutex_wrlock( uniset_rwmutex& m );
            ~uniset_rwmutex_wrlock();

        private:
            uniset_rwmutex_wrlock(const uniset_rwmutex_wrlock&) = delete;
            uniset_rwmutex_wrlock& operator=(const uniset_rwmutex_wrlock&) = delete;
            uniset_rwmutex& m;
    };

    class uniset_rwmutex_rlock
    {
        public:
            uniset_rwmutex_rlock( uniset_rwmutex& m );
            ~uniset_rwmutex_rlock();

        private:
            uniset_rwmutex_rlock(const uniset_rwmutex_rlock&) = delete;
            uniset_rwmutex_rlock& operator=(const uniset_rwmutex_rlock&) = delete;
            uniset_rwmutex& m;
    };
    // -------------------------------------------------------------------------
} // end of UniSetTypes namespace

#endif
