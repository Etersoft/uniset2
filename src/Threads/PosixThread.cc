/* This file is part of the UniSet project
 * Copyright (c) 2002 Free Software Foundation, Inc.
 * Copyright (c) 2002 Anton Korbin <ahtoh>
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
 * \author Anthony Korbin
*/
// --------------------------------------------------------------------------
#include "PosixThread.h"
#include <iostream>
using namespace std;

int PosixThread::countThreads = 1;
pthread_rwlock_t PosixThread::lockx = PTHREAD_RWLOCK_INITIALIZER;
pthread_mutex_t PosixThread::mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t PosixThread::condx = PTHREAD_COND_INITIALIZER;

//----------------------------------------------------------------------------------------

PosixThread::PosixThread():
	tid(0),
	attrPtr(NULL)
{
}

//----------------------------------------------------------------------------------------

PosixThread::~PosixThread()
{
	countThreads--;
	delete attrPtr;
//	pthread_exit( NULL );
}

//----------------------------------------------------------------------------------------

void PosixThread::start( void *args )
{
	if( tid )
	{
		cerr << "startThread: You don't RECREATE thread!" << endl;
		return;
	}


	reinit();

//	cout << "Создаю поток..."<< endl;

	if( pthread_create( &tid, attrPtr, PosixThread::funcp, args ) == -1 )
		{/*throw ThreadNotCreate;*/}
	else
	{
		countThreads++;
		pthread_attr_destroy( attrPtr );
	}

//	cout << "создал поток..."<< endl;
}

//----------------------------------------------------------------------------------------

void PosixThread::stop()
{
	unlock();
	rwunlock();

	tid = 0;
	countThreads--;

	pthread_exit( NULL );
//	cout << "PosixThread: завершил поток"<< endl;
//	tid = 0;
//	countThreads--;

}

//----------------------------------------------------------------------------------------
void PosixThread::reinit()
{
	if (attrPtr != NULL)
		delete attrPtr;

	attrPtr = new pthread_attr_t;
	if( pthread_attr_init( attrPtr ) == -1 )
		{/*throw AttrNotInit;*/cerr << "PosixThread(reinit): не удалось..."<< endl;}

}
//----------------------------------------------------------------------------------------

void PosixThread::thrkill( int signo )
{
	pthread_kill( tid, signo );
}

//----------------------------------------------------------------------------------------

void PosixThread::setAttr( TAttr attr, int state )
{
	switch( attr )
	{
		case SCOPE:
		{
			if( state == PTHREAD_SCOPE_SYSTEM )
				pthread_attr_setscope( attrPtr, PTHREAD_SCOPE_SYSTEM );
			else
				pthread_attr_setscope( attrPtr, PTHREAD_SCOPE_PROCESS );
			break;
		}
		case DETACH:
		{
			if( state == PTHREAD_CREATE_DETACHED )
				pthread_attr_setdetachstate( attrPtr, PTHREAD_CREATE_DETACHED );
		else
				pthread_attr_setdetachstate( attrPtr, PTHREAD_CREATE_JOINABLE );
			break;
		}
		case PRIORITY:
		{
			struct sched_param sched;
			sched.sched_priority = state;
			pthread_attr_setschedparam( attrPtr, &sched );
			break;
		}
		default:
		{
			cerr << "Attr not change. See parameters setAtr(...)" << endl;
		}
	}
}

//----------------------------------------------------------------------------------------

void PosixThread::setPriority( int priority )
{
	setAttr( PosixThread::PRIORITY, priority );
}

//----------------------------------------------------------------------------------------

void* PosixThread::funcp( void *args )
{
	PosixThread *pt;
	pt = (PosixThread*)args;
	pt->work();
	pthread_exit(NULL);	//???
	return 0;
}

//----------------------------------------------------------------------------------------

void PosixThread::readlock( pthread_rwlock_t *lock )
{
	pthread_rwlock_rdlock( lock );
}

//----------------------------------------------------------------------------------------

void PosixThread::writelock( pthread_rwlock_t *lock )
{
	pthread_rwlock_wrlock( lock );
}

//----------------------------------------------------------------------------------------

void PosixThread::lock( pthread_mutex_t *mute )
{
	pthread_mutex_lock( mute );
}

//----------------------------------------------------------------------------------------

void PosixThread::rwunlock( pthread_rwlock_t *lock )
{
	pthread_rwlock_unlock( lock );
}

//----------------------------------------------------------------------------------------

void PosixThread::unlock( pthread_mutex_t *mute )
{
	pthread_mutex_unlock( mute );
}

//----------------------------------------------------------------------------------------

void PosixThread::wait( pthread_cond_t *cond, pthread_mutex_t *mute )
{
	pthread_cond_wait( cond, mute );
}

//----------------------------------------------------------------------------------------

void PosixThread::continueRun( pthread_cond_t *cond )
{
	pthread_cond_signal( cond );
}

//----------------------------------------------------------------------------------------

void PosixThread::continueRunAll( pthread_cond_t *cond )
{
	pthread_cond_broadcast( cond );
}

//----------------------------------------------------------------------------------------
