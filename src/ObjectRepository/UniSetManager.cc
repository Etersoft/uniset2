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
class MPush: public unary_function< const std::shared_ptr<UniSetManager>& , bool>
{
	public:
		explicit MPush(const UniSetTypes::TransportMessage& msg): msg(msg) {}
		bool operator()( const std::shared_ptr<UniSetManager>& m ) const
		{
			try
			{
				if( m )
				{
					m->push( msg );
					m->broadcast( msg );
				}

				return true;
			}
			catch(...) {}

			return false;
		}

	private:
		const UniSetTypes::TransportMessage& msg;
};

// объект-функция для посылки сообщения объекту
class OPush: public unary_function< const std::shared_ptr<UniSetObject>& , bool>
{
	public:
		explicit OPush(const UniSetTypes::TransportMessage& msg): msg(msg) {}
		bool operator()( const std::shared_ptr<UniSetObject>& o ) const
		{
			try
			{
				if( o )
				{
					o->push( msg );
					return true;
				}
			}
			catch(...) {}

			return false;
		}
	private:
		const UniSetTypes::TransportMessage& msg;
};

// ------------------------------------------------------------------------------------------
UniSetManager::UniSetManager():
	UniSetObject(UniSetTypes::DefaultObjectId),
	sig(0),
	olistMutex("UniSetManager_olistMutex"),
	mlistMutex("UniSetManager_mlistMutex")
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
	olist.clear();
	mlist.clear();
}
// ------------------------------------------------------------------------------------------
std::shared_ptr<UniSetManager> UniSetManager::get_mptr()
{
	return std::dynamic_pointer_cast<UniSetManager>(get_ptr());
}
// ------------------------------------------------------------------------------------------
void UniSetManager::initPOA( const std::weak_ptr<UniSetManager>& rmngr )
{
	auto m = rmngr.lock();

	if( !m )
	{
		ostringstream err;
		err << myname << "(initPOA): failed weak_ptr !!";
		ucrit << err.str() << endl;
		throw SystemError(err.str());
	}

	if( CORBA::is_nil(pman) )
		this->pman = m->getPOAManager();

	PortableServer::POA_var rpoa = m->getPOA();

	if( rpoa != poa )
		poa = m->getPOA();

	if( CORBA::is_nil(poa) )
		ucrit << myname << "(initPOA): failed init poa " << endl;

	// Инициализация самого менеджера и его подобъектов
	UniSetObject::init(rmngr);
	objects(initial);
	managers(initial);
}
// ------------------------------------------------------------------------------------------
bool UniSetManager::add( const std::shared_ptr<UniSetObject>& obj )
{
	auto m = std::dynamic_pointer_cast<UniSetManager>(obj);

	if( m )
		return addManager(m);

	return addObject(obj);
}
// ------------------------------------------------------------------------------------------
bool UniSetManager::remove( const std::shared_ptr<UniSetObject>& obj )
{
	auto m = std::dynamic_pointer_cast<UniSetManager>(obj);

	if( m )
		return removeManager(m);

	return removeObject(obj);
}
// ------------------------------------------------------------------------------------------
bool UniSetManager::addObject( const std::shared_ptr<UniSetObject>& obj )
{

	{
		//lock
		uniset_rwmutex_wrlock lock(olistMutex);
		auto li = find(olist.begin(), olist.end(), obj);

		if( li == olist.end() )
		{
			uinfo << myname << "(activator): добавляем объект " << obj->getName() << endl;
			olist.push_back(obj);
		}
	} // unlock
	return true;
}

