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
 * \brief Создатель потоков
 * \author Pavel Vainerman
 */
// -------------------------------------------------------------------------- 
#ifndef OmniThreadCreator_h_
#define OmniThreadCreator_h_
//---------------------------------------------------------------------------
#include <omnithread.h>
#include <memory>
//----------------------------------------------------------------------------
/*! \class OmniThreadCreator
 * \par
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
                OmniThreadCreator<MyClass>* thr;
        };

        MyClass::MyClass()
        {
            thr = new OmniThreadCreator<MyClass>(this, &MyClass::thread);
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

            // или так
            OmniThreadCreator<MyClass>*  th = new OmniThreadCreator<TestClass>(&mc, &MyClass::thread);
            th->start();
            // делаем что-то еще
        }
    \endcode
 *
*/ 
//----------------------------------------------------------------------------------------
template<class ThreadMaster>
class OmniThreadCreator:
    public omni_thread
{
    public:

        /*! прототип функции вызова */
        typedef void(ThreadMaster::* Action)();

        OmniThreadCreator( const std::shared_ptr<ThreadMaster>& m, Action a, bool undetached=false );
        ~OmniThreadCreator(){}

       inline bool isRunning(){ return state()==omni_thread::STATE_RUNNING; }
       inline void stop(){ exit(0); }
       inline pid_t getTID(){ return id(); }

        inline void join(){ omni_thread::join(NULL); }

    protected:
        void* run_undetached(void *x)
        {
            if(m)
                (m.get()->*act)();

            return (void*)0;
        }

        virtual void run(void* arg)
        {
            if(m)
                (m.get()->*act)();
        }

    private:
        OmniThreadCreator();
        std::shared_ptr<ThreadMaster> m;
        Action act;
};

//----------------------------------------------------------------------------------------
template <class ThreadMaster>
OmniThreadCreator<ThreadMaster>::OmniThreadCreator( const std::shared_ptr<ThreadMaster>& _m, Action a, bool undetach  ):
    omni_thread(),
    m(_m),
    act(a)
{
    if(undetach)
         start_undetached();
}
//----------------------------------------------------------------------------------------

template <class ThreadMaster>
OmniThreadCreator<ThreadMaster>::OmniThreadCreator():
    m(0),
    act(0)
{
}
//----------------------------------------------------------------------------------------

#endif // OmniThreadCreator_h_
