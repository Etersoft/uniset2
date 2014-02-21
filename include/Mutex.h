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
 * \author Pavel Vainerman
 */
// -------------------------------------------------------------------------- 
#ifndef UniSet_MUTEX_H_
#define UniSet_MUTEX_H_
// -----------------------------------------------------------------------------------------
#include <string>
#include <atomic>
#include <chrono>
#include <mutex>
#include <cc++/thread.h>
// -----------------------------------------------------------------------------------------
namespace UniSetTypes
{
    class uniset_mutex
    {
        public:
            uniset_mutex();
            uniset_mutex( const std::string& name );
            ~uniset_mutex();

            void lock();
            void unlock();
            bool try_lock_for( const time_t& msec );

            inline std::string name(){ return nm; }
            inline void setName( const std::string& name ){ nm = name; }

        protected:

        private:
            friend class uniset_mutex_lock;
            uniset_mutex(const uniset_mutex& r) = delete;
            uniset_mutex &operator=(const uniset_mutex& r) = delete;
            std::string nm;
            std::timed_mutex m_lock;
     };

    std::ostream& operator<<(std::ostream& os, uniset_mutex& m );
    // -------------------------------------------------------------------------
    /*! \class uniset_mutex_lock
     *  \author Pavel Vainerman
     *
     *  Предназначен для блокирования совместного доступа. Как пользоваться см. \ref MutexHowToPage
     *  \note Если ресурс уже занят, то lock ждет его освобождения...
    */
    class uniset_mutex_lock
    {
        public:
            uniset_mutex_lock( uniset_mutex& m, const time_t timeoutMS=0 );
            ~uniset_mutex_lock();

            bool lock_ok();

        private:
            uniset_mutex* mutex;
            std::atomic_bool locked;

            uniset_mutex_lock(const uniset_mutex_lock&)=delete;
            uniset_mutex_lock& operator=(const uniset_mutex_lock&)=delete;
    };

    // -------------------------------------------------------------------------
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

            bool trylock();
            bool tryrlock();
            bool trywrlock();

            uniset_rwmutex( const uniset_rwmutex& r ) = delete;
            uniset_rwmutex& operator=(const uniset_rwmutex& r)=delete;

            uniset_rwmutex( uniset_rwmutex&& r ) = default;
            uniset_rwmutex& operator=(uniset_rwmutex&& r)=default;

            inline std::string name(){ return nm; }
            inline void setName( const std::string& name ){ nm = name; }

        private:
            std::string nm;
            friend class uniset_rwmutex_lock;
            ost::ThreadLock m;
    };

    std::ostream& operator<<(std::ostream& os, uniset_rwmutex& m );
    // -------------------------------------------------------------------------
    class uniset_rwmutex_wrlock
    {
        public:
            uniset_rwmutex_wrlock( uniset_rwmutex& m );
            ~uniset_rwmutex_wrlock();

        private:
            uniset_rwmutex_wrlock(const uniset_rwmutex_wrlock&)=delete;
            uniset_rwmutex_wrlock& operator=(const uniset_rwmutex_wrlock&)=delete;
            uniset_rwmutex& m;
    };

    class uniset_rwmutex_rlock
    {
        public:
            uniset_rwmutex_rlock( uniset_rwmutex& m );
            ~uniset_rwmutex_rlock();

        private:
            uniset_rwmutex_rlock(const uniset_rwmutex_rlock&)=delete;
            uniset_rwmutex_rlock& operator=(const uniset_rwmutex_rlock&)=delete;
            uniset_rwmutex& m;
    };
    // -------------------------------------------------------------------------
} // end of UniSetTypes namespace

#endif
