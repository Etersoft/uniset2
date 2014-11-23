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

#if 0
# НЕ МЫ СОЗДАВАЛИ.. НЕ НАМ И УНИЧТОЖАТЬ!
# нужно перейти на shared_ptr<>..
    for( auto& i: olist )
    {
        try
        {
            delete i;
        }
        catch(...){}
    }

    for( auto& i: mlist )
    {
        try
        {
            delete i;
        }
        catch(...){}
    }
#endif
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
        ucrit << myname << "(initPOA): failed init poa " << endl;

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
        auto li=find(olist.begin(),olist.end(), obj);
        if( li==olist.end() )
        {
            uinfo << myname << "(activator): добавляем объект "<< obj->getName()<< endl;
            olist.push_back(obj);
        }
    } // unlock    
    return true;
}

// ------------------------------------------------------------------------------------------
bool UniSetManager::removeObject( UniSetObject* obj )
{
    {    //lock
        uniset_rwmutex_wrlock lock(olistMutex);
        auto li=find(olist.begin(),olist.end(), obj);
        if( li!=olist.end() )
        {
            uinfo << myname << "(activator): удаляем объект "<< obj->getName()<< endl;
            try
            {
                obj->deactivate();
            }
            catch(Exception& ex)
            {
                uwarn << myname << "(removeObject): " << ex << endl;
            }
            catch(CORBA::SystemException& ex)
            {
                uwarn << myname << "(removeObject): поймали CORBA::SystemException: " << ex.NP_minorString() << endl;
            }
            catch(CORBA::Exception& ex)
            {
                uwarn << myname << "(removeObject): CORBA::Exception" << endl;
            }
            catch( omniORB::fatalException& fe ) 
            {
                ucrit << myname << "(managers): Caught omniORB::fatalException:" << endl;
                ucrit << myname << "(managers): file: " << fe.file()
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
void UniSetManager::managers( OManagerCommand cmd )
{
    uinfo << myname <<"(managers): mlist.size=" << mlist.size() << " cmd=" << cmd  << endl;
    {    //lock
        uniset_rwmutex_rlock lock(mlistMutex);
        for( auto &li: mlist )
        {
            try
            {
                switch(cmd)
                {
                    case initial:
                        li->initPOA(this);
                        break;

                    case activ:
                        li->activate();
                        break;

                    case deactiv:
                        li->deactivate();
                        break;

                    case term:
                        li->sigterm(sig);
                        break;

                    default:
                        break;
                }
            }
            catch( Exception& ex )
            {
                 ucrit << myname << "(managers): " << ex << endl
                       << " Не смог зарегистрировать (разрегистрировать) объект -->"<< li->getName() << endl;
            }
            catch( CORBA::SystemException& ex )
            {
                ucrit << myname << "(managers): поймали CORBA::SystemException:" << ex.NP_minorString() << endl;
            }
            catch( CORBA::Exception& ex )
            {
                ucrit << myname << "(managers): Caught CORBA::Exception. " << ex._name() << endl;
            }
            catch( omniORB::fatalException& fe )
            {
                ucrit << myname << "(managers): Caught omniORB::fatalException:" << endl;
                ucrit << myname << "(managers): file: " << fe.file()
                      << " line: " << fe.line()
                      << " mesg: " << fe.errmsg() << endl;
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
    uinfo << myname <<"(objects): olist.size=" 
                        << olist.size() << " cmd=" << cmd  << endl;
    {    //lock
        uniset_rwmutex_rlock lock(olistMutex);

        for( auto &li: olist )
        {
            try
            {
                switch(cmd)
                {
                    case initial:
                        li->init(this);
                        break;

                    case activ:
                        li->activate();
                        break;

                    case deactiv:
                        li->deactivate();
                        break;

                    case term:
                        li->sigterm(sig);
                        break;

                    default:
                        break;
                }
            }
            catch( Exception& ex )
            {
                ucrit << myname << "(objects): " << ex << endl;
                ucrit << myname << "(objects): не смог зарегистрировать (разрегистрировать) объект -->"<< li->getName() << endl;
            }
            catch(CORBA::SystemException& ex)
            {
                ucrit << myname << "(objects): поймали CORBA::SystemException:" << ex.NP_minorString() << endl;
            }
            catch( CORBA::Exception& ex )
            {
                ucrit << myname << "(objects): Caught CORBA::Exception. " 
                      << ex._name()
                      << " (" << li->getName() << ")" << endl;
            }
            catch( omniORB::fatalException& fe ) 
            {
               ucrit << myname << "(objects): Caught omniORB::fatalException:" << endl;
               ucrit << myname << "(objects): file: " << fe.file()
                     << " line: " << fe.line()
                     << " mesg: " << fe.errmsg() << endl;
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
    uinfo << myname << "(activateObjects):  активизирую объекты"<< endl;
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
bool UniSetManager::deactivateObject()
{
    uinfo << myname << "(deactivateObjects):  деактивизирую объекты"<< endl;
    // именно в такой последовательности!
    objects(deactiv);
    managers(deactiv);
    return true;
}
// ------------------------------------------------------------------------------------------
void UniSetManager::sigterm( int signo )
{
    sig=signo;
    try
    {
        objects(term);
    }
    catch(...){}
    
    try
    {
        managers(term);
    }
    catch(...){}

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
        auto it= find(mlist.begin(),mlist.end(),child);
        if(it == mlist.end() )
        {
             mlist.push_back( child );
             uinfo << myname << ": добавляем менеджер "<< child->getName()<< endl;
        }
        else 
            uwarn << myname << ": попытка повторного добавления объекта "<< child->getName() << endl;
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
        for( auto &li: mlist )
        {
            if ( li->getId()==id )
                return li;
        }
    } // unlock

    return 0;
}

// ------------------------------------------------------------------------------------------

const UniSetObject* UniSetManager::itemO(const ObjectId id)
{
    {    //lock
        uniset_rwmutex_rlock lock(olistMutex);
        for( auto &li: olist )
        {
            if ( li->getId()==id )
                return li;
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
    unsigned int ind = begin;

    // получаем у самого менджера
    SimpleInfo_var msi=mngr->getInfo();
    (*seq)[ind] = msi;

    ind++;
    if( ind > uplimit )
        return ind;

    for( auto it=mngr->beginOList(); it!=mngr->endOList(); ++it )
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
            uwarn << myname << "(getObjectsInfo): CORBA::Exception" << endl;
        }
        catch(...)
        {
            uwarn << myname << "(getObjectsInfo): не смог получить у объекта "
                    << uniset_conf()->oind->getNameById( (*it)->getId() ) << " информацию" << endl;
        }
    }

    if( ind > uplimit )
        return ind;

    // а далее у его менеджеров (рекурсивно)
    for( auto it=mngr->beginMList(); it!=mngr->endMList(); ++it )
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
std::ostream& operator<<(std::ostream& os, UniSetManager::OManagerCommand& cmd )
{
    // { deactiv, activ, initial, term };
    if( cmd == UniSetManager::deactiv )
        return os << "deactivate";
    if( cmd == UniSetManager::activ )
        return os << "activate";
    if( cmd == UniSetManager::initial )
        return os << "init";
    if( cmd == UniSetManager::term )
        return os << "terminate";

    return os << "unkwnown";
}
// ------------------------------------------------------------------------------------------