// ------------------------------------------------------------------------------------------
bool UniSetManager::removeObject( const std::shared_ptr<UniSetObject>& obj )
{
	{
		//lock
		uniset_rwmutex_wrlock lock(olistMutex);
		auto li = find(olist.begin(), olist.end(), obj);

		if( li != olist.end() )
		{
			uinfo << myname << "(activator): удаляем объект " << obj->getName() << endl;

			try
			{
				if(obj)
					obj->deactivate();
			}
			catch( const Exception& ex )
			{
				uwarn << myname << "(removeObject): " << ex << endl;
			}
			catch( const CORBA::SystemException& ex )
			{
				uwarn << myname << "(removeObject): поймали CORBA::SystemException: " << ex.NP_minorString() << endl;
			}
			catch( const CORBA::Exception& ex )
			{
				uwarn << myname << "(removeObject): CORBA::Exception" << endl;
			}
			catch( const omniORB::fatalException& fe )
			{
				ucrit << myname << "(managers): Caught omniORB::fatalException:" << endl;
				ucrit << myname << "(managers): file: " << fe.file()
					  << " line: " << fe.line()
					  << " mesg: " << fe.errmsg() << endl;
			}

			catch(...) {}

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
	uinfo << myname << "(managers): mlist.size=" << mlist.size() << " cmd=" << cmd  << endl;
	{
		//lock
		uniset_rwmutex_rlock lock(mlistMutex);

		for( auto& li : mlist )
		{
			if( !li )
				continue;

			try
			{
				switch(cmd)
				{
					case initial:
						li->initPOA( get_mptr() );
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
			catch( const Exception& ex )
			{
				ucrit << myname << "(managers): " << ex << endl
					  << " Не смог зарегистрировать (разрегистрировать) объект -->" << li->getName() << endl;
			}
			catch( const CORBA::SystemException& ex )
			{
				ucrit << myname << "(managers): поймали CORBA::SystemException:" << ex.NP_minorString() << endl;
			}
			catch( const CORBA::Exception& ex )
			{
				ucrit << myname << "(managers): Caught CORBA::Exception. " << ex._name() << endl;
			}
			catch( const omniORB::fatalException& fe )
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
	uinfo << myname << "(objects): olist.size="
		  << olist.size() << " cmd=" << cmd  << endl;
	{
		//lock
		uniset_rwmutex_rlock lock(olistMutex);

		for( auto& li : olist )
		{
			if( !li )
				continue;

			try
			{
				switch(cmd)
				{
					case initial:
						li->init(get_mptr());
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
			catch( const Exception& ex )
			{
				ucrit << myname << "(objects): " << ex << endl;
				ucrit << myname << "(objects): не смог зарегистрировать (разрегистрировать) объект -->" << li->getName() << endl;
				if( cmd == activ )
					std::terminate();
			}
			catch( const CORBA::SystemException& ex )
			{
				ucrit << myname << "(objects): поймали CORBA::SystemException:" << ex.NP_minorString() << endl;
			}
			catch( const CORBA::Exception& ex )
			{
				ucrit << myname << "(objects): Caught CORBA::Exception. "
					  << ex._name()
					  << " (" << li->getName() << ")" << endl;
			}
			catch( const omniORB::fatalException& fe )
			{
				ucrit << myname << "(objects): Caught omniORB::fatalException:" << endl;
				ucrit << myname << "(objects): file: " << fe.file()
					  << " line: " << fe.line()
					  << " mesg: " << fe.errmsg() << endl;

				if( cmd == activ )
					std::terminate();
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
	uinfo << myname << "(activateObjects):  активизирую объекты" << endl;
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
	uinfo << myname << "(deactivateObjects):  деактивизирую объекты" << endl;
	// именно в такой последовательности!
	objects(deactiv);
	managers(deactiv);
	return true;
}
// ------------------------------------------------------------------------------------------
void UniSetManager::sigterm( int signo )
{
	sig = signo;

	try
	{
		objects(term);
	}
	catch(...) {}

	try
	{
		managers(term);
	}
	catch(...) {}

	UniSetObject::sigterm(signo);
}
// ------------------------------------------------------------------------------------------

void UniSetManager::broadcast(const TransportMessage& msg)
{
	// себя не забыть...
	//    push(msg);

	// Всем объектам...
	{
		//lock
		uniset_rwmutex_rlock lock(olistMutex);
		for_each(olist.begin(), olist.end(), OPush(msg)); // STL метод
	} // unlock

	// Всем менеджерам....
	{
		//lock
		uniset_rwmutex_rlock lock(mlistMutex);
		for_each(mlist.begin(), mlist.end(), MPush(msg)); // STL метод
	} // unlock
}

// ------------------------------------------------------------------------------------------
bool UniSetManager::addManager( const std::shared_ptr<UniSetManager>& child )
{
	{
		//lock
		uniset_rwmutex_wrlock lock(mlistMutex);

		// Проверка на совпадение
		auto it = find(mlist.begin(), mlist.end(), child);

		if(it == mlist.end() )
		{
			mlist.push_back( child );
			uinfo << myname << ": добавляем менеджер " << child->getName() << endl;
		}
		else
			uwarn << myname << ": попытка повторного добавления объекта " << child->getName() << endl;
	} // unlock

	return true;
}

// ------------------------------------------------------------------------------------------
bool UniSetManager::removeManager( const std::shared_ptr<UniSetManager>& child )
{
	{
		//lock
		uniset_rwmutex_wrlock lock(mlistMutex);
		mlist.remove(child);
	} // unlock

	return true;
}

// ------------------------------------------------------------------------------------------

const std::shared_ptr<UniSetManager> UniSetManager::itemM( const ObjectId id )
{

	{
		//lock
		uniset_rwmutex_rlock lock(mlistMutex);

		for( auto& li : mlist )
		{
			if ( li->getId() == id )
				return li;
		}
	} // unlock

	return nullptr; // std::shared_ptr<UniSetManager>();
}

// ------------------------------------------------------------------------------------------

const std::shared_ptr<UniSetObject> UniSetManager::itemO( const ObjectId id )
{
	{
		//lock
		uniset_rwmutex_rlock lock(olistMutex);

		for( auto& li : olist )
		{
			if ( li->getId() == id )
				return li;
		}
	} // unlock

	return nullptr; // std::shared_ptr<UniSetObject>();
}

// ------------------------------------------------------------------------------------------

int UniSetManager::getObjectsInfo( const std::shared_ptr<UniSetManager>& mngr, SimpleInfoSeq* seq,
								   int begin, const long uplimit, CORBA::Long userparam )
{
	auto ind = begin;

	// получаем у самого менджера
	SimpleInfo_var msi = mngr->getInfo(userparam);
	(*seq)[ind] = msi;

	ind++;

	if( ind > uplimit )
		return ind;

	for( auto it = mngr->beginOList(); it != mngr->endOList(); ++it )
	{
		try
		{
			SimpleInfo_var si = (*it)->getInfo(userparam);
			(*seq)[ind] = si;
			ind++;

			if( ind > uplimit )
				break;
		}
		catch( const CORBA::Exception& ex )
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
	for( auto& i : mlist )
	{
		ind = getObjectsInfo(i, seq, ind, uplimit, userparam );

		if( ind > uplimit )
			break;
	}

	return ind;
}
// ------------------------------------------------------------------------------------------

SimpleInfoSeq* UniSetManager::getObjectsInfo(CORBA::Long maxlength , CORBA::Long userparam )
{
	SimpleInfoSeq* res = new SimpleInfoSeq();    // ЗА ОСВОБОЖДЕНИЕ ПАМЯТИ ОТВЕЧАЕТ КЛИЕНТ!!!!!!
	// поэтому ему лучше пользоваться при получении _var-классом
	int length = objectsCount() + 1;

	if( length >= maxlength )
		length = maxlength;

	res->length(length);

	// используем рекурсивную функцию
	int ind = 0;
	const int limit = length;

	ind = getObjectsInfo( get_mptr(), res, ind, limit, userparam );
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
UniSetManagerList::const_iterator UniSetManager::beginMList()
{
	return mlist.begin();
}
// ------------------------------------------------------------------------------------------
UniSetManagerList::const_iterator UniSetManager::endMList()
{
	return mlist.end();
}
// ------------------------------------------------------------------------------------------
ObjectsList::const_iterator UniSetManager::beginOList()
{
	return olist.begin();
}
// ------------------------------------------------------------------------------------------
ObjectsList::const_iterator UniSetManager::endOList()
{
	return olist.end();
}
// ------------------------------------------------------------------------------------------
size_t UniSetManager::objectsCount() const
{
	size_t res = olist.size() + mlist.size();

	for( const auto& i : mlist )
		res += i->objectsCount();

	return res;
}
// ------------------------------------------------------------------------------------------
PortableServer::POA_ptr UniSetManager::getPOA()
{
	return PortableServer::POA::_duplicate(poa);
}
// ------------------------------------------------------------------------------------------
PortableServer::POAManager_ptr UniSetManager::getPOAManager()
{
	return  PortableServer::POAManager::_duplicate(pman);
}
// ------------------------------------------------------------------------------------------
