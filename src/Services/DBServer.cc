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
 *  \brief файл реализации DB-сервера
 *  \author Pavel Vainerman
*/
// -------------------------------------------------------------------------- 

#include <sys/time.h>
#include <sstream>
#include <iomanip>

#include "ORepHelpers.h"
#include "DBServer.h"
#include "Configuration.h"
#include "Debug.h"
#include "UniXML.h"
// ------------------------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
// ------------------------------------------------------------------------------------------
DBServer::DBServer(ObjectId id): 
	UniSetObject_LT(id)
{
	if( getId() == DefaultObjectId )
	{
		id = conf->getDBServer();
		if( id == DefaultObjectId )
		{
			ostringstream msg;
			msg << "(DBServer): Запуск невозможен! НЕ ОПРЕДЕЛЁН ObjectId !!!!!\n";
			throw Exception(msg.str());
		}
		setID(id);
	}
}

DBServer::DBServer(): 
	UniSetObject_LT(conf->getDBServer())
{
	if( getId() == DefaultObjectId )
	{
		ObjectId id = conf->getDBServer();
		if( id == DefaultObjectId )
		{
			ostringstream msg;
			msg << "(DBServer): Запуск невозможен! НЕ ОПРЕДЕЛЁН ObjectId !!!!!\n";
			throw Exception(msg.str());
		}
		setID(id);
	}
}
//--------------------------------------------------------------------------------------------
DBServer::~DBServer()
{
}
//--------------------------------------------------------------------------------------------
void DBServer::processingMessage( UniSetTypes::VoidMessage *msg )
{
	switch(msg->type)
	{
		case Message::SysCommand:
		{
			SystemMessage sm(msg);
			sysCommand(&sm);
			break;
		}
			
		case Message::Confirm:
		{
			ConfirmMessage cm(msg);
			parse(&cm);
			break;
		}
	
		case Message::SensorInfo:
		{
			SensorMessage sm(msg);
			parse(&sm);
			break;
		}
	
		default:
			break; 	
	}

}
//--------------------------------------------------------------------------------------------
bool DBServer::activateObject()
{
	UniSetObject_LT::activateObject();
	init_dbserver();
	return true;
}
//--------------------------------------------------------------------------------------------
