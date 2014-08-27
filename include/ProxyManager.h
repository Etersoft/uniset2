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
 * \author Pavel Vainerman
*/
// -------------------------------------------------------------------------- 
#ifndef ProxyManager_H_
#define ProxyManager_H_
//---------------------------------------------------------------------------
#include <map>
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
    
        UInterface* uin;

    protected:
        ProxyManager();    
        virtual void processingMessage( UniSetTypes::VoidMessage* msg );
        virtual void allMessage( UniSetTypes::VoidMessage* msg );

        virtual bool activateObject();
        virtual bool deactivateObject();

    private:
        typedef std::map<UniSetTypes::ObjectId, PassiveObject*> PObjectMap;
        PObjectMap omap;
};
//----------------------------------------------------------------------------------------
#endif // ProxyManager
//----------------------------------------------------------------------------------------
