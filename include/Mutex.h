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
 * \date   $Date: 2007/11/18 19:13:35 $
 * \version $Id: Mutex.h,v 1.14 2007/11/18 19:13:35 vpashka Exp $
 */
// -------------------------------------------------------------------------- 
#ifndef UniSet_MUTEX_H_
#define UniSet_MUTEX_H_
// -----------------------------------------------------------------------------------------
#include <string>
#include <omnithread.h>
#include <signal.h>
// -----------------------------------------------------------------------------------------
//#warning Необходимо разобраться с atomic_add и т.п. (и вообще использовать futex-ы)
// -----------------------------------------------------------------------------------------
namespace UniSetTypes
{
	/*! \class uniset_mutex 

	 * \note Напрямую функциями \a lock() и \a unlock() лучше не пользоваться. 
	 * Как пользоваться см. \ref MutexHowToPage
	 * \todo Проверить правильность конструкторов копирования и operator=	 
	*/
	class uniset_mutex
	{
		public:
		    uniset_mutex();
			uniset_mutex( const std::string name );
    		~uniset_mutex();

			inline bool isRelease() const
			{
				return (bool)release;
			};			

			void lock();
		    void unlock();
			
			inline std::string name()
			{
				return nm;
			};
			
		protected:
		
		private:
			friend class uniset_mutex_lock;
		   	uniset_mutex (const uniset_mutex& r);
	   		const uniset_mutex &operator=(const uniset_mutex& r);
			volatile sig_atomic_t release;
			const std::string nm;
			omni_condition* ocond;
			omni_mutex     omutex; 
 	};

	/*! \class uniset_mutex_lock
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
		    uniset_mutex_lock(const uniset_mutex_lock&);
		    uniset_mutex_lock& operator=(const uniset_mutex_lock&);
	};

	// -------------------------------------------------------------------------
	class uniset_spin_mutex:
		public uniset_mutex
	{
		public:
			uniset_spin_mutex();
			~uniset_spin_mutex();
		   	uniset_spin_mutex (const uniset_spin_mutex& r);
	   		const uniset_spin_mutex &operator=(const uniset_spin_mutex& r);
			
			void lock( int check_pause_msec=0 );
			void unlock();

		private:
	};
	// -------------------------------------------------------------------------
	class uniset_spin_lock
	{
		public:
			uniset_spin_lock( uniset_spin_mutex& m, int check_pause_msec=0 );
			~uniset_spin_lock();

		private:
		    uniset_spin_lock(const uniset_spin_lock&);
		    uniset_spin_lock& operator=(const uniset_spin_lock&);
			uniset_spin_mutex& m;
	};
	// -------------------------------------------------------------------------
} // end of UniSetTypes namespace

#endif
