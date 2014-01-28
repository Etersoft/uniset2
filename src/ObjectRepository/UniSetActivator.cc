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
 *  \author Pavel Vainerman
*/
// -------------------------------------------------------------------------- 

#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <sstream>

#include "Exceptions.h"
#include "ORepHelpers.h"
#include "UInterface.h"
#include "UniSetActivator.h"
#include "Debug.h"
#include "Configuration.h"
#include "Mutex.h"

// ------------------------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
// ------------------------------------------------------------------------------------------
/*
     Завершение работы организовано следующим образом.
    Имеется глобальный указатель gActivator (т.к. активатор в системе должен быть только один).
    Он заказывает на себя все сигналы связанные с завершением работы.
    
    В качестве обработчика сигналов регистрируется UniSetActivator::terminated( int signo ).
    В этом обработчике происходит вызов UniSetActivator::oaDestroy(int signo) для фактического
    завершения работы и заказывается сигнал SIG_ALRM на время TERMINATE_TIMEOUT,
    c обработчиком UniSetActivator::finishterm в котором происходит
    "надежное" прибивание текущего процесса (raise(SIGKILL)). Это сделано на тот случай, если
    в oaDestroy произойдет зависание.
*/
// ------------------------------------------------------------------------------------------
/*! замок для блокирования совместного доступа к функции обрабтки сигналов */
static UniSetTypes::uniset_rwmutex signalMutex("Activator::signalMutex");
// static UniSetTypes::uniset_mutex waittermMutex("Activator::waittermMutex");

/*! замок для блокирования совместного к списку получателей сигналов */
//UniSetTypes::uniset_mutex sigListMutex("Activator::sigListMutex");

//static omni_mutex pmutex;
//static omni_condition pcondx(&pmutex);

// ------------------------------------------------------------------------------------------
static UniSetActivator* gActivator=0;
//static omni_mutex termutex;
//static omni_condition termcond(&termutex);
//static ThreadCreator<UniSetActivator>* termthread=0;
static int SIGNO;
static int MYPID;
static const int TERMINATE_TIMEOUT = 2; //  время отведенное на завершение процесса [сек]
ost::AtomicCounter procterm = 0;
ost::AtomicCounter doneterm = 0;

// PassiveTimer termtmr;
// ------------------------------------------------------------------------------------------
UniSetActivator::UniSetActivator( ObjectId id ):
UniSetManager(id),
orbthr(0),
omDestroy(false),
sig(false)
{
    UniSetActivator::init();
}
// ------------------------------------------------------------------------------------------
UniSetActivator::UniSetActivator():
UniSetManager(UniSetTypes::DefaultObjectId),
orbthr(0),
omDestroy(false),
sig(false)
{
//    thread(false);    //    отключаем поток (раз не задан id)
    UniSetActivator::init();
}

// ------------------------------------------------------------------------------------------
void UniSetActivator::init()
{
    orb = conf->getORB();
    CORBA::Object_var obj = orb->resolve_initial_references("RootPOA");
    PortableServer::POA_var root_poa = PortableServer::POA::_narrow(obj);
    pman = root_poa->the_POAManager();
    CORBA::PolicyList pl = conf->getPolicy();
    poa = root_poa->create_POA("my poa", pman, pl);

    if( CORBA::is_nil(poa) )
        ucrit << myname << "(init): init poa failed!!!" << endl;

    gActivator=this;
    atexit( UniSetActivator::normalexit );
    set_terminate( UniSetActivator::normalterminate ); // ловушка для неизвестных исключений
}

// ------------------------------------------------------------------------------------------

UniSetActivator::~UniSetActivator()
{
    if(!procterm )
    {
        ulogsys << myname << "(destructor): ..."<< endl << flush;
        if( !omDestroy )
            oaDestroy();

        procterm = 1;
        doneterm = 1;
        set_signals(false);    
        gActivator=0;    
    }

    if( orbthr )
        delete orbthr;
}
// ------------------------------------------------------------------------------------------

void UniSetActivator::oaDestroy(int signo)
{
//        waittermMutex.lock();
        if( !omDestroy )
        {
            omDestroy = true;
            ulogsys << myname << "(oaDestroy): begin..."<< endl;

            ulogsys << myname << "(oaDestroy): terminate... " << endl;
            term(signo);
            ulogsys << myname << "(oaDestroy): terminate ok. " << endl;

            try
            {    
                stop();
            }
            catch(...){}

            ulogsys << myname << "(oaDestroy): pman deactivate... " << endl;
            pman->deactivate(false,true);
            ulogsys << myname << "(oaDestroy): pman deactivate ok. " << endl;

            ulogsys << myname << "(oaDestroy): orb destroy... " << endl;
            try
            {
                orb->destroy();
            }
            catch(...){}

            ulogsys << myname << "(oaDestroy): orb destroy ok."<< endl;

            if( orbthr )
            {
                delete orbthr;
                orbthr = 0;
            }

        }
//        waittermMutex.unlock();
}

