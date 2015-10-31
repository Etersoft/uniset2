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
#include <iomanip>
#include "ObjectIndex_Array.h"
#include "ORepHelpers.h"
#include "Configuration.h"

// -----------------------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
// -----------------------------------------------------------------------------------------
ObjectIndex_Array::~ObjectIndex_Array()
{

}
// -----------------------------------------------------------------------------------------
ObjectIndex_Array::ObjectIndex_Array( const ObjectInfo* objectInfo ):
	objectInfo(objectInfo),
	maxId(0)
{
	for (numOfObject = 0;; numOfObject++)
	{
		if( objectInfo[numOfObject].repName.empty() )
			break;

		assert (numOfObject == objectInfo[numOfObject].id);
		mok[objectInfo[numOfObject].repName] = numOfObject;
		maxId++;
	}
}
// -----------------------------------------------------------------------------------------
ObjectId ObjectIndex_Array::getIdByName( const string& name )
{
	auto it = mok.find(name);

	if( it != mok.end() )
		return it->second;

	return DefaultObjectId;
}

// -----------------------------------------------------------------------------------------
string ObjectIndex_Array::getMapName( const ObjectId id )
{
	if( id != UniSetTypes::DefaultObjectId && id >= 0 && id < maxId )
		return objectInfo[id].repName;

	return "";
	//    throw OutOfRange("ObjectIndex_Array::getMapName OutOfRange");
}
// -----------------------------------------------------------------------------------------
string ObjectIndex_Array::getTextName( const ObjectId id )
{
	if( id != UniSetTypes::DefaultObjectId && id >= 0 && id < maxId )
		return objectInfo[id].textName;

	return "";
	//    throw OutOfRange("ObjectIndex_Array::getTextName OutOfRange");
}
// -----------------------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, ObjectIndex_Array& oi )
{
	return oi.printMap(os);
}
// -----------------------------------------------------------------------------------------

std::ostream& ObjectIndex_Array::printMap( std::ostream& os )
{
	auto oind = uniset_conf()->oind;

	for( auto i = 0;; i++)
	{
		if( objectInfo[i].repName.empty() )
			break;

		assert (i == objectInfo[i].id);

		os  << setw(5) << objectInfo[i].id << "  "
			<< setw(45) << oind->getBaseName(objectInfo[i].repName)
			<< "  " << objectInfo[i].textName << endl;
	}

	return os;
}
// -----------------------------------------------------------------------------------------
const ObjectInfo* ObjectIndex_Array::getObjectInfo( const ObjectId id )
{
	if( id != UniSetTypes::DefaultObjectId && id >= 0 && id < maxId )
		return &(objectInfo[id]);

	return NULL;
}
// -----------------------------------------------------------------------------------------
const ObjectInfo* ObjectIndex_Array::getObjectInfo( const std::string& name )
{
	auto it = mok.find(name);

	if( it != mok.end() )
		return &(objectInfo[it->second]);

	return NULL;
}
// ------------------------------------------------------------------------------------------
