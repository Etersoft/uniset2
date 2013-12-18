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
#include <cstdlib>
#include <sstream>
#include <list>
#include <string>
#include <functional>
#include <algorithm>

#include "Exceptions.h"
#include "ORepHelpers.h"
#include "UInterface.h"
#include "UniSetManager.h"
#include "Debug.h"

// ------------------------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
// ------------------------------------------------------------------------------------------
// объект-функция для посылки сообщения менеджеру
class MPush: public unary_function<UniSetManager*, bool>
{
    public:
        MPush(const UniSetTypes::TransportMessage& msg):msg(msg){}
        bool operator()(UniSetManager* m) const
        {
            try
            {
                m->push( msg );
                m->broadcast( msg );
                return true;
            }
            catch(...){}
            return false;
        }

    private:
        const UniSetTypes::TransportMessage& msg;
};

// объект-функция для посылки сообщения объекту
class OPush: public unary_function<UniSetObject*, bool>
{
    public:
        OPush(const UniSetTypes::TransportMessage& msg):msg(msg){}
        bool operator()(UniSetObject* o) const
        {
            try
            {
                o->push( msg );
                return true;
            }
            catch(...){}
            return false;
        }
    private:
        const UniSetTypes::TransportMessage& msg;
};

// ------------------------------------------------------------------------------------------
UniSetManager::UniSetManager():
UniSetObject(UniSetTypes::DefaultObjectId),
sig(0),
olistMutex("olistMutex"),
mlistMutex("mlistMutex")
{
}
// ------------------------------------------------------------------------------------------
UniSetManager::UniSetManager( ObjectId id ):
UniSetObject(id),
sig(0)
{
    olistMutex.setName(myname + "_olistMutex");
    mlistMutex.setName(myname + "_mlistMutex");
}

// ------------------------------------------------------------------------------------------

UniSetManager::UniSetManager(const string& name, const string& section):
UniSetObject(name, section),
sig(0)
{
    olistMutex.setName(myname + "_olistMutex");
    mlistMutex.setName(myname + "_mlistMutex");
}

// ------------------------------------------------------------------------------------------
UniSetManager::~UniSetManager()
{
    try
    {
        objects(deactiv);
    }
    catch(...){}
    try
    {
        managers(deactiv);
    }
    catch(...){}
    olist.clear();
    mlist.clear();
}
// ------------------------------------------------------------------------------------------
void UniSetManager::initPOA( UniSetManager* rmngr )
{
    if( CORBA::is_nil(pman) )
        this->pman = rmngr->getPOAManager();
/*
    string mname(getName());
    mname+="_poamngr";
    PortableServer::POA_var root_poa = rmngr->getPOA();
    poa = root_poa->create_POA(mname.c_str(), pman, policyList);
*/
    PortableServer::POA_var rpoa = rmngr->getPOA();
    if( rpoa != poa )
        poa = rmngr->getPOA();

    if( CORBA::is_nil(poa) )
        ulog.crit() << myname << "(initPOA): failed init poa " << endl;

    // Инициализация самого менеджера и его подобъектов
    UniSetObject::init(rmngr);
    objects(initial);
    managers(initial);
}
// ------------------------------------------------------------------------------------------
bool UniSetManager::addObject( UniSetObject *obj )
{
    {    //lock
        uniset_rwmutex_wrlock lock(olistMutex);
        ObjectsList::iterator li=find(olist.begin(),olist.end(), obj);
        if( li==olist.end() )
        {
            if( ulog.is_info() )
                ulog.info() << myname << "(activator): добавляем объект "<< obj->getName()<< endl;
             olist.push_back(obj);
        }
    } // unlock
    return true;
}

// ------------------------------------------------------------------------------------------
bool UniSetManager::removeObject(UniSetObject* obj)
{
    {    //lock
        uniset_rwmutex_wrlock lock(olistMutex);
        ObjectsList::iterator li=find(olist.begin(),olist.end(), obj);
        if( li!=olist.end() )
        {
            if( ulog.is_info() )
                ulog.info() << myname << "(activator): удаляем объект "<< obj->getName()<< endl;
            try
            {
                obj->disactivate();
            }
            catch(Exception& ex)
            {
                ulog.warn() << myname << "(removeObject): " << ex << endl;
            }
            catch(CORBA::SystemException& ex)
            {
                ulog.warn() << myname << "(removeObject): поймали CORBA::SystemException: " << ex.NP_minorString() << endl;
            }
            catch(CORBA::Exception& ex)
            {
                ulog.warn() << myname << "(removeObject): CORBA::Exception" << endl;
            }
            catch( omniORB::fatalException& fe )
            {
                ulog.crit() << myname << "(managers): Caught omniORB::fatalException:" << endl;
                ulog.crit() << myname << "(managers): file: " << fe.file()
                    << " line: " << fe.line()
                    << " mesg: " << fe.errmsg() << endl;
            }

            catch(...){}
            olist.erase(li);
        }
    } // unlock

    return true;
}