// ------------------------------------------------------------------------------------------
/*! 
 *    Если thread=true то функция создает отдельный поток для обработки приходящих сообщений.
 *     И передает все ресурсы этого потока orb. А также регистрирует процесс в репозитории.
 *    \note Только после этого объект становится доступен другим процессам
 *    А далее выходит...
 *    Иначе все ресурсы основного потока передаются для обработки приходящих сообщений (и она не выходит)
 *
*/
void UniSetActivator::run(bool thread)
{
    ulogsys << myname << "(run): создаю менеджер "<< endl;

    UniSetManager::initPOA(this);

    if( getId() == UniSetTypes::DefaultObjectId )
        offThread(); // отключение потока обработки сообщений, раз не задан ObjectId

    UniSetManager::activate(); // а там вызывается активация всех подчиненных объектов и менеджеров

    getinfo();        // заполнение информации об объектах
    active=true;

    ulogsys << myname << "(run): активизируем менеджер"<<endl;
    pman->activate();
    msleep(50);

    set_signals(true);    
    if( thread )
    {
        uinfo << myname << "(run): запускаемся с созданием отдельного потока...  "<< endl;
        orbthr = new ThreadCreator<UniSetActivator>(this, &UniSetActivator::work);
        int ret = orbthr->start();
        if( ret !=0 )
        {
            ucrit << myname << "(run):  НЕ СМОГЛИ СОЗДАТЬ ORB-поток"<<endl;    
            throw SystemError("(UniSetActivator::run): CREATE ORB THREAD FAILED");
        }
    }
    else
    {
        uinfo << myname << "(run): запускаемся без создания отдельного потока...  "<< endl;
        work();
    }
}
// ------------------------------------------------------------------------------------------
/*! 
 *    Функция останавливает работу orb и завершает поток. А так же удаляет ссылку из репозитория.
 *    \note Объект становится недоступен другим процессам
*/
void UniSetActivator::stop()
{
//    uniset_mutex_lock l(disactivateMutex, 500);
    if( active )
    {
        active=false;

        ulogsys << myname << "(stop): disactivate...  "<< endl;

        disactivate();

        ulogsys << myname << "(stop): disactivate ok.  "<<endl;
        ulogsys << myname << "(stop): discard request..."<< endl;

        pman->discard_requests(true);

        ulogsys << myname << "(stop): discard request ok."<< endl;

/*
        try
        {
            ulogsys << myname << "(stop):: shutdown orb...  "<<endl;
            orb->shutdown(false);
        }
        catch(...){}

        ulogsys << myname << "(stop): shutdown ok."<< endl;
*/
    }
}

// ------------------------------------------------------------------------------------------

void UniSetActivator::work()
{
    ulogsys << myname << "(work): запускаем orb на обработку запросов..."<< endl;
    try
    {
        if(orbthr)
            thpid = orbthr->getTID();
        else
            thpid = getpid();

        orb->run();
    }
    catch(CORBA::SystemException& ex)
    {
        ucrit << myname << "(work): поймали CORBA::SystemException: " << ex.NP_minorString() << endl;
    }
    catch(CORBA::Exception& ex)
    {
        ucrit << myname << "(work): поймали CORBA::Exception." << endl;
    }
    catch(omniORB::fatalException& fe)
    {
        ucrit << myname << "(work): : поймали omniORB::fatalException:" << endl;
        ucrit << myname << "(work):   file: " << fe.file() << endl;
        ucrit << myname << "(work):   line: " << fe.line() << endl;
        ucrit << myname << "(work):   mesg: " << fe.errmsg() << endl;
    }
    catch(...)
    {
        ucrit << myname << "(work): catch ..." << endl;
    }

    ulogsys << myname << "(work): orb стоп!!!"<< endl;

/*
    ulogsys << myname << "(oaDestroy): orb destroy... " << endl;
    try
    {
        orb->destroy();
    }
    catch(...){}
    ulogsys << myname << "(oaDestroy): orb destroy ok."<< endl;
*/
}
// ------------------------------------------------------------------------------------------
void UniSetActivator::getinfo()
{
    for( UniSetManagerList::const_iterator it= beginMList();
            it!= endMList(); ++it )
    {
        MInfo mi;
        mi.mnr = (*it);
        mi.msgpid = (*it)->getMsgPID();
        lstMInfo.push_back(mi);
    }

    for( ObjectsList::const_iterator it= beginOList();
            it!= endOList(); ++it )
    {
        OInfo oi;
        oi.obj = (*it);
        oi.msgpid = (*it)->getMsgPID();
        lstOInfo.push_back(oi);
    }
}
// ------------------------------------------------------------------------------------------
void UniSetActivator::sysCommand( const UniSetTypes::SystemMessage *sm )
{
    switch(sm->command)
    {
        case SystemMessage::LogRotate:
        {
            ulogsys << myname << "(sysCommand): logRotate" << endl;
            // переоткрываем логи
            string fname = ulog.getLogFile();
            if( !fname.empty() )
            {
                ulog.logFile(fname.c_str());
                ulog << myname << "(sysCommand): ***************** ulog LOG ROTATE *****************" << endl;
            }
        }
        break;
    }
}
// -------------------------------------------------------------------------

