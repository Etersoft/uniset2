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
 * \author Pavel Vainerman
 */
// -------------------------------------------------------------------------- 
#ifndef ObjectIndex_XML_H_
#define ObjectIndex_XML_H_
// --------------------------------------------------------------------------
#include <map>
#include <vector>
#include <string>
#include "ObjectIndex.h"
#include "UniXML.h"
// --------------------------------------------------------------------------
namespace UniSetTypes
{

/*! \todo Проверить функции этого класса на повторную входимость */
class ObjectIndex_XML:
    public ObjectIndex
{
    public:
        ObjectIndex_XML(const std::string& xmlfile, int minSize=1000 );
        ObjectIndex_XML(UniXML& xml, int minSize=1000 );
        virtual ~ObjectIndex_XML();

        virtual const UniSetTypes::ObjectInfo* getObjectInfo(const ObjectId);
        virtual const UniSetTypes::ObjectInfo* getObjectInfo( const std::string& name );
        virtual ObjectId getIdByName(const std::string& name);
        virtual std::string getMapName(const ObjectId id);
        virtual std::string getTextName(const ObjectId id);

        virtual std::ostream& printMap(std::ostream& os);
        friend std::ostream& operator<<(std::ostream& os, ObjectIndex_XML& oi );

    protected:
        virtual void build(UniXML& xml);
        unsigned int read_section( UniXML& xml, const std::string& sec, unsigned int ind );
        unsigned int read_nodes( UniXML& xml, const std::string& sec, unsigned int ind );

    private:
        typedef std::map<std::string, ObjectId> MapObjectKey;
        MapObjectKey mok; // для обратного писка
        std::vector<ObjectInfo> omap; // для прямого поиска

};
// -----------------------------------------------------------------------------------------
}    // end of namespace
// -----------------------------------------------------------------------------------------
#endif
