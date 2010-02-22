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
 * \brief Реализация RepositoryAgent
 * \author Pavel Vainerman
 * \date  $Date: 2005/01/28 20:52:21 $
 * \version $Id: RepositoryAgent.h,v 1.5 2005/01/28 20:52:21 vitlav Exp $
*/
// -------------------------------------------------------------------------- 
#ifndef RepositoryAgent_H_
#define RepositoryAgent_H_
//---------------------------------------------------------------------------
#include "RepositoryAgent_i.hh"
#include "BaseProcess_i.hh"
#include "BaseProcess.h"
#include "UniSetTypes.h"
#include "ObjectIndex.h"
//----------------------------------------------------------------------------------------
/*! \class RepositoryAgent
*/ 
class RepositoryAgent: 
		public POA_RepositoryAgent_i,
		public BaseProcess
{
	public:

		RepositoryAgent( ObjectId id, const UniSetTypes::ObjectInfo *pObjectsMap );
		~RepositoryAgent();


//	  virtual void registration(const char* name, ::CORBA::Object_ptr ref);
//	  virtual void unregistration(const char* name, ::CORBA::Object_ptr ref);

	  virtual CORBA::Object_ptr resolve(const char* name);
	  virtual CORBA::Object_ptr resolveid( UniSetTypes::ObjectId id);
	  
	  virtual void execute();

	protected:
			RepositoryAgent();
			ObjectIndex oind;

	private:
};

#endif
