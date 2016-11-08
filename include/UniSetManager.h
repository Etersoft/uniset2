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
 * \brief Реализация интерфейса менеджера объектов.
 * \author Pavel Vainerman
 */
// --------------------------------------------------------------------------
#ifndef UniSetManager_H_
#define UniSetManager_H_
// --------------------------------------------------------------------------
#include <omniORB4/CORBA.h>
#include <memory>
#include "UniSetTypes.h"
#include "UniSetObject.h"
#include "UniSetManager_i.hh"
//---------------------------------------------------------------------------
namespace uniset
{
//---------------------------------------------------------------------------
class UniSetActivator;

class UniSetManager;
typedef std::list< std::shared_ptr<UniSetManager> > UniSetManagerList;
//---------------------------------------------------------------------------
/*! \class UniSetManager
 *    \par
 *    Содержит в себе функции управления объектами. Их регистрации и т.п.
 *    Создается менеджер объектов, после чего вызывается initObjects()
 *    для инициализации объектов которыми управляет
 *    данный менеджер...
 *    Менеджер в свою очередь сам является объектом и обладает всеми его свойствами
 *
 *     Для пересылки сообщения всем подчиненным объектам используется
 *        функция UniSetManager::broadcast(const TransportMessage& msg)
 *    \par
 *     У базового менеджера имеются базовые три функции см. UniSetManager_i.
 *    \note Только при вызове функции UniSetManager::broadcast() происходит
 *        формирование сообщения всем подчиненным объектам... Если команда проиходит
 *    при помощи push, то пересылки всем починённым объектам не происходит...
 *
 *
*/
class UniSetManager:
	public UniSetObject,
	public POA_UniSetManager_i
{
	public:
		UniSetManager( uniset::ObjectId id);
		UniSetManager( const std::string& name, const std::string& section );
		virtual ~UniSetManager();

		std::shared_ptr<UniSetManager> get_mptr();

		virtual uniset::ObjectType getType() override
		{
			return uniset::ObjectType("UniSetManager");
		}

		// ------  функции объявленные в интерфейсе(IDL) ------
		virtual void broadcast( const uniset::TransportMessage& msg) override;
		virtual uniset::SimpleInfoSeq* getObjectsInfo( CORBA::Long MaxLength = 300, CORBA::Long userparam = 0  ) override ;

		// --------------------------
		virtual bool add( const std::shared_ptr<UniSetObject>& obj );
		virtual bool remove( const std::shared_ptr<UniSetObject>& obj );
		// --------------------------
		/*! Получение доступа к подчиненному менеджеру по идентификатору
		 * \return shared_ptr<>, если объект не найден будет возвращен shared_ptr<> = nullptr
		*/
		const std::shared_ptr<UniSetManager> itemM(const uniset::ObjectId id);

		/*! Получение доступа к подчиненному объекту по идентификатору
		 * \return shared_ptr<>, если объект не найден будет возвращен shared_ptr<> = nullptr
		*/
		const std::shared_ptr<UniSetObject> itemO( const uniset::ObjectId id );

		// Функции для работы со списками подчиненных объектов
		// ---------------
		UniSetManagerList::const_iterator beginMList();
		UniSetManagerList::const_iterator endMList();
		ObjectsList::const_iterator beginOList();
		ObjectsList::const_iterator endOList();

		size_t objectsCount() const;    // количество подчиненных объектов
		// ---------------

		PortableServer::POA_ptr getPOA();
		PortableServer::POAManager_ptr getPOAManager();

	protected:

		UniSetManager();

		virtual bool addManager( const std::shared_ptr<UniSetManager>& mngr );
		virtual bool removeManager( const std::shared_ptr<UniSetManager>& mngr );
		virtual bool addObject( const std::shared_ptr<UniSetObject>& obj );
		virtual bool removeObject( const std::shared_ptr<UniSetObject>& obj );

		enum OManagerCommand { deactiv, activ, initial, term };
		friend std::ostream& operator<<(std::ostream& os, uniset::UniSetManager::OManagerCommand& cmd );

		// работа со списком объектов
		void objects(OManagerCommand cmd);
		// работа со списком менеджеров
		void managers(OManagerCommand cmd);

		virtual void sigterm( int signo ) override;

		void initPOA( const std::weak_ptr<UniSetManager>& rmngr );

		//! \note Переопределяя не забывайте вызвать базовую
		virtual bool activateObject() override;
		//! \note Переопределяя не забывайте вызвать базовую
		virtual bool deactivateObject() override;

		const std::shared_ptr<UniSetObject> findObject( const std::string& name );
		const std::shared_ptr<UniSetManager> findManager( const std::string& name );

		// рекурсивный поиск по всем объекам
		const std::shared_ptr<UniSetObject> deepFindObject( const std::string& name );

		// рекурсивное наполнение списка объектов
		void getAllObjectsList( std::vector<std::shared_ptr<UniSetObject>>& vec, size_t lim=1000 );

		typedef UniSetManagerList::iterator MListIterator;

		int getObjectsInfo(const std::shared_ptr<UniSetManager>& mngr, uniset::SimpleInfoSeq* seq,
						   int begin, const long uplimit, CORBA::Long userparam );

		PortableServer::POA_var poa;
		PortableServer::POAManager_var pman;

	private:

		int sig;
		UniSetManagerList mlist;
		ObjectsList olist;

		uniset::uniset_rwmutex olistMutex;
		uniset::uniset_rwmutex mlistMutex;
};
// -------------------------------------------------------------------------
} // end of uniset namespace
#endif
