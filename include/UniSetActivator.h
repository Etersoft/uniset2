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
 * \brief Активатор объектов
 * \author Pavel Vainerman
 */
// -------------------------------------------------------------------------- 
#ifndef UniSetActivator_H_
#define UniSetActivator_H_
// -------------------------------------------------------------------------- 
#include <omniORB4/CORBA.h>
#include "UniSetTypes.h"
#include "UniSetObject.h"
#include "UniSetManager.h"
#include "ThreadCreator.h"
//#include "OmniThreadCreator.h"
//----------------------------------------------------------------------------------------
/*! \class UniSetActivator
 *    Создает POA менеджер и регистрирует в нем объекты. 
 *    Для обработки CORBA-запросов создается поток или передаются ресурсы 
 *        главного потока см. void activate(bool thread)
 *    
 *    Активатор в свою очередь сам является менеджером(и объектом) и обладает всеми его свойствами    
 *
 * \todo  Разобраться со всякими oaDestroy, stop, oakill и сделать одну надежную завершающую функцию.
*/ 
class UniSetActivator: 
    public UniSetManager
{
    public:
    
        UniSetActivator();
        UniSetActivator( UniSetTypes::ObjectId id );
        virtual ~UniSetActivator();

        virtual void run(bool thread);
        virtual void stop();
        virtual void oaDestroy(int signo=0);
        void waitDestroy();
        
        inline void oakill(int signo){ raise(signo);}

        virtual UniSetTypes::ObjectType getType(){ return UniSetTypes::getObjectType("UniSetActivator"); }
        
    protected:


        /*! Команды доступные при заказе сигналов 
         * см. askSignal()
        */
        enum AskSigCommand    {
                                Ask,     /*!< заказать получение сигнала */
                                Denial /*!< отказаться от получения сигнала */
                            };

        /*! заказ на обработку сигнала signo
         * Для обработки предназначена функция signal().
         * \warning Сообщение о приходе сигналов SITERM, SIGINT, SIGABRT приходит
         * вне зависимости от заказа. От этих сообщений нельзя отказаться... 
         * \warning Заказ других сигналов пока не работает..
         * \warning функция временно недоступна (private). Ведуться работы...
         * \todo сделать возможность заказа других сигналов
        */
//        void askSignal(int signo, AskSigCommand cmd=Ask);    

        virtual void work();

        inline CORBA::ORB_ptr getORB()
        {
            return orb;
        }

        virtual void sysCommand( const UniSetTypes::SystemMessage *sm );

    private:

//        static void processingSignal(int signo);            
        static void terminated(int signo);
        static void finishterm(int signo);
        static void normalexit();
        static void normalterminate();
        static void set_signals(bool ask);
        void term( int signo );
        void init();

        friend class ThreadCreator<UniSetActivator>;
        ThreadCreator<UniSetActivator> *orbthr;
        
        CORBA::ORB_var orb;
        
        bool omDestroy;            
        bool sig;
        pid_t thpid; // pid orb потока

        struct Info
        {
            pid_t msgpid;    // pid порожденого потока обработки сообщений
        };
        
        struct OInfo:
            public Info
        {
            UniSetObject* obj;
        };

        struct MInfo:
            public Info
        {
            UniSetManager* mnr;
        };

        std::list<OInfo> lstOInfo;
        std::list<MInfo> lstMInfo;
        void getinfo();        
};

/*
template<class TClass>
int    UniSetActivator::attach(TClass* p, void(TClass:: *f)(void*) )
{
    if( next >= MAX_CHILD_THREAD )
        return -1;

    callpull[next] = new OmniThreadCreator<TClass>( p, f);                     
    next++;
    return 0;
}
*/
#endif
