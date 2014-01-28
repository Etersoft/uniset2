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
 * \brief Базовый класс для реализации шаблона "наблюдатель"
 * \author Pavel Vainerman
 */
//---------------------------------------------------------------------------
#ifndef UniSetObserver_H_
#define UniSetObserver_H_
//---------------------------------------------------------------------------
#include <list>
#include "UniSetObject.h"
#include "MessageType.h"
// --------------------------------------------------------------------------
class UniSetSubject
{
	public:
		UniSetSubject(UniSetTypes::ObjectId id);
		~UniSetSubject();

		virtual void attach( UniSetObject* o );
		virtual void detach( UniSetObject* o );

		virtual void attach( UniSetObject* o, int MessageType );
		virtual void detach( UniSetObject* o, int MessageType );

	protected:
		UniSetSubject();
		virtual void notify(int notify, int data,
						UniSetTypes::Message::Priority priority=UniSetTypes::Message::Low);
		virtual void notify( UniSetTypes::TransportMessage& msg );

	private:
		typedef	std::list<UniSetObject*> ObserverList;
		ObserverList lst;
		UniSetTypes::ObjectId id;

};
//---------------------------------------------------------------------------
#endif
