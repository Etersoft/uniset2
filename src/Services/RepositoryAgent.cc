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

#include "Exceptions.h"
#include "RepositoryAgent.h"

// ------------------------------------------------------------------------------------------
using namespace UniSetTypes;

extern const ObjectInfo *pObjectsMap;
// ------------------------------------------------------------------------------------------

RepositoryAgent::RepositoryAgent():
	oind(pObjectsMap)
{
}

RepositoryAgent::~RepositoryAgent()
{
}

/*
RepositoryAgent::RepositoryAgent(const string name, const string section, int* argc, char* **argv):  
	BaseProcess_i(name, section, argc, argv)
{
}

RepositoryAgent::RepositoryAgent( ObjectId id, int* argc, char* **argv):  
	BaseProcess_i( id, argc, argv)
{
}

*/

RepositoryAgent::RepositoryAgent( ObjectId id, const ObjectInfo *pObjMap):
	BaseProcess(id),
	oind(pObjMap)
{
//	cout << ui.getNameById(id) << endl;
}

// ------------------------------------------------------------------------------------------
/*
void RepositoryAgent::registration(const char* name, ::CORBA::Object_ptr ref)
{
}
// ------------------------------------------------------------------------------------------
void RepositoryAgent::unregistration(const char* name, ::CORBA::Object_ptr ref)
{
}
*/
// ------------------------------------------------------------------------------------------
CORBA::Object_ptr RepositoryAgent::resolve(const char* name)
{
	cout << "resolve: " << name << endl;
	
	CORBA::Object_ptr ref;
	try
	{
		ref= ui.resolve(name);
	}
	catch(NameNotFound)
	{
		throw RepositoryAgent_i::NameNotFound();
	}
	catch(ORepFailed)
	{
		throw RepositoryAgent_i::ResolveError();
	}

	return CORBA::Object::_duplicate(ref);
}
// ------------------------------------------------------------------------------------------
CORBA::Object_ptr RepositoryAgent::resolveid(BaseObjectId id)
{
	cout << "resolveid: " << ui.getNameById(id) << endl;
	CORBA::Object_ptr ref;
	try
	{
		ref= ui.resolve(id);
	}
	catch(NameNotFound)
	{
		throw RepositoryAgent_i::NameNotFound();
	}
	catch(ORepFailed)
	{
		throw RepositoryAgent_i::ResolveError();
	}

	return CORBA::Object::_duplicate(ref);
}


// ------------------------------------------------------------------------------------------
void RepositoryAgent::execute()
{
	while(active)//	for(;;)
	{
		if ( waitMessage(&msg) )
		{
/*		
			switch(msg.type)
			{
				case MessageType::Command:
				{
					cout << BaseProcess_i::myname << ": msg id ="<< msg.id << endl;	
					break;
				}
				
				default:
					break;
			}
*/			
		}
	}
}
