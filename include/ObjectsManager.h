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
// --------------------------------------------------------------------------
/*! \file
 * \brief Реализация интерфейса менеджера объектов.
 * \author Pavel Vainerman <pv>
 * \date  $Date: 2009/01/16 23:16:42 $
 * \version $Id: ObjectsManager.h,v 1.13 2009/01/16 23:16:42 vpashka Exp $
 */
// -------------------------------------------------------------------------- 
#ifndef ObjectsManager_H_
#define ObjectsManager_H_
// -------------------------------------------------------------------------- 
#include <omniORB4/CORBA.h>
#include "UniSetTypes.h"
#include "UniSetObject.h"
#include "ObjectsManager_i.hh"
//---------------------------------------------------------------------------
class ObjectsActivator;

class ObjectsManager;
typedef std::list<ObjectsManager*> ObjectsManagerList;		
//---------------------------------------------------------------------------
/*! \class ObjectsManager
 *	\par
 *	Содержит в себе функции управления объектами. Их регистрации и т.п.
 *	Создается менеджер объектов, после чего вызывается initObjects() 
 *	для инициализации объектов которыми управляет 
 *	данный менеджер...
 *	Менеджер в свою очередь сам является объектом и обладает всеми его свойствами	
 *	
 * 	Для пересылки сообщения всем подчиненным объектам используется 
 *		функция ObjectsManager::broadcast(const TransportMessage& msg)
 *	\par
 * 	У базового менеджера имеются базовые три функции см. ObjectsManager_i. 
 *	\note Только при вызове функции ObjectsManager::broadcast() происходит 
 *		формирование сообщения всем подчиненным объектам... Если команда проиходит
 *	при помощи push, то пересылки всем починённым объектам не происходит...
 *
 *
*/ 
class ObjectsManager: 
	public UniSetObject,
	public POA_ObjectsManager_i
{
	public:
		ObjectsManager( UniSetTypes::ObjectId id);
		ObjectsManager( const std::string name, const std::string section );
		virtual ~ObjectsManager();


		virtual UniSetTypes::ObjectType getType(){ return "ObjectsManager"; }

		// ------  функции объявленные в интерфейсе(IDL) ------
		virtual void broadcast(const UniSetTypes::TransportMessage& msg);
		virtual UniSetTypes::SimpleInfoSeq* getObjectsInfo( CORBA::Long MaxLength=300 );

		// --------------------------
		void initPOA(ObjectsManager* rmngr);

		virtual bool addObject(UniSetObject *obj);
		virtual bool removeObject(UniSetObject *obj);

		virtual bool addManager( ObjectsManager *mngr );
		virtual bool removeManager( ObjectsManager *mngr );

		
		/*! Получение доступа к подчиненному менеджеру по идентификатору
		 * \return объект ненайден будет возвращен 0.
		*/ 
		const ObjectsManager* itemM(const UniSetTypes::ObjectId id);


		/*! Получение доступа к подчиненному объекту по идентификатору
		 * \return объект ненайден будет возвращен 0.
		*/ 
		const UniSetObject* itemO( const UniSetTypes::ObjectId id );

		
		// Функции для аботы со списками подчиненных объектов
		inline ObjectsManagerList::const_iterator beginMList()
		{
			return mlist.begin();
		}

		inline ObjectsManagerList::const_iterator endMList()
		{
			return mlist.end();
		}

		inline ObjectsList::const_iterator beginOList()
		{
			return olist.begin();
		}

		inline ObjectsList::const_iterator endOList()
		{
			return olist.end();
		}

		int objectsCount();	// количество подчиненных объектов


		PortableServer::POA_ptr getPOA(){ return PortableServer::POA::_duplicate(poa); }
		PortableServer::POAManager_ptr getPOAManager(){ return  PortableServer::POAManager::_duplicate(pman); }

	protected:

		ObjectsManager();

		enum OManagerCommand{deactiv, activ, initial, term};

		// работа со списком объектов
		void objects(OManagerCommand cmd);
		// работа со списком менеджеров
		void managers(OManagerCommand cmd);

		virtual void sigterm( int signo );

		//! \note Переопределяя не забывайте вызвать базовую 
		virtual bool activateObject();
		//! \note Переопределяя не забывайте вызвать базовую 
		virtual bool disactivateObject();


		typedef ObjectsManagerList::iterator MListIterator;		

		int getObjectsInfo( ObjectsManager* mngr, UniSetTypes::SimpleInfoSeq* seq, 
							int begin, const long uplimit );

		PortableServer::POA_var poa;	
		PortableServer::POAManager_var pman;

	private:
	
		friend class ObjectsActivator;
	
		int sig;
		ObjectsManagerList mlist;
		ObjectsList olist;

		UniSetTypes::uniset_mutex olistMutex;
		UniSetTypes::uniset_mutex mlistMutex;

};

#endif
