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
// -------------------------------------------------------------------------
#include <sstream>
#include <iostream>
#include "UniXML.h"
#include "Extensions.h"
#include "Schema.h"
#include "TDelay.h"

// -------------------------------------------------------------------------
using namespace std;
using namespace UniSetExtensions;
// -------------------------------------------------------------------------
SchemaXML::SchemaXML()
{
}

SchemaXML::~SchemaXML()
{
}
// -------------------------------------------------------------------------
void SchemaXML::read( const string& xmlfile )
{
	UniXML xml;

	const string sec("elements");
	const string conn_sec("connections");

	//    try
	//    {
	xml.open(xmlfile);
	//    }
	//    catch(...){}

	xmlNode* root( xml.findNode(xml.getFirstNode(), sec) );

	if( !root )
	{
		ostringstream msg;
		msg << "(SchemaXML::read): не нашли корневого раздела " << sec;
		throw LogicException(msg.str());
	}

	// Считываем список элементов
	UniXML::iterator it(root);

	if( !it.goChildren() )
	{
		ostringstream msg;
		msg << "(SchemaXML::read): не удалось перейти к списку элементов " << sec;
		throw LogicException(msg.str());
	}

	for( ; it.getCurrent(); it.goNext() )
	{
		string type(xml.getProp(it, "type"));
		string ID(xml.getProp(it, "id"));
		int inCount = xml.getPIntProp(it, "inCount", 1);

		if( type == "OR" )
			manage( make_shared<TOR>(ID, inCount) );
		else if( type == "AND" )
			manage( make_shared<TAND>(ID, inCount) );
		else if( type == "Delay" )
		{
			int delayMS = xml.getIntProp(it, "delayMS");
			manage( make_shared<TDelay>(ID, delayMS, inCount) );
		}
		else if( type == "NOT" )
		{
			bool defout = xml.getIntProp(it, "default_out_state");
			manage( make_shared<TNOT>(ID, defout) );
		}
		else
		{
			ostringstream msg;
			msg << "(SchemaXML::read): НЕИЗВЕСТНЫЙ ТИП ЭЛЕМЕНТА -->" << type;
			throw LogicException(msg.str());
		}
	}

	// Строим связи
	xmlNode* conNode( xml.findNode(xml.getFirstNode(), conn_sec) );

	if( !conNode )
	{
		ostringstream msg;
		msg << "(SchemaXML::read): не нашли корневого раздела " << conn_sec;
		throw LogicException(msg.str());
	}

	it = conNode;

	if( !it.goChildren() )
	{
		ostringstream msg;
		msg << "(SchemaXML::read): не удалось перейти к списку элементов " << conn_sec;
		throw LogicException(msg.str());
	}

	for( ; it.getCurrent(); it.goNext() )
	{
		string type(xml.getProp(it, "type"));
		string fID(xml.getProp(it, "from"));
		string tID(xml.getProp(it, "to"));
		int toIn = xml.getIntProp(it, "toInput");

		if( type == "ext" )
		{
			dinfo << "SchemaXML: set EXTlink: from=" << fID << " to=" << tID << " toInput=" << toIn  << endl;
			extlink(fID, tID, toIn);
		}
		else if( type == "int" )
		{
			dinfo << "SchemaXML: set INTlink: from=" << fID << " to=" << tID << " toInput=" << toIn  << endl;
			link(fID, tID, toIn);
		}
		else if( type == "out" )
		{
			auto el = find(fID);

			if( !el )
			{
				ostringstream msg;
				msg << "(SchemaXML::read): НЕ НАЙДЕН ЭЛЕМЕНТ С ID=" << fID;
				throw LogicException(msg.str());
			}

			dinfo << "SchemaXML: set Out: from=" << fID << " to=" << tID << endl;
			outList.push_front( EXTOut(tID, el) );
		}
	}
}
// -------------------------------------------------------------------------
