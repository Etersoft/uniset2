/* File: This file is part of the UniSet project
 * Copyright (C) 2002 Vitaly Lipatov
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
 * \author Vitaly Lipatov
 */
// -------------------------------------------------------------------------- 
#ifndef ObjcetIndex_H_
#define ObjcetIndex_H_
// --------------------------------------------------------------------------
#include <ostream>
#include <string>
#include "UniSetTypes.h"
// --------------------------------------------------------------------------
namespace UniSetTypes
{
class ObjectIndex
{
    public:
        ObjectIndex(){};
        virtual ~ObjectIndex(){};

        // info
        virtual const ObjectInfo* getObjectInfo( const UniSetTypes::ObjectId )=0;
        virtual const ObjectInfo* getObjectInfo( const std::string& name )=0;

        static std::string getBaseName( const std::string& fname );

        // object id
        virtual ObjectId getIdByName(const std::string& name)=0;
        virtual std::string getNameById( const UniSetTypes::ObjectId id );

        // node
        inline std::string getNodeName( const UniSetTypes::ObjectId id )
        {
            return getNameById(id);
        }

        inline ObjectId getNodeId( const std::string& name ) { return getIdByName(name); }

        // src name
        virtual std::string getMapName( const UniSetTypes::ObjectId id )=0;
        virtual std::string getTextName( const UniSetTypes::ObjectId id )=0;

        //
        virtual std::ostream& printMap(std::ostream& os)=0;

        void initLocalNode( const UniSetTypes::ObjectId nodeid );

    protected:
        std::string nmLocalNode;    // для оптимизации

    private:

};


// -----------------------------------------------------------------------------------------
}    // end of namespace
// -----------------------------------------------------------------------------------------
#endif
