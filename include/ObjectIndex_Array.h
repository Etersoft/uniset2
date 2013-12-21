/* File: This file is part of the UniSet project
 * Copyright (C) 2002 Vitaly Lipatov, Pavel Vainerman
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
#ifndef ObjectIndex_Array_H_
#define ObjectIndex_Array_H_
// --------------------------------------------------------------------------
#include <string>
#include <map>
#include <ostream>
#include "UniSetTypes.h"
#include "Exceptions.h"
#include "ObjectIndex.h"
// --------------------------------------------------------------------------
namespace UniSetTypes
{
/*!
    \todo Проверить функции этого класса на повторную входимость
    \bug При обращении к objectsMap[0].textName срабатывает исключение(видимо какое-то из std). 
        Требуется дополнительное изучение.
*/
class ObjectIndex_Array:
    public ObjectIndex
{
    public:
        ObjectIndex_Array(const ObjectInfo* objectInfo);
        virtual ~ObjectIndex_Array();


        virtual const ObjectInfo* getObjectInfo(const ObjectId);
        virtual const ObjectInfo* getObjectInfo( const std::string& name );
        virtual ObjectId getIdByName(const std::string& name);
        virtual std::string getMapName(const ObjectId id);
        virtual std::string getTextName(const ObjectId id);

        virtual std::ostream& printMap(std::ostream& os);
        friend std::ostream& operator<<(std::ostream& os, ObjectIndex_Array& oi );
        
    private:
        
        int numOfObject;
        typedef std::map<std::string, ObjectId> MapObjectKey;                                     
        MapObjectKey::iterator MapObjectKeyIterator;
        MapObjectKey mok;
        const ObjectInfo *objectInfo;
        int maxId;
};
// -----------------------------------------------------------------------------------------
}    // end of namespace
// -----------------------------------------------------------------------------------------
#endif
