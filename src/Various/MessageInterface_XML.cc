// This file is part of the UniSet project.

/* This file is part of the UniSet project
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
#include <sstream>
#include <iomanip>
#include "MessageInterface_XML.h"
#include "Configuration.h"

// -----------------------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
// -----------------------------------------------------------------------------------------
MessageInterface_XML::MessageInterface_XML( const string xmlfile, int msize ):
	msgmap(msize)
{
	string mfile(xmlfile);

	if( mfile.empty() )
		mfile = conf->getConfFileName();

	UniXML xml;
//	try
//	{
		xml.open(mfile);
		read(xml);
//	}
//	catch(...){}

}
// ------------------------------------------------------------------------------------------
MessageInterface_XML::MessageInterface_XML( UniXML& xml, int msize ):
	msgmap(msize)
{
	read(xml);
}
// ------------------------------------------------------------------------------------------
MessageInterface_XML::~MessageInterface_XML()
{

}

// ------------------------------------------------------------------------------------------

string MessageInterface_XML::getMessage( UniSetTypes::MessageCode code )
{
	if( (unsigned)code<0 || (unsigned)code>=msgmap.size() )
		return "";

	return msgmap[code].text;
}

// ------------------------------------------------------------------------------------------

bool MessageInterface_XML::isExist( UniSetTypes::MessageCode code )
{
	if( (unsigned)code<0 || (unsigned)code>= msgmap.size() )
		return false;
	
	return true;
}

// ------------------------------------------------------------------------------------------

UniSetTypes::MessageCode MessageInterface_XML::getCode( const string& msg )
{
	for( unsigned int i=0; i<msgmap.size(); i++ )
	{
		if( msgmap[i].text == msg )
			return msgmap[i].code;
	}

	return DefaultMessageCode;
}


// ------------------------------------------------------------------------------------------

UniSetTypes::MessageCode MessageInterface_XML::getCodeByIdName( const string& name )
{
	for( unsigned int i=0; i<msgmap.size(); i++ )
	{
		if( msgmap[i].idname == name )
			return msgmap[i].code;
	}

	return DefaultMessageCode;
}

// ------------------------------------------------------------------------------------------
void MessageInterface_XML::read( UniXML& xml )
{
	xmlNode* root( xml.findNode(xml.getFirstNode(),"messages") );
	if( !root )
	{
		ostringstream msg;
		msg << "(MessageInterface_XML::read): не нашли корневого раздела 'messages'";
		throw NameNotFound(msg.str());
	}

	UniXML_iterator it(root);
	if( !it.goChildren() )
	{
		ostringstream msg;
		msg << "(MessageInterface_XML::read): не удалось перейти к списку элементов 'messages'";
		throw NameNotFound(msg.str());
	}

	unsigned int ind=0;

	for( ;it.getCurrent(); it.goNext() )
	{
		msgmap[ind].idname 	= xml.getProp(it,"name");
		msgmap[ind].text	= xml.getProp(it,"text");
		msgmap[ind].code	= ind;

		ind++;
		if( ind >= msgmap.size() )
		{
			ostringstream msg;
			msg << "(MessageInterface_XML::read): не хватило размера массива maxSize=" << msgmap.size();
			unideb[Debug::WARN] << msg.str() << "... Делаем resize + 100\n";
			msgmap.resize(msgmap.size()+100);
		}
	}
	
	// подгоняем под реальный размер
	msgmap.resize(ind);
}

// ------------------------------------------------------------------------------------------
std::ostream& MessageInterface_XML::printMessagesMap( std::ostream& os )
{
	cout << "size: " << msgmap.size() << endl;
	for( unsigned int i=0; i<msgmap.size(); i++ )
	{
		os  << setw(5) << msgmap[i].code << " "
			<< setw(10) << msgmap[i].idname << " "  
			<< setw(45) << msgmap[i].text << endl;
	}

	return os;
}
// -----------------------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, MessageInterface_XML& mi )
{
	return mi.printMessagesMap(os);
}
// -----------------------------------------------------------------------------------------
