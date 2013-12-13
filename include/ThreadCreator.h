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
//----------------------------------------------------------------------------------------
/*! \file
 * \brief Создатель потоков
 * \author Pavel Vainerman
 */
//--------------------------------------------------------------------------------
#ifndef ThreadCreator_h_
#define ThreadCreator_h_
//----------------------------------------------------------------------------------------
#include <cc++/thread.h>
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
	public ost::PosixThread
{
	public:

		/*! прототип функции вызова */
		typedef void(ThreadMaster::* Action)(void);

		ThreadCreator( ThreadMaster* m, Action a );
		~ThreadCreator();

		void stop();

		inline void setPriority( int ptior )
		{
			#warning ThreadCreator::setPriority NOT REALIZED YET
		}

	inline void setCancel( ost::Thread::Cancel mode )
		{
			ost::PosixThread::setCancel(mode);
		}

		inline void setFinalAction( ThreadMaster* m, Action a )
		{
			finm = m;
			finact = a;
		}

		inline void setInitialAction( ThreadMaster* m, Action a )
		{
			initm = m;
			initact = a;
		}

	protected:
		virtual void run();
		virtual void final()
		{
			if( finm )
				(finm->*finact)();
		}

		virtual void initial()
		{
			if( initm )
				(initm->*initact)();
		}

	private:
		ThreadCreator();
	
		ThreadMaster* m;
		Action act;

		ThreadMaster* finm;
		Action finact;

		ThreadMaster* initm;
		Action initact;
};

//----------------------------------------------------------------------------------------
template <class ThreadMaster>
ThreadCreator<ThreadMaster>::ThreadCreator( ThreadMaster* m, Action a ):
	m(m),
	act(a),
	finm(0),
	finact(0),
	initm(0),
	initact(0)
{
}
//----------------------------------------------------------------------------------------
template <class ThreadMaster>
void ThreadCreator<ThreadMaster>::run()
{
	if(m)
		(m->*act)();
//	PosixThread::stop()
}
//----------------------------------------------------------------------------------------
template <class ThreadMaster>
void ThreadCreator<ThreadMaster>::stop()
{
	terminate();
}
//----------------------------------------------------------------------------------------
template <class ThreadMaster>
ThreadCreator<ThreadMaster>::ThreadCreator():
	m(0),
	act(0),
	finm(0),
	finact(0),
	initm(0),
	initact(0)
{
}
//----------------------------------------------------------------------------------------
template <class ThreadMaster>
ThreadCreator<ThreadMaster>::~ThreadCreator()
{
}
//----------------------------------------------------------------------------------------
#endif // ThreadCreator_h_
