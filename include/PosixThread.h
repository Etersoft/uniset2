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
 * \brief Интефес для создания и управления потоками
 * \author Anthony Korbin
*/
//---------------------------------------------------------------------------- 
#ifndef PosixThread_h_
#define PosixThread_h_
//----------------------------------------------------------------------------------------
#include <pthread.h>
#include <iostream>
#include <signal.h>
//----------------------------------------------------------------------------------------
/*! \class PosixThread */ 
class PosixThread
{
public:
		PosixThread();
		virtual ~PosixThread();

		void start( void *args );	/*!< запуск */
		void stop();		

		void thrkill( int signo );		/*!< послать сигнал signo */	

		enum TAttr{ SCOPE, DETACH, PRIORITY };

		void setAttr( TAttr Attr, int state );

		pthread_t getTID(){ return tid; }

		void setPriority( int priority );

		static void* funcp( void * test );



protected:

		void reinit();	

		virtual void work() = 0; /*!< Функция выполняемая в потоке */

		void readlock( pthread_rwlock_t *lock = &lockx );
		void writelock( pthread_rwlock_t *lock = &lockx );
		void lock( pthread_mutex_t *mute = &mutex );
		void unlock( pthread_mutex_t *mute = &mutex );
		void rwunlock( pthread_rwlock_t *lock = &lockx );
		void wait( pthread_cond_t *cond = &condx, pthread_mutex_t *mute = &mutex );
		void continueRun( pthread_cond_t *cond = &condx );
		void continueRunAll( pthread_cond_t *cond = &condx );
private:
		pthread_t tid;
		pthread_attr_t *attrPtr;

		static pthread_rwlock_t lockx;
		static pthread_mutex_t mutex;
		static pthread_cond_t condx;
	
		static int countThreads;
};

#endif // PosixThread_h_
