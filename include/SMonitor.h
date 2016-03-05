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
// -----------------------------------------------------------------------------
#ifndef SMonitor_H_
#define SMonitor_H_
// -----------------------------------------------------------------------------
#include <list>
#include <UniSetObject.h>
#include "UniSetTypes.h"
// -----------------------------------------------------------------------------
class SMonitor:
	public UniSetObject
{
	public:

		SMonitor( UniSetTypes::ObjectId id );
		~SMonitor();

		// -----
	protected:
		virtual void sysCommand( const UniSetTypes::SystemMessage* sm ) override;
		virtual void sensorInfo( const UniSetTypes::SensorMessage* si ) override;
		virtual void timerInfo( const UniSetTypes::TimerMessage* tm ) override;
		virtual void sigterm( int signo ) override;
		SMonitor();

	private:
		typedef std::list<UniSetTypes::ParamSInfo> MyIDList;
		MyIDList lst;
		std::string script;
};
// -----------------------------------------------------------------------------
#endif
// -----------------------------------------------------------------------------
