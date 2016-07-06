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
 * \author Pavel Vainerman
*/
// --------------------------------------------------------------------------
#ifndef ProxyManager_H_
#define ProxyManager_H_
//---------------------------------------------------------------------------
#include <unordered_map>
#include <memory>
#include "UniSetObject.h"

//----------------------------------------------------------------------------
class PassiveObject;
//----------------------------------------------------------------------------

/*! \class ProxyManager
 *    Менеджер пассивных объектов, который выступает вместо них во всех внешних связях....
*/
class ProxyManager:
	public UniSetObject
{

	public:
		ProxyManager( UniSetTypes::ObjectId id );
		~ProxyManager();

		void attachObject( PassiveObject* po, UniSetTypes::ObjectId id );
		void detachObject( UniSetTypes::ObjectId id );

		std::shared_ptr<UInterface> uin;

	protected:
		ProxyManager();
		virtual void processingMessage( const UniSetTypes::VoidMessage* msg ) override;
		virtual void allMessage( const UniSetTypes::VoidMessage* msg );

		virtual bool activateObject() override;
		virtual bool deactivateObject() override;

	private:
		typedef std::unordered_map<UniSetTypes::ObjectId, PassiveObject*> PObjectMap;
		PObjectMap omap;
};
//----------------------------------------------------------------------------------------
#endif // ProxyManager
//----------------------------------------------------------------------------------------
