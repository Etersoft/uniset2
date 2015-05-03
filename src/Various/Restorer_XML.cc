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
#include "Debug.h"
#include "Configuration.h"
#include "Restorer.h"
#include "Exceptions.h"
#include "ORepHelpers.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniversalIO;
using namespace UniSetTypes;
// -----------------------------------------------------------------------------
Restorer_XML::Restorer_XML():
	i_filterField(""),
	i_filterValue(""),
	c_filterField(""),
	c_filterValue("")
{
}
// -----------------------------------------------------------------------------
Restorer_XML::~Restorer_XML()
{

}
// -----------------------------------------------------------------------------
void Restorer_XML::setItemFilter( const string& field, const string& val )
{
	i_filterField = field;
	i_filterValue = val;
}
// -----------------------------------------------------------------------------
void Restorer_XML::setConsumerFilter( const string& field, const string& val )
{
	c_filterField = field;
	c_filterValue = val;
}
// -----------------------------------------------------------------------------
bool Restorer_XML::getConsumerInfo( UniXML::iterator& it,
									ObjectId& cid, ObjectId& cnode )
{
	if( !check_consumer_item(it) )
		return false;

	string cname( it.getProp("name"));

	if( cname.empty() )
	{
		uwarn << "(Restorer_XML:getConsumerInfo): не указано имя заказчика..." << endl;
		return false;
	}

	string otype(it.getProp("type"));

	if( otype == "controllers" )
		cname = uniset_conf()->getControllersSection() + "/" + cname;
	else if( otype == "objects" )
		cname = uniset_conf()->getObjectsSection() + "/" + cname;
	else if( otype == "services" )
		cname = uniset_conf()->getServicesSection() + "/" + cname;
	else
	{
		uwarn << "(Restorer_XML:getConsumerInfo): неизвестный тип объекта "
			  << otype << endl;
		return false;
	}

	cid = uniset_conf()->oind->getIdByName(cname);

	if( cid == UniSetTypes::DefaultObjectId )
	{
		ucrit << "(Restorer_XML:getConsumerInfo): НЕ НАЙДЕН ИДЕНТИФИКАТОР заказчика -->"
			  << cname << endl;
		return false;
	}

	string cnodename(it.getProp("node"));

	if( !cnodename.empty() )
		cnode = uniset_conf()->oind->getIdByName(cnodename);
	else
		cnode = uniset_conf()->getLocalNode();

	if( cnode == UniSetTypes::DefaultObjectId )
	{
		ucrit << "(Restorer_XML:getConsumerInfo): НЕ НАЙДЕН ИДЕНТИФИКАТОР узла -->"
			  << cnodename << endl;
		return false;
	}

	uinfo << "(Restorer_XML:getConsumerInfo): " << cname << ":" << cnodename << endl;
	return true;
}
// -----------------------------------------------------------------------------
bool Restorer_XML::check_list_item( UniXML::iterator& it )
{
	return UniSetTypes::check_filter(it, i_filterField, i_filterValue);
}
// -----------------------------------------------------------------------------
bool Restorer_XML::check_consumer_item( UniXML::iterator& it )
{
	return UniSetTypes::check_filter(it, c_filterField, c_filterValue);
}
// -----------------------------------------------------------------------------
xmlNode* Restorer_XML::find_node( const std::shared_ptr<UniXML>& xml, xmlNode* root,
								  const string& nodename, const string& nm )
{
	UniXML::iterator it(root);

	if( it.goChildren() )
	{
		for( ; it; it.goNext() )
		{
			if( it.getName() == nodename )
			{
				if( nm.empty() )
					return it;

				if( xml->getProp(it, "name") == nm )
					return it;
			}
		}
	}

	return 0;
}
// -----------------------------------------------------------------------------
void Restorer_XML::setReadItem( ReaderSlot sl )
{
	rslot = sl;
}
// -----------------------------------------------------------------------------
void Restorer_XML::setReadConsumerItem( ReaderSlot sl )
{
	cslot = sl;
}
// -----------------------------------------------------------------------------
