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
#include <sstream>
#include "Exceptions.h" 
#include "Debug.h" 
#include "Configuration.h"
#include "ProxyManager.h"
#include "PassiveObject.h"
// -------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
// -------------------------------------------------------------------------

ProxyManager::~ProxyManager()
{
}

ProxyManager::ProxyManager( UniSetTypes::ObjectId id ):
    UniSetObject(id)
{
    uin = &ui;
}


// -------------------------------------------------------------------------
void ProxyManager::attachObject( PassiveObject* po, UniSetTypes::ObjectId id )
{
    if( id == DefaultObjectId )
    {
        uwarn << myname << "(attachObject): попытка добавить объект с id="
            << DefaultObjectId << " PassiveObject=" << po->getName() << endl;

        return;
    }

    PObjectMap::iterator it = omap.find(id);
    if( it==omap.end() )
        omap.insert(PObjectMap::value_type(id,po));
}
// -------------------------------------------------------------------------
void ProxyManager::detachObject( UniSetTypes::ObjectId id )
{
    PObjectMap::iterator it = omap.find(id);
    if( it!=omap.end() )
        omap.erase(it);
}
// -------------------------------------------------------------------------
bool ProxyManager::activateObject()
{
    bool ret = UniSetObject::activateObject();
    if( !ret )
        return false;

    // Регистрируемся от имени объектов
    for( PObjectMap::const_iterator it=omap.begin();it!=omap.end();++it )
    {
        try
        {
            for( unsigned int i=0; i<2; i++ )
            {
                try
                {
                    uinfo << myname << "(registered): попытка "
                          << i+1 << " регистриую (id=" << it->first << ") "
                          << " (pname=" << it->second->getName() << ") "
                          << conf->oind->getNameById(it->first) << endl;

                    ui.registered(it->first, getRef(),true);
                    break;
                }
                catch( UniSetTypes::ObjectNameAlready& ex )
                {
                    ucrit << myname << "(registered): СПЕРВА РАЗРЕГИСТРИРУЮ (ObjectNameAlready)" << endl;
                    try
                    {
                        ui.unregister(it->first);
                    }
                    catch(Exception & ex)
                    {
                        ucrit << myname << "(unregistered): " << ex << endl;
                    }
                }
            }
        }
        catch( Exception& ex )
        {
            ucrit << myname << "(activate): " << ex << endl;
        }
    }

    return ret;
}
// -------------------------------------------------------------------------
bool ProxyManager::disactivateObject()
{
    for( PObjectMap::const_iterator it=omap.begin();it!=omap.end();++it )
    {
        try
        {
            ui.unregister(it->first);
        }
        catch(Exception& ex )
        {
            ucrit << myname << "(activate): " << ex << endl;
        }
    }

    return UniSetObject::disactivateObject();
}
// -------------------------------------------------------------------------
void ProxyManager::processingMessage( UniSetTypes::VoidMessage *msg )
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
                PObjectMap::iterator it = omap.find(msg->consumer);
                if( it!=omap.end() )
                    it->second->processingMessage(msg);
                else
                    ucrit << myname << "(processingMessage): не найден объект "
                        << " consumer= " << msg->consumer << endl;
            }
            break;
        }
    }
    catch( Exception& ex )
    {
        ucrit << myname << "(processingMessage): " << ex << endl;
    }
}
// -------------------------------------------------------------------------
void ProxyManager::allMessage( UniSetTypes::VoidMessage* msg )
{
    for( PObjectMap::const_iterator it=omap.begin();it!=omap.end();++it )
    {
        try
        {
            it->second->processingMessage(msg);
        }
        catch( Exception& ex )
        {
            ucrit << myname << "(allMessage): " << ex << endl;
        }
    }
}
// -------------------------------------------------------------------------