// ------------------------------------------------------------------------------------------
/*!
 *    Функция работы со списком менеджеров
*/
void UniSetManager::managers(OManagerCommand cmd)
{
    if( ulog.is_info() )
        ulog.info() << myname <<"(managers): mlist.size="
                        << mlist.size() << " cmd=" << cmd  << endl;
    {    //lock
        uniset_rwmutex_rlock lock(mlistMutex);
        for( UniSetManagerList::iterator li=mlist.begin();li!=mlist.end();++li )
        {
            try
            {
                switch(cmd)
                {
                    case initial:
                        (*li)->initPOA(this);
                        break;

                    case activ:
                        (*li)->activate();
                        break;

                    case deactiv:
                        (*li)->disactivate();
                        break;

                    case term:
                        (*li)->sigterm(sig);
                        break;

                    default:
                        break;
                }
            }
            catch( Exception& ex )
            {
                if( ulog.is_crit() )
                {
                    ulog.crit() << myname << "(managers): " << ex << endl;
                    ulog.crit() << myname << "(managers): не смог зарегистрировать (разрегистрировать) объект -->"<< (*li)->getName() << endl;
                }
            }
            catch(CORBA::SystemException& ex)
            {
                if( ulog.is_crit() )
                    ulog.crit() << myname << "(managers): поймали CORBA::SystemException:" << ex.NP_minorString() << endl;
            }
            catch( CORBA::Exception& ex )
            {
                if( ulog.is_crit() )
                    ulog.crit() << myname << "(managers): Caught CORBA::Exception. " << ex._name() << endl;
            }
            catch( omniORB::fatalException& fe )
            {
                if( ulog.is_crit() )
                {
                    ulog.crit() << myname << "(managers): Caught omniORB::fatalException:" << endl;
                    ulog.crit() << myname << "(managers): file: " << fe.file()
                        << " line: " << fe.line()
                        << " mesg: " << fe.errmsg() << endl;
                }
            }
        }
    } // unlock
}
// ------------------------------------------------------------------------------------------
/*!
 *    Функция работы со списком объектов.
*/
void UniSetManager::objects(OManagerCommand cmd)
{
    if( ulog.is_info() )
        ulog.info() << myname <<"(objects): olist.size="
                        << olist.size() << " cmd=" << cmd  << endl;
    {    //lock
        uniset_rwmutex_rlock lock(olistMutex);

        for (ObjectsList::iterator li=olist.begin();li!=olist.end();++li)
        {
            try
            {
                switch(cmd)
                {
                    case initial:
                        (*li)->init(this);
                        break;

                    case activ:
                        (*li)->activate();
                        break;

                    case deactiv:
                        (*li)->disactivate();
                        break;

                    case term:
                        (*li)->sigterm(sig);
                        break;

                    default:
                        break;
                }
            }
            catch( Exception& ex )
            {
                if( ulog.is_crit() )
                {
                    ulog.crit() << myname << "(objects): " << ex << endl;
                    ulog.crit() << myname << "(objects): не смог зарегистрировать (разрегистрировать) объект -->"<< (*li)->getName() << endl;
                }
            }
            catch(CORBA::SystemException& ex)
            {
                if( ulog.is_crit() )
                    ulog.crit() << myname << "(objects): поймали CORBA::SystemException:" << ex.NP_minorString() << endl;
            }
            catch( CORBA::Exception& ex )
            {
                if( ulog.is_crit() )
                    ulog.crit() << myname << "(objects): Caught CORBA::Exception. "
                        << ex._name()
                        << " (" << (*li)->getName() << ")" << endl;
            }
            catch( omniORB::fatalException& fe )
            {
                if( ulog.is_crit() )
                {
                    ulog.crit() << myname << "(objects): Caught omniORB::fatalException:" << endl;
                    ulog.crit() << myname << "(objects): file: " << fe.file()
                        << " line: " << fe.line()
                        << " mesg: " << fe.errmsg() << endl;
                }
            }
        }
    } // unlock
}
// ------------------------------------------------------------------------------------------
/*!
 *    Регистрирация объекта и всех его подобъектов в репозитории.
 *    \note Только после этого он (и они) становятся доступны другим процессам
*/
bool UniSetManager::activateObject()
{
    if( ulog.is_info() )
        ulog.info() << myname << "(activateObjects):  активизирую объекты"<< endl;
    UniSetObject::activateObject();
    managers(activ);
    objects(activ);
    return true;
}
// ------------------------------------------------------------------------------------------
/*!
 *    Удаление объекта и всех его подобъектов из репозитория.
 *    \note Объект становится недоступен другим процессам
*/
bool UniSetManager::disactivateObject()
{
    if( ulog.is_info() )
        ulog.info() << myname << "(disactivateObjects):  деактивизирую объекты"<< endl;
    // именно в такой последовательности!
    objects(deactiv);
    managers(deactiv);
    return true;
}
// ------------------------------------------------------------------------------------------
void UniSetManager::sigterm( int signo )
{
//    ulog.info() << "UniSetActivator: default processing signo="<< signo << endl;
    sig=signo;
    objects(term);
    managers(term);
    UniSetObject::sigterm(signo);
}
// ------------------------------------------------------------------------------------------

