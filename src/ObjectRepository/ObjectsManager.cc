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
 *  \date   $Date: 2009/02/10 20:38:27 $
 *  \version $Id: ObjectsManager.cc,v 1.22 2009/02/10 20:38:27 vpashka Exp $
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
#include "UniversalInterface.h"
#include "ObjectsManager.h"
#include "Debug.h"

// ------------------------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
// ------------------------------------------------------------------------------------------
// объект-функция для посылки сообщения менеджеру
class MPush: public unary_function<ObjectsManager*, bool>
{
	public: 
		MPush(const UniSetTypes::TransportMessage& msg):msg(msg){}
		bool operator()(ObjectsManager* m) const
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
ObjectsManager::ObjectsManager():
UniSetObject(UniSetTypes::DefaultObjectId)
{
}
// ------------------------------------------------------------------------------------------
ObjectsManager::ObjectsManager( ObjectId id):
UniSetObject(id),
sig(0)
{
}

// ------------------------------------------------------------------------------------------

ObjectsManager::ObjectsManager(const string name, const string section): 
UniSetObject(name, section),
sig(0)
{
}

// ------------------------------------------------------------------------------------------
ObjectsManager::~ObjectsManager()
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
void ObjectsManager::initPOA( ObjectsManager* rmngr )
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
		unideb[Debug::CRIT] << myname << "(initPOA): failed init poa " << endl;

	// Инициализация самого менеджера и его подобъектов
	UniSetObject::init(rmngr);
	objects(initial);	
	managers(initial);	
}
// ------------------------------------------------------------------------------------------
bool ObjectsManager::addObject( UniSetObject *obj )
{
	{	//lock
		uniset_mutex_lock lock(olistMutex, 1000);
		ObjectsList::iterator li=find(olist.begin(),olist.end(), obj);
		if( li==olist.end() )
		{
			if( unideb.debugging(Debug::INFO) )
				unideb[Debug::INFO] << myname << "(activator): добавляем объект "<< obj->getName()<< endl;
		 	olist.push_back(obj);
		}
	} // unlock	
	return true;
}

// ------------------------------------------------------------------------------------------
bool ObjectsManager::removeObject(UniSetObject* obj)
{
	{	//lock
		uniset_mutex_lock lock(olistMutex, 1000);
		ObjectsList::iterator li=find(olist.begin(),olist.end(), obj);
		if( li!=olist.end() )
		{
			if( unideb.debugging(Debug::INFO) )
				unideb[Debug::INFO] << myname << "(activator): удаляем объект "<< obj->getName()<< endl;				
			try
			{
				obj->disactivate();
			}
			catch(Exception& ex)
			{
				unideb[Debug::WARN] << myname << "(removeObject): " << ex << endl;
			}
			catch(CORBA::SystemException& ex)
		    {
				unideb[Debug::WARN] << myname << "(removeObject): поймали CORBA::SystemException: " << ex.NP_minorString() << endl;
	    	}
	    	catch(CORBA::Exception& ex)
		    {
				unideb[Debug::WARN] << myname << "(removeObject): CORBA::Exception" << endl;
    		}
			catch( omniORB::fatalException& fe ) 
			{
				unideb[Debug::CRIT] << myname << "(managers): Caught omniORB::fatalException:" << endl;
			    unideb[Debug::CRIT] << myname << "(managers): file: " << fe.file()
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
 *	Функция работы со списком менеджеров
*/
void ObjectsManager::managers(OManagerCommand cmd)
{
	unideb[Debug::INFO] << myname <<"(managers): mlist.size=" 
						<< mlist.size() << " cmd=" << cmd  << endl;
	{	//lock
		uniset_mutex_lock lock(mlistMutex, 1000);
		for( ObjectsManagerList::iterator li=mlist.begin();li!=mlist.end();++li )
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
				unideb[Debug::CRIT] << myname << "(managers): " << ex << endl;
				unideb[Debug::CRIT] << myname << "(managers): не смог зарегистрировать (разрегистрировать) объект -->"<< (*li)->getName() << endl;
			}
			catch(CORBA::SystemException& ex)
		    {
				unideb[Debug::CRIT] << myname << "(managers): поймали CORBA::SystemException:" << ex.NP_minorString() << endl;
		    }
			catch( CORBA::Exception& ex )
			{
			    unideb[Debug::CRIT] << myname << "(managers): Caught CORBA::Exception. " << ex._name() << endl;
			}
			catch( omniORB::fatalException& fe ) 
			{
				unideb[Debug::CRIT] << myname << "(managers): Caught omniORB::fatalException:" << endl;
			    unideb[Debug::CRIT] << myname << "(managers): file: " << fe.file()
					<< " line: " << fe.line()
			    	<< " mesg: " << fe.errmsg() << endl;
			}			
		}
	} // unlock 
}
// ------------------------------------------------------------------------------------------
/*! 
 *	Функция работы со списком объектов.
*/
void ObjectsManager::objects(OManagerCommand cmd)
{
	unideb[Debug::INFO] << myname <<"(objects): olist.size=" 
						<< olist.size() << " cmd=" << cmd  << endl;
	{	//lock
		uniset_mutex_lock lock(olistMutex, 1000);

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
				unideb[Debug::CRIT] << myname << "(objects): " << ex << endl;
				unideb[Debug::CRIT] << myname << "(objects): не смог зарегистрировать (разрегистрировать) объект -->"<< (*li)->getName() << endl;
			}
			catch(CORBA::SystemException& ex)
		    {
				unideb[Debug::CRIT] << myname << "(objects): поймали CORBA::SystemException:" << ex.NP_minorString() << endl;
		    }
			catch( CORBA::Exception& ex )
			{
			    unideb[Debug::CRIT] << myname << "(objects): Caught CORBA::Exception. " 
			    << ex._name()
			    << " (" << (*li)->getName() << ")" << endl;
			}
			catch( omniORB::fatalException& fe ) 
			{
				unideb[Debug::CRIT] << myname << "(objects): Caught omniORB::fatalException:" << endl;
			    unideb[Debug::CRIT] << myname << "(objects): file: " << fe.file()
					<< " line: " << fe.line()
			    	<< " mesg: " << fe.errmsg() << endl;
			}			
		}
	} // unlock
}
// ------------------------------------------------------------------------------------------
/*! 
 *	Регистрирация объекта и всех его подобъектов в репозитории.
 *	\note Только после этого он (и они) становятся доступны другим процессам
*/
bool ObjectsManager::activateObject()
{
	if( unideb.debugging(Debug::INFO) )
		unideb[Debug::INFO] << myname << "(activateObjects):  активизирую объекты"<< endl;
	UniSetObject::activateObject();
	managers(activ);
	objects(activ);
	return true;
}
// ------------------------------------------------------------------------------------------
/*! 
 *	Удаление объекта и всех его подобъектов из репозитория.
 *	\note Объект становится недоступен другим процессам
*/
bool ObjectsManager::disactivateObject()
{
	if( unideb.debugging(Debug::INFO) )
		unideb[Debug::INFO] << myname << "(disactivateObjects):  деактивизирую объекты"<< endl;
	// именно в такой последовательности!
	objects(deactiv);
	managers(deactiv);
	return true;
}
// ------------------------------------------------------------------------------------------
void ObjectsManager::sigterm( int signo )
{
//	unideb[Debug::INFO] << "ObjectsActivator: default processing signo="<< signo << endl;
	sig=signo;
	objects(term);
	managers(term);
	UniSetObject::sigterm(signo);
}
// ------------------------------------------------------------------------------------------

