/* This file is part of the UniSet project
 * Copyright (c) 2002 Free Software Foundation, Inc.
 * Copyright (c) 2002 Pavel Vainerman <pv>
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
 *  \author Pavel Vainerman <pv>
 *  \date   $Date: 2005/01/28 21:09:33 $
 *  \version $Id: UniSetObserver.cc,v 1.5 2005/01/28 21:09:33 vitlav Exp $
*/
// -------------------------------------------------------------------------- 

#include "UniSetObserver.h"
#include "Exceptions.h"
#include "UniversalInterface.h"
// ---------------------------------------------------------------------------
using namespace UniSetTypes;

// ---------------------------------------------------------------------------
UniSetSubject::UniSetSubject()
{
}

UniSetSubject::UniSetSubject(ObjectId id):
id(id)
{
}

UniSetSubject::~UniSetSubject()
{
}

// ---------------------------------------------------------------------------

void UniSetSubject::attach( UniSetObject* o )
{
	ObserverList::iterator li = find(lst.begin(),lst.end(),o);
	if( li==lst.end() )
	 	lst.push_front(o);
}

// ---------------------------------------------------------------------------

void UniSetSubject::detach( UniSetObject* o )
{
	lst.remove(o);
}

// ---------------------------------------------------------------------------

void UniSetSubject::notify(int notify, int data, Message::Priority priority)
{
	for( ObserverList::iterator li=lst.begin();li!=lst.end();++li )
	{
		try
		{
			UpdateMessage msg(id, notify, data, priority, (*li)->getId());
			(*li)->push( msg.transport_msg() );
		}
		catch(Exception& ex)
		{
			unideb[Debug::WARN] << ex << endl;
		}
	}
}

// ------------------------------------------------------------------------------------------

void UniSetSubject::notify( UniSetTypes::TransportMessage& msg )
{
	for (ObserverList::iterator li=lst.begin();li!=lst.end();++li)
	{
		try
		{
			(*li)->push( msg );
		}
		catch(Exception& ex)
		{
			unideb[Debug::WARN] << ex << endl;
		}
	}
}

// ------------------------------------------------------------------------------------------
void UniSetSubject::attach( UniSetObject* o, int MessageType )
{
}
// ------------------------------------------------------------------------------------------
void UniSetSubject::detach( UniSetObject* o, int MessageType )
{
}
// ------------------------------------------------------------------------------------------
