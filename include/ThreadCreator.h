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
//----------------------------------------------------------------------------------------
/*! \file
 * \brief Шаблон дла легкого создания потоков (на основе callback).
 * \author Pavel Vainerman
 */
//--------------------------------------------------------------------------------
#ifndef ThreadCreator_h_
#define ThreadCreator_h_
//----------------------------------------------------------------------------------------
#include <Poco/Thread.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>
//----------------------------------------------------------------------------------------
namespace uniset
{
	/*! \class ThreadCreator
	 *    Шаблон для создания потоков с указанием функции вызова.
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
		public Poco::Runnable
	{
		public:

			/*! прототип функции вызова
			 * use std::function ?
			 */
			typedef void(ThreadMaster::* Action)(void);

			ThreadCreator( ThreadMaster* m, Action a );
			virtual ~ThreadCreator();

			inline Poco::Thread::TID getTID() const
			{
				return thr.tid();
			}

			/*! \return 0 - sucess */
			void setPriority( Poco::Thread::Priority prior );

			/*! \return < 0 - fail */
			Poco::Thread::Priority getPriority() const;

			void stop();
			void start();

			void sleep( long milliseconds );

			inline bool isRunning()
			{
				return thr.isRunning();
			}

			inline void join()
			{
				thr.join();
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

				//delete this;
			}

			virtual void initial()
			{
				if( initm )
					(initm->*initact)();
			}

			virtual void terminate() {}

		private:
			ThreadCreator();

			ThreadMaster* m = { nullptr };
			Action act;

			ThreadMaster* finm = { nullptr };
			Action finact;

			ThreadMaster* initm = { nullptr };
			Action initact;

			Poco::Thread thr;
	};

	//----------------------------------------------------------------------------------------
	template <class ThreadMaster>
	ThreadCreator<ThreadMaster>::ThreadCreator( ThreadMaster* m, Action a ):
		m(m),
		act(a),
		finm(nullptr),
		finact(nullptr),
		initm(nullptr),
		initact(nullptr)
	{
	}
	//----------------------------------------------------------------------------------------
	template <class ThreadMaster>
	void ThreadCreator<ThreadMaster>::run()
	{
		initial();

		if( m )
			(m->*act)();

		final();
	}
	//----------------------------------------------------------------------------------------
	template <class ThreadMaster>
	void ThreadCreator<ThreadMaster>::stop()
	{
		terminate();
	}
	//----------------------------------------------------------------------------------------
	template <class ThreadMaster>
	void ThreadCreator<ThreadMaster>::start()
	{
		thr.start( *this );
	}
	//----------------------------------------------------------------------------------------
	template <class ThreadMaster>
	void ThreadCreator<ThreadMaster>::sleep( long milliseconds )
	{
		thr.sleep(milliseconds);
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
	template <class ThreadMaster>
	void ThreadCreator<ThreadMaster>::setPriority( Poco::Thread::Priority prior )
	{
		return thr.setPriority(prior);
	}
	//----------------------------------------------------------------------------------------
	template <class ThreadMaster>
	Poco::Thread::Priority ThreadCreator<ThreadMaster>::getPriority() const
	{
		return thr.getPriority();
	}
	// -------------------------------------------------------------------------
} // end of uniset namespace
//----------------------------------------------------------------------------------------
#endif // ThreadCreator_h_
