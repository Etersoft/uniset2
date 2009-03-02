/* This file is part of the UniSet project
 * Copyright (c) 2002 Free Software Foundation, Inc.
 * Copyright (c) 2002 Pavel Vainerman <pv>
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
//----------------------------------------------------------------------------------------
/*! \file
 * \brief Создатель потоков
 * \author Pavel Vainerman <pv>
 * \date $Date: 2005/01/28 20:52:21 $
 * \version $Id: ThreadCreator.h,v 1.5 2005/01/28 20:52:21 vitlav Exp $
 */
//--------------------------------------------------------------------------------
#ifndef ThreadCreator_h_
#define ThreadCreator_h_
//----------------------------------------------------------------------------------------
#include "PosixThread.h"
//----------------------------------------------------------------------------------------
/*! \class ThreadCreator
 *	Шаблон для создания потоков с указанием функции вызова. 
 * Пример использования:
 *	
	\code
		class MyClass
		{
			public:
				MyClass();
				~MyClass();
				
				execute();
			
			protected:
				void thread();
				
			private:
				ThreadCreator<MyClass>* thr;
		};
		
		MyClass::MyClass()
		{
			thr = new ThreadCreator<MyClass>(this, &MyClass::thread);
		}
		MyClass::~MyClass()
		{
			delete thr;
		}

		void MyClass::thread()
		{
			while(active)
			{
				//что-то делать
			}
		}

		void MyClass::execute()
		{
			// создаем поток
			thr->start();

			// делаем что-то еще
		}

		main()
		{
			MyClass* mc = new MyClass();
			mc->execute();
		}
	\endcode
 *
*/ 
//----------------------------------------------------------------------------------------
template<class ThreadMaster>
class ThreadCreator:
	protected PosixThread
{
	public:
	
		/*! прототип функции вызова */
		typedef void(ThreadMaster::* Action)(void);	

		pthread_t start();
		pthread_t getTID(){ return PosixThread::getTID(); }

		inline void stop()
		{
			PosixThread::stop();
		}
		
		inline void kill( int signo )		/*!< послать сигнал signo */	
		{
			PosixThread::thrkill(signo);
		}

		ThreadCreator( ThreadMaster* m, Action a );
		~ThreadCreator();

	protected:
		virtual void work(); /*!< Функция выполняемая в потоке */

	private:
		ThreadCreator();
	
		ThreadMaster* m;
		Action act;

};

//----------------------------------------------------------------------------------------
template <class ThreadMaster>
ThreadCreator<ThreadMaster>::ThreadCreator( ThreadMaster* m, Action a ):
	m(m),
	act(a)
{
}
//----------------------------------------------------------------------------------------
template <class ThreadMaster>
void ThreadCreator<ThreadMaster>::work()
{
	if(m)
		(m->*act)();
//	PosixThread::stop()
}
//----------------------------------------------------------------------------------------
template <class ThreadMaster>
pthread_t ThreadCreator<ThreadMaster>::start()
{
	PosixThread::start( static_cast<PosixThread*>(this) );
	return getTID();
}

//----------------------------------------------------------------------------------------
template <class ThreadMaster>
ThreadCreator<ThreadMaster>::ThreadCreator():
	m(0),
	act(0)
{
}
//----------------------------------------------------------------------------------------
template <class ThreadMaster>
ThreadCreator<ThreadMaster>::~ThreadCreator()
{
}
//----------------------------------------------------------------------------------------
#endif // ThreadCreator_h_
