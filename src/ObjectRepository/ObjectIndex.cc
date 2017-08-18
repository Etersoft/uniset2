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
// -----------------------------------------------------------------------------------------
/*!
    \todo Добавить проверку на предельный номер id
*/
// -----------------------------------------------------------------------------------------
#include "ObjectIndex.h"
#include "ORepHelpers.h"
#include "Configuration.h"

// -----------------------------------------------------------------------------------------
using namespace uniset;
// -----------------------------------------------------------------------------------------
//const std::string ObjectIndex::sepName = "@";
//const std::string ObjectIndex::sepNode = ":";
// -----------------------------------------------------------------------------------------

std::string ObjectIndex::getNameById( const ObjectId id ) const noexcept
{
	return getMapName(id);
}
// -----------------------------------------------------------------------------------------
std::string ObjectIndex::getNodeName(const ObjectId id) const noexcept
{
	return getNameById(id);
}
// -----------------------------------------------------------------------------------------
ObjectId ObjectIndex::getNodeId(const std::string& name) const noexcept
{
	return getIdByName(name);
}
// -----------------------------------------------------------------------------------------
std::string ObjectIndex::getBaseName( const std::string& fname ) noexcept
{
	std::string::size_type pos = fname.rfind('/');

	try
	{
		if( pos != std::string::npos )
			return fname.substr(pos + 1);
	}
	catch(...) {}

	return fname;
}
// -----------------------------------------------------------------------------------------
void ObjectIndex::initLocalNode( const ObjectId nodeid ) noexcept
{
	nmLocalNode = getMapName(nodeid);
}
// -----------------------------------------------------------------------------------------