void UniSetManager::broadcast(const TransportMessage& msg)
{
    // себя не забыть...
//    push(msg);

    // Всем объектам...
    {    //lock
        uniset_rwmutex_rlock lock(olistMutex);
        for_each(olist.begin(),olist.end(),OPush(msg)); // STL метод
    } // unlock

    // Всем менеджерам....
    {    //lock
        uniset_rwmutex_rlock lock(mlistMutex);
        for_each(mlist.begin(),mlist.end(),MPush(msg)); // STL метод
    } // unlock
}

// ------------------------------------------------------------------------------------------
bool UniSetManager::addManager( UniSetManager *child )
{
    {    //lock
        uniset_rwmutex_wrlock lock(mlistMutex);

        // Проверка на совпадение
        UniSetManagerList::iterator it= find(mlist.begin(),mlist.end(),child);
        if(it == mlist.end() )
        {
             mlist.push_back( child );
            if( ulog.is_info() )
                ulog.info() << myname << ": добавляем менеджер "<< child->getName()<< endl;
        }
        else if( ulog.is_warn() )
            ulog.warn() << myname << ": попытка повторного добавления объекта "<< child->getName() << endl;
    } // unlock

    return true;
}

// ------------------------------------------------------------------------------------------
bool UniSetManager::removeManager( UniSetManager* child )
{
    {    //lock
        uniset_rwmutex_wrlock lock(mlistMutex);
        mlist.remove(child);
    } // unlock

    return true;
}

// ------------------------------------------------------------------------------------------

const UniSetManager* UniSetManager::itemM(const ObjectId id)
{

    {    //lock
        uniset_rwmutex_rlock lock(mlistMutex);
        for( UniSetManagerList::iterator li=mlist.begin(); li!=mlist.end();++li )
        {
            if ( (*li)->getId()==id )
                return (*li);
        }
    } // unlock

    return 0;
}

// ------------------------------------------------------------------------------------------

const UniSetObject* UniSetManager::itemO(const ObjectId id)
{
    {    //lock
        uniset_rwmutex_rlock lock(olistMutex);
        for (ObjectsList::iterator li=olist.begin(); li!=olist.end();++li)
        {
            if ( (*li)->getId()==id )
                return (*li);
        }
    } // unlock

    return 0;
}

// ------------------------------------------------------------------------------------------

int UniSetManager::objectsCount()
{
    int res( olist.size()+mlist.size() );

    for( UniSetManagerList::const_iterator it= beginMList();
            it!= endMList(); ++it )
    {
        res+= (*it)->objectsCount();
    }

    return res;
}

// ------------------------------------------------------------------------------------------
int UniSetManager::getObjectsInfo( UniSetManager* mngr, SimpleInfoSeq* seq,
                                    int begin, const long uplimit )
{
    int ind = begin;

    // получаем у самого менджера
    SimpleInfo_var msi=mngr->getInfo();
    (*seq)[ind] = msi;

    ind++;
    if( ind > uplimit )
        return ind;

    for( ObjectsList::const_iterator it= mngr->beginOList();
            it!=mngr->endOList(); ++it )
    {
        try
        {
            SimpleInfo_var si=(*it)->getInfo();
            (*seq)[ind] = si;
            ind++;
            if( ind>uplimit )
                break;
        }
        catch(CORBA::Exception& ex)
        {
            ulog.warn() << myname << "(getObjectsInfo): CORBA::Exception" << endl;
        }
        catch(...)
        {
            ulog.warn() << myname << "(getObjectsInfo): не смог получить у объекта "
                    << conf->oind->getNameById( (*it)->getId() ) << " информацию" << endl;
        }
    }

    if( ind > uplimit )
        return ind;

    // а далее у его менеджеров (рекурсивно)
    for( UniSetManagerList::const_iterator it=mngr->beginMList();
         it!=mngr->endMList(); ++it )
    {
        ind = getObjectsInfo((*it),seq,ind,uplimit);
        if( ind > uplimit )
            break;
    }

    return ind;
}
// ------------------------------------------------------------------------------------------

SimpleInfoSeq* UniSetManager::getObjectsInfo( CORBA::Long maxlength )
{
    SimpleInfoSeq* res = new SimpleInfoSeq();    // ЗА ОСВОБОЖДЕНИЕ ПАМЯТИ ОТВЕЧАЕТ КЛИЕНТ!!!!!!
                                                // поэтому ему лучше пользоваться при получении _var-классом
    int length = objectsCount()+1;
    if( length >= maxlength )
        length = maxlength;

    res->length(length);

    // используем рекурсивную функцию
    int ind = 0;
    const int limit = length;
    ind = getObjectsInfo( this, res, ind, limit );
    return res;
}

// ------------------------------------------------------------------------------------------
