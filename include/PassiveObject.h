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

		virtual void processingMessage( const UniSetTypes::VoidMessage* msg );

		void setID( UniSetTypes::ObjectId id );
		void init(ProxyManager* mngr);

		inline UniSetTypes::ObjectId getId() const
		{
			return id;
		}
		inline std::string getName() const
		{
			return myname;
		}

	protected:
		virtual void sysCommand( const UniSetTypes::SystemMessage* sm );
		virtual void askSensors( UniversalIO::UIOCommand cmd ) {}
		virtual void timerInfo( const UniSetTypes::TimerMessage* tm ) {}
		virtual void sensorInfo( const UniSetTypes::SensorMessage* sm ) {}

		std::string myname = { "" };
		ProxyManager* mngr = { nullptr };

	private:
		UniSetTypes::ObjectId id = { UniSetTypes::DefaultObjectId };
};

// -------------------------------------------------------------------------
#endif // PassiveObject_H_
// -------------------------------------------------------------------------
