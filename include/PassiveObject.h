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
// -------------------------------------------------------------------------
#ifndef PassiveObject_H_
#define PassiveObject_H_
// -------------------------------------------------------------------------

#include <string>
#include "UniSetTypes.h"
#include "MessageType.h"
#include "ProxyManager.h"
// -------------------------------------------------------------------------


/*!
 * Пасивный объект не имеющий самостоятельного потока обработки сообщений, но имеющий 
 * уникальный идентификатор. Предназначен для работы под управлением ProxyManager.
 * 
*/ 
class PassiveObject
{
	public:
		PassiveObject();
		PassiveObject( UniSetTypes::ObjectId id );
		PassiveObject( UniSetTypes::ObjectId id, ProxyManager* mngr );
		virtual ~PassiveObject();

		virtual void processingMessage( UniSetTypes::VoidMessage* msg );

		void setID( UniSetTypes::ObjectId id );
		void init(ProxyManager* mngr);
		
		inline UniSetTypes::ObjectId getId(){ return id; }
		inline std::string getName(){ return myname; }

	protected:
		virtual void sysCommand( UniSetTypes::SystemMessage *sm );
		virtual void askSensors( UniversalIO::UIOCommand cmd ){}
		virtual void timerInfo( UniSetTypes::TimerMessage *tm ){}
		virtual void sensorInfo( UniSetTypes::SensorMessage *sm ){}
	
		std::string myname;

		ProxyManager* mngr;
	private:
		UniSetTypes::ObjectId id;
};

// -------------------------------------------------------------------------
#endif // PassiveObject_H_
// -------------------------------------------------------------------------
