// This file is part of the UniSet project.

/* This file is part of the UniSet project
 * Copyright (C) 2002 Vitaly Lipatov <lav@etersoft.ru>
 * Copyright (C) 2003 Pavel Vainerman <pv@etersoft.ru>
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
// -----------------------------------------------------------------------------------------
/*! 
    \todo Добавить проверку на предельный номер id
*/
// -----------------------------------------------------------------------------------------
#include "ObjectIndex.h"
#include "ORepHelpers.h"
#include "Configuration.h"

// -----------------------------------------------------------------------------------------
using namespace UniSetTypes;
// -----------------------------------------------------------------------------------------
//const std::string ObjectIndex::sepName = "@";
//const std::string ObjectIndex::sepNode = ":";
// -----------------------------------------------------------------------------------------

string ObjectIndex::getNameById( const ObjectId id )
{
    return getMapName(id);
}
// -----------------------------------------------------------------------------------------
std::string ObjectIndex::getBaseName( const std::string& fname )
{
    string::size_type pos = fname.rfind('/');
    if( pos != string::npos )
        return fname.substr(pos+1);

    return fname;
}
// -----------------------------------------------------------------------------------------
void ObjectIndex::initLocalNode( const ObjectId nodeid )
{
    nmLocalNode = getMapName(nodeid);
}
// -----------------------------------------------------------------------------------------