void ObjectsManager::broadcast(const TransportMessage& msg)
{
	// себя не забыть...
//	push(msg);
	
	// Всем объектам...
	{	//lock
		uniset_mutex_lock lock(olistMutex, 2000);
		for_each(olist.begin(),olist.end(),OPush(msg)); // STL метод
	} // unlock

	// Всем менеджерам....
	{	//lock
		uniset_mutex_lock lock(mlistMutex, 2000);
		for_each(mlist.begin(),mlist.end(),MPush(msg)); // STL метод
	} // unlock
}

// ------------------------------------------------------------------------------------------
bool ObjectsManager::addManager( ObjectsManager *child )
{
	{	//lock
		uniset_mutex_lock lock(mlistMutex, 1000);

		// Проверка на совпадение
		ObjectsManagerList::iterator it= find(mlist.begin(),mlist.end(),child);
		if(it == mlist.end() )
		{
		 	mlist.push_back( child );
			if( unideb.debugging(Debug::INFO) )
				unideb[Debug::INFO] << myname << ": добавляем менеджер "<< child->getName()<< endl;
		}
		else
			unideb[Debug::WARN] << myname << ": попытка повторного добавления объекта "<< child->getName() << endl;
	} // unlock	
	
	return true;
}

// ------------------------------------------------------------------------------------------
bool ObjectsManager::removeManager( ObjectsManager* child )
{
	{	//lock
		uniset_mutex_lock lock(mlistMutex, 1000);
		mlist.remove(child);
	} // unlock	
	
	return true;
}

// ------------------------------------------------------------------------------------------

const ObjectsManager* ObjectsManager::itemM(const ObjectId id)
{
	
	{	//lock
		uniset_mutex_lock lock(mlistMutex, 1000);
		for( ObjectsManagerList::iterator li=mlist.begin(); li!=mlist.end();++li )
		{
			if ( (*li)->getId()==id )
				return (*li);	
		}
	} // unlock	

	return 0;
}

// ------------------------------------------------------------------------------------------

const UniSetObject* ObjectsManager::itemO(const ObjectId id)
{
	{	//lock
		uniset_mutex_lock lock(olistMutex, 1000);
		for (ObjectsList::iterator li=olist.begin(); li!=olist.end();++li)
		{
			if ( (*li)->getId()==id )
				return (*li);	
		}
	} // unlock	

	return 0;
}

// ------------------------------------------------------------------------------------------

int ObjectsManager::objectsCount()
{
	int res( olist.size()+mlist.size() );
	
	for( ObjectsManagerList::const_iterator it= beginMList();
			it!= endMList(); ++it )
	{
		res+= (*it)->objectsCount();
	}	
	
	return res;
}

// ------------------------------------------------------------------------------------------
int ObjectsManager::getObjectsInfo( ObjectsManager* mngr, SimpleInfoSeq* seq, 
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
			unideb[Debug::WARN] << myname << "(getObjectsInfo): CORBA::Exception" << endl;
	    }
		catch(...)
		{
			unideb[Debug::WARN] << myname << "(getObjectsInfo): не смог получить у объекта "
					<< conf->oind->getNameById( (*it)->getId() ) << " информацию" << endl;
		}	
	}

	if( ind > uplimit )
		return ind;

	// а далее у его менеджеров (рекурсивно)
	for( ObjectsManagerList::const_iterator it=mngr->beginMList();
		 it!=mngr->endMList(); ++it )
	{
		ind = getObjectsInfo((*it),seq,ind,uplimit);
		if( ind > uplimit )
			break;
	}

	return ind;
}
// ------------------------------------------------------------------------------------------

SimpleInfoSeq* ObjectsManager::getObjectsInfo( CORBA::Long maxlength )
{
	SimpleInfoSeq* res = new SimpleInfoSeq();	// ЗА ОСВОБОЖДЕНИЕ ПАМЯТИ ОТВЕЧАЕТ КЛИЕНТ!!!!!!
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
