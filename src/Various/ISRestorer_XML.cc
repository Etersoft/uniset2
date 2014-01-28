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
#include <fstream>
#include "Debug.h"
#include "Configuration.h"
#include "ISRestorer.h"
#include "Exceptions.h"
#include "ORepHelpers.h"
// ------------------------------------------------------------------------------------------
using namespace std;
using namespace UniversalIO;
using namespace UniSetTypes;
// ------------------------------------------------------------------------------------------
ISRestorer_XML::ISRestorer_XML(string fname):
	fname(fname)
{
	init(fname);
}

// ------------------------------------------------------------------------------------------
ISRestorer_XML::~ISRestorer_XML()
{
}
// ------------------------------------------------------------------------------------------
void ISRestorer_XML::init( std::string fname )
{
	/*! 
		\warning Файл открывается только при создании... 
		Т.е. не будут учтены изменения в промежутке между записью(dump-а) файла
	*/
	try
	{
		uxml.open(fname);
	}
	catch (...){}
}
// ------------------------------------------------------------------------------------------
void ISRestorer_XML::dump( InfoServer* is, const UniSetTypes::MessageCode code,
								const InfoServer::ConsumerList& lst )
{
	if( unideb.debugging(Debug::WARN) )	
		unideb[Debug::WARN] << "ISRestorer_XML::dump NOT SUPPORT!!!!" << endl;
}
// ------------------------------------------------------------------------------------------
void ISRestorer_XML::read( InfoServer* is, const string fn )
{
	UniXML* confxml = conf->getConfXML();

	if( !fn.empty() )
	{
		 // оптимизация (не загружаем второй раз xml-файл)
		if( fn == conf->getConfFileName() && confxml )
			read( is, *confxml );
		else
		{
			UniXML xml(fn);
			read(is,xml);
			xml.close();
		}
	}
	else
	{
		// оптимизация (не загружаем второй раз xml-файл)
		if( fname == conf->getConfFileName() && confxml )
			read( is, *confxml );
		else
		{
			uxml.close();
			uxml.open(fname);
			read(is,uxml);
		}
	}
}
// ------------------------------------------------------------------------------------------

void ISRestorer_XML::read( InfoServer* is, UniXML& xml )
{
	xmlNode* node = find_node(xml, xml.getFirstNode(),"messages");
	if( node )
		read_list(xml, node, is);
}
// ------------------------------------------------------------------------------------------

void ISRestorer_XML::read_list(UniXML& xml, xmlNode* node, InfoServer* is )
{
	UniXML_iterator it(node);

	bool autoID=it.getIntProp("autoID");

	if( !it.goChildren() )
		return;

	for( ;it.getCurrent(); it.goNext() )
	{
		if( !check_list_item(it) )
			continue;
	
		MessageCode code;
		if( !autoID )
			code = conf->mi->getCodeByIdName(it.getProp("name"));
		else
		{
			code = xml.getIntProp(it,"id");
		}
		
		if( code == UniSetTypes::DefaultMessageCode )
		{
			unideb[Debug::WARN] << "(read_list): не смог получить message code для "
				<< it.getProp("name") << endl;
			continue;
		}

		UniXML_iterator itc(it.getCurrent());
		if( !itc.goChildren() )
			continue;

		InfoServer::ConsumerList lst;
		for(;itc.getCurrent();itc.goNext())
		{
			if( !check_consumer_item(itc) )
				continue;

			ConsumerInfo ci;
			if( !getConsumerInfo(it,ci.id,ci.node) )
				continue;

			InfoServer::ConsumerInfoExt cinf(ci);
			cinf.ask = xml.getIntProp(itc,"ask");
			lst.push_front(cinf);
			cslot(xml,itc,it.getCurrent());
		}

		rslot(xml,it,node);
		
		if( !lst.empty() )
			addlist(is,code,lst);
	}
}
// ------------------------------------------------------------------------------------------
