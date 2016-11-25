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
#include <sstream>
#include "Exceptions.h"
#include "Debug.h"
#include "Configuration.h"
#include "ProxyManager.h"
#include "PassiveObject.h"
// -------------------------------------------------------------------------
using namespace uniset;
using namespace std;
// -------------------------------------------------------------------------

ProxyManager::~ProxyManager()
{
}

ProxyManager::ProxyManager( uniset::ObjectId id ):
	UniSetObject(id)
{
	uin = ui;
}


// -------------------------------------------------------------------------
void ProxyManager::attachObject( PassiveObject* po, uniset::ObjectId id )
{
	if( id == DefaultObjectId )
	{
		uwarn << myname << "(attachObject): попытка добавить объект с id="
			  << DefaultObjectId << " PassiveObject=" << po->getName() << endl;

		return;
	}

	auto it = omap.find(id);

	if( it == omap.end() )
		omap.emplace(id, po);
}
// -------------------------------------------------------------------------
void ProxyManager::detachObject( uniset::ObjectId id )
{
	auto it = omap.find(id);

	if( it != omap.end() )
		omap.erase(it);
}
// -------------------------------------------------------------------------
bool ProxyManager::activateObject()
{
	bool ret = UniSetObject::activateObject();

	if( !ret )
		return false;

	// Регистрируемся от имени объектов
	for( auto& it : omap )
	{
		try
		{
			for( unsigned int i = 0; i < 2; i++ )
			{
				try
				{
					ulogrep << myname << "(registered): попытка "
							<< i + 1 << " регистриую (id=" << it.first << ") "
							<< " (pname=" << it.second->getName() << ") "
							<< uniset_conf()->oind->getNameById(it.first) << endl;

					ui->registered(it.first, getRef(), true);
					break;
				}
				catch( uniset::ObjectNameAlready& ex )
				{
					ucrit << myname << "(registered): СПЕРВА РАЗРЕГИСТРИРУЮ (ObjectNameAlready)" << endl;

					try
					{
						ui->unregister(it.first);
					}
					catch( const uniset::Exception& ex )
					{
						ucrit << myname << "(unregistered): " << ex << endl;
					}
				}
			}
		}
		catch( const uniset::Exception& ex )
		{
			ucrit << myname << "(activate): " << ex << endl;
		}
	}

	return ret;
}
// -------------------------------------------------------------------------
bool ProxyManager::deactivateObject()
{
	for( PObjectMap::const_iterator it = omap.begin(); it != omap.end(); ++it )
	{
		try
		{
			ui->unregister(it->first);
		}
		catch( const uniset::Exception& ex )
		{
			ucrit << myname << "(activate): " << ex << endl;
		}
	}

	return UniSetObject::deactivateObject();
}
// -------------------------------------------------------------------------
void ProxyManager::processingMessage( const uniset::VoidMessage* msg )
{
	try
	{
		switch(msg->type)
		{
			case Message::SysCommand:
				allMessage(msg);
				break;

			default:
			{
				auto it = omap.find(msg->consumer);

				if( it != omap.end() )
					it->second->processingMessage(msg);
				else
					ucrit << myname << "(processingMessage): не найден объект "
						  << " consumer= " << msg->consumer << endl;
			}
			break;
		}
	}
	catch( const uniset::Exception& ex )
	{
		ucrit << myname << "(processingMessage): " << ex << endl;
	}
}
// -------------------------------------------------------------------------
void ProxyManager::allMessage( const uniset::VoidMessage* msg )
{
	for( auto& o : omap )
	{
		try
		{
			o.second->processingMessage(msg);
		}
		catch( const uniset::Exception& ex )
		{
			ucrit << myname << "(allMessage): " << ex << endl;
		}
	}
}
// -------------------------------------------------------------------------
