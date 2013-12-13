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
#include <omnithread.h>
#include <signal.h>
#include <cc++/thread.h>
// -----------------------------------------------------------------------------------------
namespace UniSetTypes
{
	typedef ost::AtomicCounter mutex_atomic_t;

	/*! \class uniset_mutex 

	 * \note Напрямую функциями \a lock() и \a unlock() лучше не пользоваться. 
	 * Как пользоваться см. \ref MutexHowToPage
	 * \todo Проверить правильность конструкторов копирования и operator=
	*/
	class uniset_mutex
	{
		public:
		    uniset_mutex();
			uniset_mutex( std::string name );
    		~uniset_mutex();

			bool isRelease();
			void lock();
		    void unlock();
			
			inline std::string name(){ return nm; }
			inline void setName( const std::string& name ){ nm = name; }
			
		protected:
		
		private:
			friend class uniset_mutex_lock;
		   	uniset_mutex (const uniset_mutex& r);
	   		const uniset_mutex &operator=(const uniset_mutex& r);
			omni_condition* cnd;
			std::string nm;
			omni_semaphore sem;
			omni_mutex mtx;
			mutex_atomic_t locked;
 	};

	std::ostream& operator<<(std::ostream& os, uniset_mutex& m );
	// -------------------------------------------------------------------------
	/*! \class uniset_mutex_lock
	 * \author Pavel Vainerman
	 *
	 *	Предназначен для блокирования совместного доступа. Как пользоваться см. \ref MutexHowToPage
	 *	\note Если ресурс уже занят, то lock ждет его освобождения... 
	 *	\warning Насколько ожидание защищено от зависания надо еще проверять!
	 * 	\todo Может на откуп пользователям оставить проверку занятости ресурса перед захватом? может не ждать?
	*/
	class uniset_mutex_lock
	{
		public:
			uniset_mutex_lock( uniset_mutex& m, int timeoutMS=0 );
		    ~uniset_mutex_lock();
			
			bool lock_ok();

		private:
		   	uniset_mutex* mutex;
			mutex_atomic_t mlock;
		    uniset_mutex_lock(const uniset_mutex_lock&);
		    uniset_mutex_lock& operator=(const uniset_mutex_lock&);
	};

	// -------------------------------------------------------------------------
	// Mutex c приоритетом WRlock над RLock...
	class uniset_rwmutex
	{
		public:
			uniset_rwmutex( const std::string& name );
			uniset_rwmutex();
			~uniset_rwmutex();

			void lock( int check_pause_msec=1 );
			void unlock();

			void wrlock( int check_pause_msec=1 );
			void rlock( int check_pause_msec=1 );

			uniset_rwmutex (const uniset_rwmutex& r);
			const uniset_rwmutex &operator=(const uniset_rwmutex& r);

			inline std::string name(){ return nm; }
			inline void setName( const std::string& name ){ nm = name; }

		private:
			std::string nm;
			friend class uniset_rwmutex_lock;
			ost::ThreadLock m;
			ost::AtomicCounter wr_wait;
	};
	std::ostream& operator<<(std::ostream& os, uniset_rwmutex& m );
	// -------------------------------------------------------------------------
	class uniset_rwmutex_wrlock
	{
		public:
			uniset_rwmutex_wrlock( uniset_rwmutex& m, int check_pause_msec=1 );
			~uniset_rwmutex_wrlock();

		private:
		    uniset_rwmutex_wrlock(const uniset_rwmutex_wrlock&);
		    uniset_rwmutex_wrlock& operator=(const uniset_rwmutex_wrlock&);
			uniset_rwmutex& m;
	};

	class uniset_rwmutex_rlock
	{
		public:
			uniset_rwmutex_rlock( uniset_rwmutex& m, int check_pause_msec=5 );
			~uniset_rwmutex_rlock();

		private:
		    uniset_rwmutex_rlock(const uniset_rwmutex_rlock&);
		    uniset_rwmutex_rlock& operator=(const uniset_rwmutex_rlock&);
			uniset_rwmutex& m;
	};
	// -------------------------------------------------------------------------
} // end of UniSetTypes namespace

#endif