/*
void UniSetActivator::sig_child(int signo)
{
    ulogsys << gActivator->getName() << "(sig_child): дочерний процесс закончил работу...(sig=" << signo << ")" << endl;
    while( waitpid(-1, 0, WNOHANG) > 0);
}
*/
// ------------------------------------------------------------------------------------------
void UniSetActivator::set_signals(bool ask)
{

    struct sigaction act, oact;
    sigemptyset(&act.sa_mask);
    sigemptyset(&oact.sa_mask);

    // добавляем сигналы, которые будут игнорироваться
    // при обработке сигнала 
    sigaddset(&act.sa_mask, SIGINT);
    sigaddset(&act.sa_mask, SIGTERM);
    sigaddset(&act.sa_mask, SIGABRT );
    sigaddset(&act.sa_mask, SIGQUIT);
//    sigaddset(&act.sa_mask, SIGSEGV);


//    sigaddset(&act.sa_mask, SIGALRM);
//    act.sa_flags = 0;
//    act.sa_flags |= SA_RESTART;
//    act.sa_flags |= SA_RESETHAND;
    if(ask)
        act.sa_handler = terminated;
    else
        act.sa_handler = SIG_DFL;
        
    sigaction(SIGINT, &act, &oact);
    sigaction(SIGTERM, &act, &oact);
    sigaction(SIGABRT, &act, &oact);
    sigaction(SIGQUIT, &act, &oact);

//    sigaction(SIGSEGV, &act, &oact);
}

// ------------------------------------------------------------------------------------------
void UniSetActivator::finishterm( int signo )
{
    if( !doneterm )
    {
        ulogsys << ( gActivator ? gActivator->getName() : "" )
                << "(finishterm): прерываем процесс завершения...!" << endl<< flush;

        if( gActivator )
            gActivator->set_signals(false);

        sigset(SIGALRM, SIG_DFL);
        doneterm = 1;
        raise(SIGKILL);
    }
}
// ------------------------------------------------------------------------------------------
void UniSetActivator::terminated( int signo )
{
    if( !signo || doneterm || !gActivator || procterm )
        return;

    {    // lock

        // на случай прихода нескольких сигналов
        uniset_rwmutex_wrlock l(signalMutex); //, TERMINATE_TIMEOUT*1000);
        if( !procterm )
        {
            procterm = 1;
            SIGNO = signo;
            MYPID = getpid();
            ulogsys << ( gActivator ? gActivator->getName() : "" ) << "(terminated): catch SIGNO="<< signo << "("
                    << strsignal(signo) <<")"<< endl << flush
                    << ( gActivator ? gActivator->getName() : "" ) << "(terminated): устанавливаем timer завершения на "
                    << TERMINATE_TIMEOUT << " сек " << endl << flush;

            sighold(SIGALRM);
            sigset(SIGALRM, UniSetActivator::finishterm);
            alarm(TERMINATE_TIMEOUT);
            sigrelse(SIGALRM);
            if( gActivator )
                gActivator->oaDestroy(SIGNO); // gActivator->term(SIGNO);

            doneterm = 1;

            ulogsys << ( gActivator ? gActivator->getName() : "" ) << "(terminated): завершаемся..."<< endl<< flush;

            if( gActivator )
                UniSetActivator::set_signals(false);

            sigset(SIGALRM, SIG_DFL);
            raise(SIGNO);
        }
    }
}
// ------------------------------------------------------------------------------------------

void UniSetActivator::normalexit()
{
    if( gActivator )
        ulogsys << gActivator->getName() << "(default exit): good bye."<< endl << flush;
}

void UniSetActivator::normalterminate()
{
    if( gActivator )
        ucrit << gActivator->getName() << "(default exception terminate): Никто не выловил исключение!!! Good bye."<< endl<< flush;
//    abort();
}
// ------------------------------------------------------------------------------------------
void UniSetActivator::term( int signo )
{
    ulogsys << myname << "(term): TERM" << endl;

    if( doneterm )
        return;

    if( signo )
        sig = true;

    try
    {
        ulogsys << myname << "(term): вызываем sigterm()" << endl;
        sigterm(signo);

        ulogsys << myname << "(term): sigterm() ok." << endl;
    }
    catch(Exception& ex)
    {
        ucrit << myname << "(term): " << ex << endl;
    }
    catch(...){}

    ulogsys << myname << "(term): END TERM" << endl;
}
// ------------------------------------------------------------------------------------------
void UniSetActivator::waitDestroy()
{
    while( !doneterm && gActivator )
        msleep(50);

    gActivator = 0;
}
// ------------------------------------------------------------------------------------------
